#ifndef GATEWAY_H
#define GATEWAY_H

/*************************************************
* 工业网关全局资源管理头文件
* 功能：统一所有线程、句柄、状态、业务变量
* 存放：结构体完整定义 + 全局变量声明
* 工程规范：所有模块(mqtt/rtu/main)统一包含此头文件
**************************************************/
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <modbus/modbus.h>

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
    
    // rtu云端控制线程采集 1=开启采集  0=关闭采集
    int rtu_collect_enable; 
} gateway_manager_t;

// 全局唯一网关实例（所有文件共用）
// 写在这里：所有include本头文件的.c都能识别、不用重复extern
extern gateway_manager_t mgr;

#endif
