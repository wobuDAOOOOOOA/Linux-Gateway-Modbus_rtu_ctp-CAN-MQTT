#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
#define MAX_RETRY 3
#define RETRY_DELAY 5   // 秒
  uint16_t tab_reg[64];
  int rc;
modbus_t* modbus_tcp_bconnect(void) {
    modbus_t *ctx = modbus_new_tcp("127.0.0.1", 5020);
    if (ctx == NULL) return NULL;

    modbus_set_slave(ctx, 1);
    if (modbus_connect(ctx) == -1) {
        modbus_free(ctx);
        return NULL;
    }
    return ctx;
}
int modbus_robust_read(modbus_t **ctx, int addr, int nb, uint16_t *dest) {
    int retry = 0;
    int rc;

    while (retry < MAX_RETRY) {
        // ========== 1. 检查连接是否存在 ==========
        if (*ctx == NULL) {
            printf("连接不存在，尝试创建连接 (第 %d 次重试)\n", retry + 1);
            *ctx = modbus_tcp_bconnect();
            if (*ctx == NULL) {
                retry++;
                if (retry < MAX_RETRY) {
                    printf("连接失败，%d 秒后重试\n", RETRY_DELAY);
                    sleep(RETRY_DELAY);
                }
                continue;   // 跳到 while 开头，不执行下面的读取
            }
            printf("连接成功\n");
        }

        // ========== 2. 尝试读取 ==========
        rc = modbus_read_registers(*ctx, addr, nb, dest);
        if (rc != -1) {
            return rc;  // 读取成功，直接返回
        }

        // ========== 3. 读取失败，清理连接 ==========
        printf("读取失败，连接可能已断开，清理并重试\n");
        modbus_close(*ctx);
        modbus_free(*ctx);
        *ctx = NULL;
        retry++;
        if (retry < MAX_RETRY) {
            printf("%d 秒后重试\n", RETRY_DELAY);
            sleep(RETRY_DELAY);
            printf("指数退避");
        }
        // 这里不需要 continue，因为后面没有代码了，自然进入下一次 while
    }

    // ========== 4. 重试次数用完 ==========
    printf("重连失败，已达最大重试次数 %d\n", MAX_RETRY);
    return -1;
}
