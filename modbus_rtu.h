#ifndef _MODBUS_RTU_H_
#define _MODBUS_RTU_H_
#include <modbus/modbus.h>
#include <errno.h>

int modbus_RTU_read();


extern  int16_t RTU_DATA[64];
extern  int8_t RTU_rc;
#endif