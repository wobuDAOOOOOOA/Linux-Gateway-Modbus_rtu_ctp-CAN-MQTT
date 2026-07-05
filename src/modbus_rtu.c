#include <stdio.h>
#include <modbus/modbus.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include"log.h"
#include"config.h"

#define RTU_MAXretry     3
#define RTU_BASE_DELAY   5    // 基础延时5s

extern gateway_config_t cfg;
modbus_t* RTU_ctx = NULL;

modbus_t* modbus_RTU_bconnect(void)
{
    LOG_DEBUG("RTU:进入modbus_RTU创建连接函数");
    LOG_INFO("RTU:modbus_RTU创建连接...");

    RTU_ctx = modbus_new_rtu(cfg.modbus_port, cfg.modbus_baudrate, 'N', 8, 1);
    if(RTU_ctx == NULL)
    {
        LOG_ERROR("RTU:无法建立上下文 %s", modbus_strerror(errno));
        return NULL;
    }
    // 设置读写超时（libmodbus官方推荐，防止卡死）
    modbus_set_response_timeout(RTU_ctx, 0, 500000); // 500ms超时

    if(modbus_connect(RTU_ctx) == -1)
    {
        LOG_ERROR("RTU:连接失败,错误：%s", modbus_strerror(errno));
        modbus_free(RTU_ctx);
        RTU_ctx = NULL;
        return NULL;
    }
    LOG_INFO("RTU:串口连接建立成功");
    return RTU_ctx;
}

int modbus_rtu_robust_read(modbus_t **ctx, int addr, int nb, uint16_t *dest)
{
    int retry = 0;
    int rc;
   

    while(retry < RTU_MAXretry)
    {
        // 1. 句柄为空，执行重连
        if (*ctx == NULL)
        {
            LOG_ERROR("RTU:句柄为空，第%d次重连", retry+1);
            *ctx = modbus_RTU_bconnect();
            if (*ctx == NULL)
            {
                // 重连失败，指数退避等待
                int delay = RTU_BASE_DELAY * (1 << retry);
                // 随机抖动 ±2s，避免多设备同步重试
                int jitter = rand() % 4 - 2;
                delay += jitter;
                if(delay < 1) delay = 1;

                retry++;
                if (retry < RTU_MAXretry)
                {
                    LOG_ERROR("RTU:重连失败，等待%ds后重试", delay);
                    sleep(delay);
                }
                continue; // 回到循环开头再次判断
            }
            // 重连成功，不增加retry，直接往下执行读取
        }

        // 2. 句柄有效，执行寄存器读取
        modbus_set_slave(*ctx, cfg.modbus_slave_id);
        rc = modbus_read_registers(*ctx, addr, nb, dest);
        if (rc != -1)
        {
            LOG_INFO("RTU:读取成功，获取%d个寄存器", rc);
            return rc;
        }

        // 3. 读取失败，销毁连接，准备重试
        LOG_ERROR("RTU:读取失败 err=%s，销毁连接准备重试", modbus_strerror(errno));
        modbus_close(*ctx);
        modbus_free(*ctx);
        *ctx = NULL;
        retry++;

        // 指数退避延时
        if(retry < RTU_MAXretry)
        {
            int delay = RTU_BASE_DELAY * (1 << retry);
            int jitter = rand() % 4 - 2;
            delay += jitter;
            if(delay < 1) delay = 1;
            LOG_ERROR("RTU:第%d次重试，等待%ds", retry, delay);
            sleep(delay);
        }
    }

    LOG_ERROR("RTU:读取重试耗尽，最大重试次数%d", RTU_MAXretry);

    return -1;
}