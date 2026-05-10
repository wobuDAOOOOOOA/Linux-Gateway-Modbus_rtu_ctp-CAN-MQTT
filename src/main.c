#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
#include"mqtt_huawei.h"
#include"log.h"
uint16_t regs[64];
modbus_t *ctx = NULL;// 初始为 NULL，robust_read 内部会创建连接
modbus_t *RTU_CTX = NULL;
short unsigned int RTU_DATA[64];
int tcp_retry = 0 ,rtu_retry = 0;



// 线程函数（格式固定，参数和返回值都是 void*）
void *modbus_rtu_read(void *arg) {
    while (1) {
        // //TCP

       if (modbus_robust_read(&ctx, 0, 10, regs) == -1) {
            printf("TCP:重连失败，5秒后继续重试\n");
            sleep(5);
            continue;
        }
       
        for (int i = 0; i < 10; i++) {
            printf("reg[%d] = %d\n", i, regs[i]);
        }
             my_can_send(0x123,2,regs);
        sleep(1);
    }
    return NULL;
}




// 线程函数（格式固定，参数和返回值都是 void*）
void *modbus_tcp_read(void *arg) {
    while (1) {
       // //RTU
        if (modbus_rtu_robust_read(&RTU_CTX,0,2,RTU_DATA) == -1)
        {
            LOG_ERROR("RTU:重连失败,100秒后继续重试\n");
            sleep(5);
            continue;
        }
         for (int i = 0; i < 2; i++) {
            LOG_INFO("RTU_DATA[%d] = %d\n", i,RTU_DATA[i]);
        }
    my_can_send(0x123, 2, RTU_DATA);
        sleep(1);
    }
    return NULL;
}



// 线程函数（格式固定，参数和返回值都是 void*）
void *can_receive_pthread(void *arg) {
    while (1) {
        printf("线程2工作中...\n");
        sleep(1);
    }
    return NULL;
}















int main(void) {
    pthread_t tids[3];
pthread_create(&tids[0], NULL, modbus_rtu_read, NULL);
pthread_create(&tids[1], NULL, modbus_tcp_read, NULL);
pthread_create(&tids[2], NULL, can_receive_pthread, NULL);
    while(1)

{  
    printf("主线程工作中...\n");
        sleep(1);

}  

}