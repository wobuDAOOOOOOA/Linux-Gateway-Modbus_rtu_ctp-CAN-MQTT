#ifndef _CAN_H_
#define _CAN_H_
int can_Init(void);
int my_can_send(u_int16_t id, u_int16_t dlc, short unsigned int* data_M);
int can_receive(unsigned char *buffer , int *len);


#endif
