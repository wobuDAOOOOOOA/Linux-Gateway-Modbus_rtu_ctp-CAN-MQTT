#include<stdio.h>
#include"modbus_tcp.h"
#include"can.h"
int main(void)
{
   modbus_t *ctx = modbus_tcp_bconnect();
    while(1)
    {
    
    modbus_read(ctx);   
   
   }

}
