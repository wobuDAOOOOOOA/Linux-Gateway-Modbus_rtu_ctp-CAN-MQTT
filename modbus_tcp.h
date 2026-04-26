#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_
#include <modbus/modbus.h>
#include <errno.h>

modbus_t* modbus_tcp_bconnect();
void modbus_read(modbus_t *ctx);

extern  uint16_t tab_reg[64];
extern  int rc;
#endif
