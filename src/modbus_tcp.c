#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include<string.h>
#include"log.h"
#include"config.h"
#include "gateway.h"

// 与RTU完全统一参数，双协议架构一致
#define TCP_MAX_RETRY     3
#define TCP_BASE_DELAY   5

extern gateway_config_t cfg;
extern gateway_manager_t mgr;

modbus_t* modbus_tcp_bconnect(void)
{
    LOG_DEBUG("TCP:进入modbus_TCP创建连接函数");
    LOG_INFO("TCP:开始建立TCP连接 %s:%d", cfg.modbus_ip, cfg.modbus_tcp_port);

    // 完全读取cfg配置，无任何硬编码
    modbus_t *ctx = modbus_new_tcp(cfg.modbus_ip, cfg.modbus_tcp_port);
    if (ctx == NULL)
    {
        LOG_ERROR("TCP:创建上下文失败 %s", modbus_strerror(errno));
        return NULL;
    }

    // 设置TCP超时，防止卡死
    modbus_set_response_timeout(ctx, 0, 500000);

    modbus_set_slave(ctx, cfg.modbus_slave_id);
    if (modbus_connect(ctx) == -1)
    {
        LOG_ERROR("TCP:连接失败 %s", modbus_strerror(errno));
        modbus_free(ctx);
        return NULL;
    }

    LOG_INFO("TCP:连接建立成功");
    return ctx;
}


/**
 * @brief 工业级TCP读写重试核心（指数退避+随机抖动）
 * @param ctx 双层指针，失败自动置空释放
 * @return 成功返回寄存器数量，失败-1
 */
int modbus_robust_read(modbus_t **ctx, int addr, int nb, uint16_t *dest)
{
    int retry = 0;
    int rc;
    srand((unsigned)time(NULL));

    while (retry < TCP_MAX_RETRY)
    {
        // 1. 句柄为空 → 触发重连
        if (*ctx == NULL)
        {
            LOG_ERROR("TCP:句柄为空，第%d次重连", retry + 1);
            *ctx = modbus_tcp_bconnect();
            if (*ctx == NULL)
            {
                // 指数退避 + 随机抖动，防止多设备同步刷屏重连
                int delay = TCP_BASE_DELAY * (1 << retry);
                int jitter = rand() % 4 - 2;
                delay += jitter;
                if (delay < 1) delay = 1;

                retry++;
                if (retry < TCP_MAX_RETRY)
                {
                    LOG_ERROR("TCP:重连失败，等待%ds后重试", delay);
                    sleep(delay);
                }
                continue;
            }
        }

        // 2. 句柄正常，执行读取
        rc = modbus_read_registers(*ctx, addr, nb, dest);
        if (rc != -1)
        {
            LOG_INFO("TCP:读取成功，获取%d个寄存器", rc);
            return rc;
        }

        // 3. 读取失败，彻底销毁连接防泄漏
        LOG_ERROR("TCP:读取异常 %s，销毁TCP连接", modbus_strerror(errno));
        modbus_close(*ctx);
        modbus_free(*ctx);
        *ctx = NULL;

        retry++;
        if (retry < TCP_MAX_RETRY)
        {
            int delay = TCP_BASE_DELAY * (1 << retry);
            int jitter = rand() % 4 - 2;
            delay += jitter;
            if (delay < 1) delay = 1;
            LOG_ERROR("TCP:第%d次重试，等待%ds", retry, delay);
            sleep(delay);
        }
    }

    LOG_ERROR("TCP:所有重试耗尽，进入冷休眠状态");
    return -1;
}

