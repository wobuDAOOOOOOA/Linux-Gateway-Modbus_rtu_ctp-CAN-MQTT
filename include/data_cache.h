#ifndef DATA_CACHE_H
#define DATA_CACHE_H

#include <stdbool.h>
#include <time.h>

#define CACHE_SIZE 256  // 最多缓存256条数据

typedef enum {
    CACHE_TYPE_TELEMETRY = 0,   // 温湿度+大气压数据
    CACHE_TYPE_ALARM_RTU,       // RTU告警
    CACHE_TYPE_ALARM_TCP        // TCP告警
} cache_data_type_t;

typedef struct {
    cache_data_type_t type;
    time_t timestamp;
    
    // 遥测数据（温湿度+大气压）
    float temperature;
    float humidity;
    float pressure;
    
    // 告警数据
    char alarm_type[32];
    char alarm_module[32];
    char alarm_msg[128];
} data_record_t;

typedef struct {
    data_record_t records[CACHE_SIZE];
    int head;
    int tail;
    int count;
    bool full;
} data_cache_t;

// 函数声明
void data_cache_init(void);
int data_cache_push_telemetry(float temp, float humi, float press);
int data_cache_push_alarm_rtu(const char *type, const char *module, const char *msg);
int data_cache_push_alarm_tcp(const char *type, const char *module, const char *msg);
void data_cache_flush(void);
int data_cache_get_count(void);
bool data_cache_is_empty(void);
bool data_cache_is_full(void);
int mqtt_is_connected(void);

#endif