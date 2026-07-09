#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <modbus/modbus.h>
#include "gateway.h"
extern gateway_manager_t mgr;

void modbus_tcp_relay_init(void) {
int rc;
mgr.relay_ctx = modbus_new_tcp("192.168.0.10", 502);
    if (mgr.relay_ctx == NULL) return -1;
    modbus_set_slave(mgr.relay_ctx, 1);
    if (modbus_connect(mgr.relay_ctx) == -1) {
        modbus_free(mgr.relay_ctx);
        return -1;
    }
    return 0;
}
// 控制函数（只写线圈，不重复建连）
void modbus_relay_on(int channel) {
    if (mgr.relay_ctx == NULL) return;
    modbus_write_bit(mgr.relay_ctx, channel, TRUE);
}
void modbus_relay_off(int channel) {
    if (mgr.relay_ctx == NULL) return;
    modbus_write_bit(mgr.relay_ctx, channel, FALSE);
}
// 程序结束前关闭
void modbus_relay_cleanup(void) {
    if (mgr.relay_ctx) {
        modbus_close(mgr.relay_ctx);
        modbus_free(mgr.relay_ctx);
    }
}


