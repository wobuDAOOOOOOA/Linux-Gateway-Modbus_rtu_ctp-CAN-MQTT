#ifndef _MODBUS_RTU_H_
#define _MODBUS_RTU_H_
#include <modbus/modbus.h>
#include <errno.h>

modbus_t* modbus_RTU_bconnect(void);
int modbus_rtu_robust_read(modbus_t **RTU_ctx,int addr, int nb, uint16_t *dest);



extern  int8_t RTU_rc;
#endif