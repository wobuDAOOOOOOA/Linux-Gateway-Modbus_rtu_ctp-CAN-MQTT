/*********************************************
*  BMP280 Linux I2C 完整驱动
*  适配架构：标准Linux硬件I2C子系统 + 字符设备驱动
*  对应你的核心思路：
*  1. 硬件I2C不写SDA/SCL引脚，挂靠父级I2C总线
*  2. i2c_driver框架自动匹配设备树、自动执行probe
*  3. probe初始化硬件 + 自动创建/dev设备节点
*  4. file_operations对外暴露read入口
*  5. 应用层read()系统调用读取内核数据
*  6. 内核只通信、不浮点运算，浮点全部放用户层
**********************************************/

// 内核必备头文件
#include <linux/module.h>       // 驱动模块入口、出口、许可证
#include <linux/kernel.h>       // 内核打印、基础内核函数
#include <linux/fs.h>           // 文件操作结构体file_operations
#include <linux/cdev.h>         // 字符设备注册、注销核心API
#include <linux/uaccess.h>      // 内核 <-> 用户态数据拷贝(copy_to_user)
#include <linux/i2c.h>          // Linux标准I2C子系统、i2c_master收发API
#include <linux/delay.h>        // 内核延时函数msleep
#include <linux/device.h>       // 自动创建设备节点、设备类

/*********************************************
* 宏定义区域（统一管理，方便后期修改）
**********************************************/
#define BMP280_DEV_NAME      "bmp280_dev"    // /dev/xxx 设备节点名称
#define BMP280_CLASS_NAME    "bmp280_class"  // 设备类名称(系统自动生成)
#define DEV_COUNT            1               // 驱动设备数量，单设备写1

/*********************************************
* BMP280 芯片寄存器地址（硬件手册固定值）
**********************************************/
#define BMP280_REG_ID        0xD0    // 芯片ID寄存器，用于验证设备是否在线
#define BMP280_REG_RESET     0xE0    // 软件复位寄存器
#define BMP280_REG_CTRL_MEAS 0xF4    // 测量配置寄存器（采样率、工作模式）
#define BMP280_REG_DATA      0xF7    // 温湿度原始ADC数据起始寄存器
/*********************************************
* 私有设备结构体（驱动全局数据容器）
* 大白话：把当前驱动所有资源全部打包在一起
* 一个结构体管理所有东西，规范、易维护
**********************************************/
struct bmp280_dev {
    struct cdev cdev;               // 标准字符设备结构体（内核必备）
    struct i2c_client *client;      // I2C设备客户端（内核自动匹配生成）
    dev_t devno;                    // 设备号（主设备号+次设备号）
    struct class *dev_class;        // 设备类指针，用于自动创节点

    // 从传感器读取的原始20位数据（内核只存原始值，不做浮点）
    u32 raw_temp;
    u32 raw_press;
};

// 全局私有结构体指针，整个驱动通用
static struct bmp280_dev *bmp280_priv;


/*********************************************
* 函数：probe 设备匹配回调
* 执行时机：
* 内核启动/加载ko -> 扫描设备树 -> compatible匹配成功 -> 自动执行probe
* probe只做三件大事：
* 1、申请内存、保存i2c客户端
* 2、注册字符设备、自动创建/dev节点
* 3、初始化传感器硬件
**********************************************/
static int bmp280_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    // 申请内核内存，分配私有结构体
    bmp280_priv = devm_kzalloc(&client->dev, sizeof(struct bmp280_dev), GFP_KERNEL);
    if (!bmp280_priv)
        return -ENOMEM;

    // 保存当前i2c设备信息，后续读写全部依赖此client
    bmp280_priv->client = client;
    i2c_set_clientdata(client, bmp280_priv);

    // 1、自动分配设备号（不用自己手动指定）
   // 我要申请设备号 → 得有地方存（私有结构体devno）→ 从0开始次设备号 → 设备数量1 → 设备名。
    ret = alloc_chrdev_region(&bmp280_priv->devno, 0, DEV_COUNT, BMP280_DEV_NAME);
    if (ret < 0)
        goto err_alloc;

    // 2、绑定文件操作集，告诉内核：这个设备用这一套read/open接口
    //推导逻辑：初始化哪个cdev？——自己priv里的cdev；绑定哪个读写函数？——自己定义的fops。
    cdev_init(&bmp280_priv->cdev, &bmp280_fops);//私有cdev ＋ 自己的fops
    
    // 标记模块所有者，防止模块卸载后内核仍调用设备接口，导致崩溃
    bmp280_priv->cdev.owner = THIS_MODULE;

    ret = cdev_add(&bmp280_priv->cdev, bmp280_priv->devno, DEV_COUNT);
    if (ret < 0)
        goto err_cdev;

    // 3、创建设备类、自动在/dev下生成设备节点
    bmp280_priv->dev_class = class_create(THIS_MODULE, BMP280_CLASS_NAME);
    if (IS_ERR(bmp280_priv->dev_class)) {
        ret = PTR_ERR(bmp280_priv->dev_class);
        goto err_class;
    }
    device_create(bmp280_priv->dev_class, NULL, bmp280_priv->devno, NULL, BMP280_DEV_NAME);

    // 4、初始化BMP280传感器硬件
    ret = bmp280_sensor_init(client);
    if (ret < 0)
        goto err_init;

    dev_info(&client->dev, "bmp280 probe ok, /dev/%s ready\n", BMP280_DEV_NAME);
    return 0;

    // 逐层出错资源释放（内核驱动必须干净释放资源）
err_init:
    device_destroy(bmp280_priv->dev_class, bmp280_priv->devno);
    class_destroy(bmp280_priv->dev_class);
err_class:
    cdev_del(&bmp280_priv->cdev);
err_cdev:
    unregister_chrdev_region(bmp280_priv->devno, DEV_COUNT);
err_alloc:
    devm_kfree(&client->dev, bmp280_priv);
    return ret;
}