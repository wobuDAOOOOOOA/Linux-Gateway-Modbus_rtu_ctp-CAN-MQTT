#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
#include"log.h"

#define MAX_RETRY 3
#define RETRY_DELAY 5   // 秒
  uint16_t tab_reg[64];
  int rc;
modbus_t* modbus_tcp_bconnect(void) {
    modbus_t *ctx = modbus_new_tcp("192.168.2.1", 5020);
    if (ctx == NULL) return NULL;
       LOG_DEBUG("进入modbus_TCP创建连接函数");
        LOG_INFO("modbus_TCP创建连接.......");

    modbus_set_slave(ctx, 1);
    if (modbus_connect(ctx) == -1) {
        modbus_free(ctx);
        return NULL;
    }
    return ctx;
}
int modbus_robust_read(modbus_t **ctx, int addr, int nb, uint16_t *dest) {
    int rc;
    LOG_DEBUG("TCP:进入modbus_TCP重连函数");

    // ========== 1. 如果连接不存在，先建立连接 ==========
    if (*ctx == NULL) {
        LOG_ERROR("TCP:连接句柄为空，尝试创建连接\n");
        *ctx = modbus_tcp_bconnect();
        if (*ctx == NULL) {
            LOG_ERROR("TCP:连接失败\n");
            return -1;  // 立即返回失败，不持有锁去sleep
        }
        LOG_INFO("TCP:连接建立成功\n");
    }

    // ========== 2. 尝试读取数据 ==========
    rc = modbus_read_registers(*ctx, addr, nb, dest);
    if (rc == -1) {
        // 读取失败，清理连接
        LOG_ERROR("TCP:读取失败 (%s)，清理连接\n", modbus_strerror(errno));
        modbus_close(*ctx);
        modbus_free(*ctx);
        *ctx = NULL;
        return -1;  // 立即返回失败
    }

    return rc;
}
