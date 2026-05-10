#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
#include"mqtt_huawei.h"
#include"log.h"
#include <linux/can.h>
#include <sys/socket.h>
#include <unistd.h>    // for read()

// 声明外部变量 s，它在 can.c 中定义
uint16_t regs[2]={1,2};
modbus_t *ctx = NULL;// 初始为 NULL，robust_read 内部会创建连接
modbus_t *RTU_CTX = NULL;
short unsigned int RTU_DATA[64];
int tcp_retry = 0 ,rtu_retry = 0;
static float latest_temperature = 0.0;
static float latest_humidity = 0.0;
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// 线程函数（格式固定，参数和返回值都是 void*）
void *modbus_tcp_read(void *arg) {
    while (1) {
        // //TCP

       if (modbus_robust_read(&ctx, 0, 10, regs) == -1) {
            printf("TCP:重连失败，5秒后继续重试\n");
            sleep(5);
            continue;
        }
       
        for (int i = 0; i < 2; i++) {
            printf("reg[%d] = %d\n", i, regs[i]);
        }
       // pthread_mutex_lock(&mutex);
       my_can_send(0x123, 2, regs);
        //pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}




// // 线程函数（格式固定，参数和返回值都是 void*）
void *modbus_rtu_read(void *arg) {
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
       pthread_mutex_lock(&mutex);
        my_can_send(0x123, 2, RTU_DATA);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}



// 线程函数（格式固定，参数和返回值都是 void*）
void *can_receive_pthread(void *arg) {
    unsigned char data[8];
    int len;

    while (1) {

        if (can_receive(data, &len) == 0) {
            pthread_mutex_lock(&data_mutex);
            if (len >= 2) {
                latest_humidity = data[0];
                latest_temperature = data[1];
            }
            pthread_mutex_unlock(&data_mutex);
        }
       sleep(1);
 // 不需要 usleep，因为 can_receive 内部会阻塞等待数据
    }
    return NULL;
}


// 线程函数（格式固定，参数和返回值都是 void*）
void *MQTT_pthread(void *arg) {

    while (1) {
        pthread_mutex_lock(&mutex);
        MQTT_publish(2,2);
        pthread_mutex_unlock(&mutex);
sleep(1);
    }
    return NULL;
}




 // pthread_mutex_lock(&mutex);
       // pthread_mutex_unlock(&mutex)







int main(void) {

    pthread_t tids[4];
mqtt_Init();
can_Init();

pthread_create(&tids[0], NULL, modbus_rtu_read, NULL);
pthread_create(&tids[1], NULL, modbus_tcp_read, NULL);
pthread_create(&tids[2], NULL, can_receive_pthread, NULL);
pthread_create(&tids[3], NULL, MQTT_pthread, NULL);
  

    while(1)
{  
        LOG_DEBUG("主线程");

//   struct can_frame frame;
//         int nbytes = read(s, &frame, sizeof(frame));
//         if (nbytes > 0) {
//             LOG_INFO("主线程原生 read 收到: ID=%x", frame.can_id);
//         }


  sleep(1);

}

}  

