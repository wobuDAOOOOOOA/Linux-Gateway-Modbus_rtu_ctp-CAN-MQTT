#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>

modbus_t* RTU_ctx = NULL;
int16_t RTU_DATA[64];
int8_t RTU_rc = 0;
int  modbus_RTU_read()
{
    //创建上下文，里面要传入设备位置，波特率，校验位，数据位，停止位
RTU_ctx = modbus_new_rtu("/dev/ttyACM0",4800,'N',8,1);
if(RTU_ctx == NULL)
{
fprintf(stderr,"无法建立上下文%s\n",modbus_strerror(errno));
return -1;
}
//建立上下文成功就尝试连接
if(modbus_connect(RTU_ctx) == -1)
{
 fprintf(stderr,"连接失败,错误：%s",modbus_strerror(errno));
 modbus_free(RTU_ctx);
return -1;
}
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

}