#ifndef RELAY_H
#define RELAY_H

void modbus_tcp_relay_init(void);
void modbus_relay_on(int channel);
void modbus_relay_off(int channel);
void modbus_relay_cleanup(void);

#endif