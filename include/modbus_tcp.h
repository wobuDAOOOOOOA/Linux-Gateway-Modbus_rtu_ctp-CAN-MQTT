#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_
#include <modbus/modbus.h>
#include <errno.h>
#include "gateway.h"

modbus_t* modbus_tcp_bconnect();
int modbus_robust_read(modbus_t **ctx, int addr, int nb, uint16_t *dest);

modbus_t* modbus_tcp_connect(const char *ip, int port, int slave_id);
int modbus_tcp_device_read(tcp_device_config_t *dev_cfg, modbus_t **ctx, 
                           int addr, int nb, uint16_t *dest);



extern  uint16_t tab_reg[64];
extern  int rc;
#endif
