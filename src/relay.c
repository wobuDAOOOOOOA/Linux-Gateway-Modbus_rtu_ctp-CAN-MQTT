#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
#include <unistd.h>
#include "log.h"
#include "config.h"

extern gateway_config_t cfg;

static modbus_t *relay_ctx = NULL;

#define RELAY_MAX_RETRY  3
#define RELAY_BASE_DELAY 5

/**
 * @brief 继电器连接
 */
static modbus_t* relay_connect(void)
{
    LOG_INFO("RELAY:连接 %s:%d, 从站ID=%d",
             cfg.relay_ip, cfg.relay_port, cfg.relay_slave_id);

    modbus_t *ctx = modbus_new_tcp(cfg.relay_ip, cfg.relay_port);
    if (ctx == NULL) {
        LOG_ERROR("RELAY:创建上下文失败 %s", modbus_strerror(errno));
        return NULL;
    }

    modbus_set_response_timeout(ctx, 1, 0);
    modbus_set_slave(ctx, cfg.relay_slave_id);

    if (modbus_connect(ctx) == -1) {
        LOG_ERROR("RELAY:连接失败 %s", modbus_strerror(errno));
        modbus_free(ctx);
        return NULL;
    }

    LOG_INFO("RELAY:连接成功");
    return ctx;
}

/**
 * @brief 继电器初始化（在程序启动时调用一次）
 */
int modbus_relay_init(void)
{
    relay_ctx = relay_connect();
    if (relay_ctx == NULL) {
        LOG_ERROR("RELAY:初始化失败");
        return -1;
    }
    return 0;
}

/**
 * @brief 继电器控制（带普通重连）
 */
static int relay_control(int channel, int state)
{
    int retry = 0;

    if (relay_ctx == NULL) {
        LOG_WARN("RELAY:句柄为空，尝试重连");
        relay_ctx = relay_connect();
        if (relay_ctx == NULL) {
            LOG_ERROR("RELAY:重连失败");
            return -1;
        }
    }

    while (retry < RELAY_MAX_RETRY) {
        if (modbus_write_bit(relay_ctx, channel, state) == 1) {
            LOG_DEBUG("RELAY:通道%d %s成功", channel, state ? "ON" : "OFF");
            return 0;
        }

        LOG_ERROR("RELAY:控制失败，第%d次重试", retry + 1);
        modbus_close(relay_ctx);
        modbus_free(relay_ctx);
        relay_ctx = NULL;

        retry++;
        if (retry < RELAY_MAX_RETRY) {
            int delay = RELAY_BASE_DELAY * (1 << retry);
            LOG_ERROR("RELAY:等待%ds后重连", delay);
            sleep(delay);
            relay_ctx = relay_connect();
            if (relay_ctx == NULL) {
                continue;
            }
        }
    }

    LOG_ERROR("RELAY:控制失败，重试耗尽");
    return -1;
}

/**
 * @brief 继电器闭合
 */
void modbus_relay_on(int channel)
{
    relay_control(channel, TRUE);
}

/**
 * @brief 继电器断开
 */
void modbus_relay_off(int channel)
{
    relay_control(channel, FALSE);
}

/**
 * @brief 继电器清理
 */
void modbus_relay_cleanup(void)
{
    if (relay_ctx) {
        modbus_close(relay_ctx);
        modbus_free(relay_ctx);
        relay_ctx = NULL;
        LOG_INFO("RELAY:资源已清理");
    }
}