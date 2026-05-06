#include<stdio.h>
#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
#include"log.h"
#include"can.h"
uint16_t regs[64];
modbus_t *ctx = NULL;

modbus_t *RTU_CTX = NULL;
short unsigned int RTU_DATA[64];
int tcp_retry = 0 ,rtu_retry = 0;

int main(void) {

  //  modbus_t *ctx = NULL;   // 初始为 NULL，robust_read 内部会创建连接


    while (1) {
//TCP

       if (modbus_robust_read(&ctx, 0, 10, regs) == -1) {
            printf("TCP:重连失败，5秒后继续重试\n");
            sleep(5);
            continue;
            // tcp_retry++;
            // if(tcp_retry == 2 )
            // {
            //     LOG_ERROR("TCP重连以达到最大次数,请检查连接");
            // }
           // else  continue;

        }
       
        for (int i = 0; i < 10; i++) {
            printf("reg[%d] = %d\n", i, regs[i]);
        }
             my_can_send(0x123,2,RTU_DATA);

//RTU
        if (modbus_rtu_robust_read(&RTU_CTX,0,2,RTU_DATA) == -1)
        {
            LOG_ERROR("RTU:重连失败,100秒后继续重试\n");
            sleep(5);
            // rtu_retry++;
            //  if(rtu_retry == 2 )
            // {
            //     LOG_ERROR("RTU重连以达到最大次数,请检查连接");
            // }
            // else  continue;
            continue;
        }
         for (int i = 0; i < 2; i++) {
            LOG_INFO("RTU_DATA[%d] = %d\n", i,RTU_DATA[i]);
        }
        sleep(1);
    my_can_send(0x123, 2, RTU_DATA);

    }
    return 0;
}