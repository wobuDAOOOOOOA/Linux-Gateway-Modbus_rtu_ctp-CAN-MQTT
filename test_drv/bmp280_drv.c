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
* 函数：bmp280_i2c_write
* 功能：I2C 寄存器单字节写入（内核标准封装）
* 大白话：先发寄存器地址，再发写入的值
* 底层依赖内核：i2c_master_send 标准多字节发送API
**********************************************/
static int bmp280_i2c_write(struct i2c_client *client, u8 reg, u8 val)
{
    // I2C写时序：【寄存器地址 + 写入数据】两段式
    u8 buf[2] = {reg, val};
    // 直接调用内核I2C核心API，无需自己写时序
    return i2c_master_send(client, buf, 2);
}

/*********************************************
* 函数：bmp280_i2c_read
* 功能：I2C 寄存器多字节读取
* 大白话：
* 1、先告诉芯片我要读哪个寄存器（发reg）
* 2、等待芯片返回数据，存入buf
* 完全依赖内核I2C子系统，不用操作SDA/SCL时序
**********************************************/
static int bmp280_i2c_read(struct i2c_client *client, u8 reg, u8 *buf, u8 len)
{
    int ret;
    // 第一步：发送要读取的寄存器地址
    ret = i2c_master_send(client, &reg, 1);
    if (ret < 0)
        return ret;
    // 第二步：读取返回的多字节数据
    return i2c_master_recv(client, buf, len);
}

/*********************************************
* 函数：bmp280_sensor_init
* 功能：传感器硬件初始化（probe中只执行一次）
* 流程：校验设备ID -> 软件复位 -> 配置测量模式
**********************************************/
static int bmp280_sensor_init(struct i2c_client *client)
{
    int ret;
    u8 chip_id = 0;

    // 1、读取芯片ID，判断设备是否真的挂载、通信是否正常
    ret = bmp280_i2c_read(client, BMP280_REG_ID, &chip_id, 1);
    if (ret < 0 || chip_id != 0x58) {
        dev_err(&client->dev, "bmp280 check failed, id=0x%02X\n", chip_id);
        return -ENODEV;
    }

    // 2、软件复位传感器，恢复默认状态
    bmp280_i2c_write(client, BMP280_REG_RESET, 0xB6);
    msleep(20);  // 等待复位完成

    // 3、配置工作模式：正常工作模式、温度气压单次采样
    bmp280_i2c_write(client, BMP280_REG_CTRL_MEAS, 0x27);
    msleep(10);

    dev_info(&client->dev, "bmp280 init success\n");
    return 0;
}

/*********************************************
* 函数：bmp280_get_raw_data
* 功能：读取最新原始温压数据、解析原始ADC值
* 大白话：内核只干「读数据、拼数据」
* 不做浮点运算、不做物理换算，避免内核报错
**********************************************/
static int bmp280_get_raw_data(struct bmp280_dev *priv)
{
    u8 buf[6] = {0};
    int ret;

    // 一次性读取6字节原始数据（0xF7起始6字节）
    ret = bmp280_i2c_read(priv->client, BMP280_REG_DATA, buf, 6);
    if (ret < 0)
        return ret;

    // 按照BMP280手册，拼接20位高精度原始气压数据
    priv->raw_press = (u32)((buf[0] << 16) | (buf[1] << 8) | buf[2]) >> 4;
    // 拼接20位高精度原始温度数据
    priv->raw_temp  = (u32)((buf[3] << 16) | (buf[4] << 8) | buf[5]) >> 4;

    return 0;
}

/*********************************************
* 函数：bmp280_read
* 所属：file_operations 对外接口
* 触发时机：应用层调用 read(fd, buf, len) 自动进入
* 整条链路：
* 应用层read() -> VFS -> 驱动.read -> I2C读数据 -> copy_to_user回传
**********************************************/
static ssize_t bmp280_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int ret;
    char k_buf[64] = {0};

    // 1、读取最新传感器原始数据
    ret = bmp280_get_raw_data(bmp280_priv);
    if (ret < 0)
        return -EIO;

    // 2、内核组装字符串：原始气压、原始温度
    snprintf(k_buf, sizeof(k_buf), "press:%u,temp:%u\n",
                bmp280_priv->raw_press, bmp280_priv->raw_temp);

    // 3、核心！跨层数据拷贝：内核态 --> 用户态
    // 内核数据不能直接给APP，必须用copy_to_user
    if (copy_to_user(buf, k_buf, strlen(k_buf))) {
        return -EFAULT;
    }

    // 返回成功拷贝的字节数
    return strlen(k_buf);
}

/*********************************************
* 函数：bmp280_open
* 触发时机：应用层 open("/dev/bmp280_dev") 触发
**********************************************/
static int bmp280_open(struct inode *inode, struct file *file)
{
    return 0;
}

/*********************************************
* 文件操作结构体：应用层与内核的【唯一入口】
* 大白话：
* APP只能调用Linux标准系统调用 open/read/write
* 最终全部映射到这里的驱动函数
**********************************************/
static struct file_operations bmp280_fops = {
    .owner = THIS_MODULE,
    .open  = bmp280_open,
    .read  = bmp280_read,
};

/*********************************************
* 设备树匹配表
* 作用：和DTS中compatible字符串一一对应
* MODULE_DEVICE_TABLE：公开匹配表，让内核扫描识别
**********************************************/
static const struct of_device_id bmp280_of_match[] = {
    { .compatible = "bmp280,press-temp-sensor" },
    { /* 必须空结尾 */ }
};
MODULE_DEVICE_TABLE(of, bmp280_of_match);

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
    ret = alloc_chrdev_region(&bmp280_priv->devno, 0, DEV_COUNT, BMP280_DEV_NAME);
    if (ret < 0)
        goto err_alloc;

    // 2、绑定文件操作集，告诉内核：这个设备用这一套read/open接口
    cdev_init(&bmp280_priv->cdev, &bmp280_fops);
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

/*********************************************
* 函数：remove 设备卸载回调
* 执行时机：rmmod 卸载驱动时自动执行
* 作用：释放所有内核资源，防止内存泄漏
**********************************************/
static int bmp280_remove(struct i2c_client *client)
{
    struct bmp280_dev *priv = i2c_get_clientdata(client);

    // 销毁设备节点、设备类、注销设备号、释放内存
    device_destroy(priv->dev_class, priv->devno);
    class_destroy(priv->dev_class);
    cdev_del(&priv->cdev);
    unregister_chrdev_region(priv->devno, DEV_COUNT);
    devm_kfree(&client->dev, priv);

    dev_info(&client->dev, "bmp280 remove done\n");
    return 0;
}

/*********************************************
* I2C驱动核心结构体
* 大白话：
* 把probe、remove、匹配表统一注册给内核I2C子系统
* 没有这个结构体绑定，你的probe只是普通废函数，不会执行
**********************************************/
static struct i2c_driver bmp280_i2c_driver = {
    .probe  = bmp280_probe,
    .remove = bmp280_remove,
    .driver = {
        .name = "bmp280-i2c-driver",
        .of_match_table = bmp280_of_match,
    },
};

/*********************************************
* 模块入口出口
**********************************************/
module_i2c_driver(bmp280_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Study");
MODULE_DESCRIPTION("BMP280 I2C Driver (Standard Framework Same As AHT20)");