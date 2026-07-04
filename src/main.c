#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
#include"mqtt_huawei.h"
#include"log.h"
#include <linux/can.h>
#include <sys/socket.h>
#include"config.h"
#define ture 1
// ====================== 工业级网关资源管理器 核心结构体 ======================
typedef struct {
    // 线程句柄
    pthread_t threads[4];

    // 通信句柄
    modbus_t *rtu_ctx;
    modbus_t *tcp_ctx;

    // 同步互斥锁
    pthread_mutex_t data_mutex;
    pthread_mutex_t bus_mutex;

    // 全局运行状态机
    uint8_t running;

    // 业务数据缓存
    uint16_t regs[2];
    unsigned short rtu_data[64];
    float latest_temperature;
    float latest_humidity;

    // 重连计数
    int tcp_retry;
    int rtu_retry;
} gateway_manager_t;

// 全局唯一网关管理器实例（整个程序仅此一个）
static gateway_manager_t g_mgr;

// ====================== 资源初始化函数 ======================
static void gateway_manager_init(gateway_manager_t *mgr)
{
    memset(mgr, 0, sizeof(gateway_manager_t));

    // 初始化锁
    pthread_mutex_init(&mgr->data_mutex, NULL);
    pthread_mutex_init(&mgr->bus_mutex, NULL);
    
    // 开启运行开关
    mgr->running = ture;
    // 初始化默认寄存器数据
    mgr->regs[0] = 1;
    mgr->regs[1] = 2;
}

// ====================== 工业级终极清理函数 ======================
void gateway_cleanup(void)
{
    LOG_INFO("【工业清理】执行网关全资源释放...");
    gateway_manager_t *mgr = &g_mgr;

    // 关闭TCP连接
    if (mgr->tcp_ctx) {
        modbus_close(mgr->tcp_ctx);
        modbus_free(mgr->tcp_ctx);
        mgr->tcp_ctx = NULL;
    }

    // 关闭RTU串口连接
    if (mgr->rtu_ctx) {
        modbus_close(mgr->rtu_ctx);
        modbus_free(mgr->rtu_ctx);
        mgr->rtu_ctx = NULL;
    }

    // 销毁锁资源
    pthread_mutex_destroy(&mgr->data_mutex);
    pthread_mutex_destroy(&mgr->bus_mutex);

    LOG_INFO("【工业清理】所有资源释放完成，零泄漏！");
}

// ====================== 所有线程函数（全部改用传参上下文） ======================
void *modbus_tcp_read(void *arg) {
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    while (mgr->running) {
        if (modbus_robust_read(&mgr->tcp_ctx, 0, 10, mgr->regs) == -1) {
            printf("TCP:重连失败，5秒后继续重试\n");
            sleep(5);
            continue;
        }

        for (int i = 0; i < 2; i++) {
            printf("reg[%d] = %d\n", i, mgr->regs[i]);
        }
        my_can_send(0x123, 2, mgr->regs);
        sleep(1);
    }
    return NULL;
}

void *modbus_rtu_read(void *arg) {
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    while (mgr->running) {
        if (modbus_rtu_robust_read(&mgr->rtu_ctx,0,2,mgr->rtu_data) == -1)
        {
            LOG_ERROR("RTU:重连失败,5秒后继续重试\n");
            sleep(5);
            continue;
        }
        for (int i = 0; i < 2; i++) {
            LOG_INFO("RTU_DATA[%d] = %d\n", i, mgr->rtu_data[i]);
        }
        pthread_mutex_lock(&mgr->bus_mutex);
        my_can_send(0x123, 2, mgr->rtu_data);
        pthread_mutex_unlock(&mgr->bus_mutex);
        sleep(1);
    }
    return NULL;
}

void *can_receive_pthread(void *arg) {
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    unsigned char data[8];
    int len;

    while (mgr->running) {
        if (can_receive(data, &len) == 0) {
            pthread_mutex_lock(&mgr->data_mutex);
            if (len >= 2) {
                mgr->latest_humidity = data[0];
                mgr->latest_temperature = data[1];
            }
            pthread_mutex_unlock(&mgr->data_mutex);
        }
        sleep(1);
    }
    return NULL;
}

void *MQTT_pthread(void *arg) {
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    while (mgr->running) {
        pthread_mutex_lock(&mgr->bus_mutex);
        MQTT_publish(mgr->latest_temperature, mgr->latest_humidity);
        pthread_mutex_unlock(&mgr->bus_mutex);
        sleep(1);
    }
    return NULL;
}

// ====================== 主函数 ======================
int main(void) {
    // 1. 注册全局终极清理（工业级核心保障）
    atexit(gateway_cleanup);

    // 2. 初始化网关总管（所有资源统一初始化）
    gateway_manager_init(&g_mgr);

    // 3. 加载配置
    config_set_default(&cfg);
    config_load("./gateway.conf", &cfg);
    printf("Modbus port: %s\n", cfg.modbus_port);
    printf("can port: %s\n", cfg.can_interface);

    // 4. 底层外设初始化
    mqtt_Init();
    can_Init();

    // 5. 创建线程：统一传递管理器指针（彻底告别全局变量）
    pthread_create(&g_mgr.threads[0], NULL, modbus_rtu_read, &g_mgr);
    pthread_create(&g_mgr.threads[1], NULL, modbus_tcp_read, &g_mgr);
    pthread_create(&g_mgr.threads[2], NULL, can_receive_pthread, &g_mgr);
    pthread_create(&g_mgr.threads[3], NULL, MQTT_pthread, &g_mgr);

    // 主线程循环
    while(1)
    {
        LOG_DEBUG("主线程运行中...");
        sleep(1);
    }
}
