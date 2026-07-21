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
#define MAX_TCP_DEVICES 4

// ====================== 工业级网关资源管理器 核心结构体 ======================
typedef struct {
    char ip[64];
    int  port;
    int  slave_id;
    int  timeout_ms;      // 可选，超时时间
    uint16_t regs[32];
    modbus_t *ctx;
    int last_reported_status;
    int status ;
    char alarm_msg[128];
    time_t tcp_fail_time;
    int collect_enable;
    int last_collect_state
    
} tcp_device_config_t;



typedef struct {
    // 线程句柄
    pthread_t threads[8];

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
    float press;
    // 重连计数
    int tcp_retry;
    int rtu_retry;
    
    // rtu云端控制线程采集 1=开启采集  0=关闭采集
    int rtu_collect_enable; 
    int tcp_collect_enable;

      // ★★★ 新增：各模块状态（供MQTT线程读取上报） ★★★
    int rtu_status;      // 0=正常, 1=采集关闭, 2=离线故障
    int tcp_status;      // 0=正常, 1=采集关闭, 2=离线故障
    int can_status;      // 0=正常, 1=故障

    _Bool mqtt_connect_states;
    
    time_t rtu_fail_time;
    time_t tcp_fail_time;
    time_t can_fail_time;

    char rtu_alarm_msg[128];
    char tcp_alarm_msg[128];
    char can_alarm_msg[128];

    tcp_device_config_t tcp_devices[MAX_TCP_DEVICES];
    int tcp_device_count;

} gateway_manager_t;

// 全局唯一网关实例（所有文件共用）
// 写在这里：所有include本头文件的.c都能识别、不用重复extern
extern gateway_manager_t mgr;

#endif
