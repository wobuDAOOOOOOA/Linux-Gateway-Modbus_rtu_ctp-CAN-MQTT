#include<stdio.h>
#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
uint16_t regs[64];
modbus_t *ctx = NULL;



int main(void)
{
 while (1) {
    modbus_RTU_read();
  /* if (modbus_robust_read(&ctx, 0, 10, regs) == -1) {
        printf("重连失败，退出或继续等待\n");
        sleep(5);
        continue;
    }

    // 正常处理 regs 数据
    for (int i = 0; i < 10; i++) {
        printf("reg[%d] = %d\n", i, regs[i]);
    }
    sleep(1);*/ 

}

}
