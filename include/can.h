#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include <time.h>

// ====================== CAN 状态定义 ======================
typedef struct {
    int is_in_fault;              // 0=正常, 1=故障中
    int total_reconnect_count;    // 本次故障累计重连次数
    time_t first_fail_time;       // 首次故障时间
    char last_alarm_msg[256];     // 最新的告警消息字符串
} can_status_t;

int can_Init(void);
int can_send(uint16_t id, uint16_t dlc, unsigned short *data);
int can_receive(unsigned char *buffer, int *len);
void can_cleanup(void);

// ★★★ 获取当前CAN状态（供MQTT线程调用） ★★★
void can_get_status(can_status_t *status);

#endif