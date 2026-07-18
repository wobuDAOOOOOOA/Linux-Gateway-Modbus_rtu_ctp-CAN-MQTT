#include <linux/module.h> //内核模块入口
#include <linux/kernel.h> //内核打印内核标准头文件
#include <linux/delay.h> //内核专用延时函数头文件
#include <linux/cdev.h>  //字符设备用头文件
#include <linux/i2c.h> //内核i2c标准头文件
#include <linux/device.h>  //自动分配设备号，自动创建设备节点
#include <linux/uaccess.h>
#include <linux/fs.h> //文件操作结构体，系统调用映射

#define BMP280_DEV_NAME "bmp280_dev"
#define BMP280_DEV_CLASS "bmp280_class"
#define DEV_COUNT "1"

struct bmp280_drv{
    struct cdev cdev;    //标准字符设备结构体
    struct i2c_client *client;//i2c设备客户端
    dev_t devno;            //主次设备号
    struct class *dev_class; //设备类指针，用于自动创建节点
 
    u32_t raw_press_raw;
    u32_t raw_temp;

}
static struct  bmp280_drv *bmp280_prive;


static int bmp280_probe(struct i2c_client *client ,const struct i2c_device_id *id)
{
    
int ret;
  bmp280_drv=devm_kzalloc(client->dev,sizeof(struct bmp280_drv),GPF_KERNEL);   //给私有结构体分配内存，这里面的client->是内核扫描设备树
  //后创建的真正的设备句柄，直接用就行
  if(!bmp280_drv)
  {
  return -ENOMEN;
  }

  bmp280_prive->client = client;  //这里将设备句柄赋值给私有结构体，后面我们调用设备进行读写都用私有结构体
  i2c_set_clientdata(client,bmp280_prive);//这进行是把私有结构体给内核，让内核后面执行remove的时候能找到我的结构体清理资源
  //不然后面移除内核模块的时候会报错。


 ret= alloc_chrdev_region(&bmp280_prive->devno,0,DEV_COUNT,BMP280_DEV_NAME);  //自动分配设备号
 if(ret<0) goto erralloc;

 cdev_init(&bmp280_prive->cdev,&bmp280_ops);   //告诉内核当前字符设备的read open等回调函数函数用的是我自己定义的bmp280_ops
 bmp280_drv->cdev.owner = THIS_MODULE;         //拥有者是这个内核模块，防止模块卸载后内核仍调用设备接口，导致崩溃
ret =  cdev_add(&bmp280_prive->cdev,bmp280_prive->devno,DEV_COUNT);  //把字符设备真正注册进字符设备子系统
if(ret<0) goto err_cdev;

bmp280_prive->dev_class = class_create(THIS_MODULE,BMP280_DEV_CLASS);  //创建设备类
if(IS_ERR(bmp280_prive->dev_class))
{
    ret = IS_ERR(bmp280_prive->dev_class);
    goto errclass;
}
device_create(bmp280_prive->dev_class,NULL,bmp280_prive->devno,NULL,BMP280_DEV_NAME);  自动创建设备节点，再次之前要创建设备类

///////////////////////用手抄一遍！！！！！！！




//先分配内存，给谁分配
bmp280_drv = devm_kzalloc(struct *client



);






 





}


