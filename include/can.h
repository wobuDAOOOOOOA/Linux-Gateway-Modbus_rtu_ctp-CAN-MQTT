#ifndef CAN_H
#define CAN_H

#include <stdint.h>

int  can_Init(void);
int  can_send(uint16_t id, uint16_t dlc, unsigned short *data);
int  can_receive(unsigned char *buffer, int *len);
void can_cleanup(void);
// 新增
void can_get_fault_stats(int *total_count, int *duration_seconds);
int  can_is_in_fault(void);
#endif