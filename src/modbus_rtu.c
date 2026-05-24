#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
#include"log.h"
#include"config.h"

#define RTU_MAXretry 3
#define RTU_retrytime 5
extern gateway_config_t cfg;  // 告诉编译器：cfg 在别的文件里定义好了

modbus_t* RTU_ctx = NULL;
int8_t RTU_rc = 0;
modbus_t* modbus_RTU_bconnect(void)
{
           LOG_DEBUG("RTU:进入modbus_RTU创建连接函数");
                   LOG_INFO("RTU:modbus_RTU创建连接...");
    //创建上下文，里面要传入设备位置，波特率，校验位，数据位，停止位
RTU_ctx = modbus_new_rtu(cfg.modbus_port,cfg.modbus_baudrate,'N',8,1);
if(RTU_ctx == NULL)
{
LOG_ERROR("RTU:无法建立上下文%s\n",modbus_strerror(errno));
return -1;
}
//建立上下文成功就尝试连接
if(modbus_connect(RTU_ctx) == -1)
{
 LOG_ERROR("RTU:连接失败,错误：%s\n",modbus_strerror(errno));
 modbus_free(RTU_ctx);
return -1;
}
return RTU_ctx;
}

int modbus_rtu_robust_read(modbus_t **RTU_ctx,int addr, int nb, uint16_t *dest)
{
int8_t retry=0;
int8_t rc;
LOG_DEBUG("RTU:进入modbus_RTU重连函数");

while(retry < RTU_MAXretry)
{
    if(*RTU_ctx == NULL)
    {
        LOG_ERROR("RTU:连接句柄为空，尝试第%d次重连\n",retry+1);
        *RTU_ctx = modbus_RTU_bconnect();
           if (*RTU_ctx == NULL)
    {
        retry++;
    }
        if(retry < RTU_MAXretry)
        { 
            LOG_ERROR("RTU:重连失败，尝试%d秒后重新建立连接\n",RTU_retrytime);
            sleep(RTU_retrytime);
           continue; //跳过接下来的while的程序，重新回到while开头
        }
    }
    
       //成功就set_slave设置从机地址

modbus_set_slave(*RTU_ctx,cfg.modbus_slave_id);
//然后用modbus_read_registers
rc = modbus_read_registers(*RTU_ctx,0,2,dest);
if(rc != -1)
{
  LOG_INFO("RTU:连接成功，返回%d个寄存器数据\n",rc);
  return rc;
}
    LOG_ERROR("RTU:连接失败,尝试释放并重新连接\n");
     modbus_close(*RTU_ctx);
        modbus_free(*RTU_ctx);
        *RTU_ctx = NULL;   // 重要：把主调函数里的 ctx 也置空，下次循环会重新创建
        retry++;
   if(retry < RTU_MAXretry)
   {
  LOG_ERROR("RTU:重连失败，尝试%d秒后重新建立连接\n",RTU_retrytime);
            sleep(RTU_retrytime);
   }

 }
    LOG_ERROR("RTU:重连失败，达到最大尝试数：%d\n",RTU_MAXretry);
   return -1;
}



/*
//成功就set_slave设置从机地址

modbus_set_slave(RTU_ctx,1);
//然后用modbus_read_registers
RTU_rc = modbus_read_registers(RTU_ctx,0,2,RTU_DATA);
for (int i = 0; i < RTU_rc; i++)
{
    printf("data[%d]:%d",i,RTU_DATA[i]);
}
RTU_ctx = NULL;
return 0;

}*/