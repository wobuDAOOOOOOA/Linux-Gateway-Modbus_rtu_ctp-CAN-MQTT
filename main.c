#include<stdio.h>
#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
#include"log.h"
uint16_t regs[64];
modbus_t *ctx = NULL;

modbus_t *RTU_CTX = NULL;
short unsigned int RTU_DATA[64];

int main(void) {


  //  modbus_t *ctx = NULL;   // 初始为 NULL，robust_read 内部会创建连接


    while (1) {

       /*if (modbus_robust_read(&ctx, 0, 10, regs) == -1) {
            printf("重连失败，5秒后继续重试\n");
            sleep(5);
            continue;
        }
       
        for (int i = 0; i < 10; i++) {
            printf("reg[%d] = %d\n", i, regs[i]);
        }*/ 

        if (modbus_rtu_robust_read(&RTU_CTX,0,2,RTU_DATA) == -1)
        {
            LOG_ERROR("重连失败,5秒后继续重试\n");
            sleep(5);
            continue;/* code */
        }
         for (int i = 0; i < 2; i++) {
            LOG_INFO("RTU_DATA[%d] = %d\n", i,RTU_DATA[i]);
        }
        sleep(1);
    }
    return 0;
}