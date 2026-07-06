#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include"modbus_tcp.h"
#include"modbus_rtu.h"
#include"can.h"
#include"mqtt_huawei.h"
#include"log.h"
#include <linux/can.h>
#include <sys/socket.h>
#include"config.h"
#include"gateway.h"

#define ture 1
int jj;

  // 全局唯一实体定义（全工程仅此一处）
    gateway_manager_t mgr;
// ====================== 资源初始化函数 ======================
static void gateway_manager_init(gateway_manager_t *mgr)
{
    memset(mgr, 0, sizeof(gateway_manager_t));

    // 初始化锁
    pthread_mutex_init(&mgr->data_mutex, NULL);
    pthread_mutex_init(&mgr->bus_mutex, NULL);
       // 开启运行开关
    mgr->running = ture;
    mgr->rtu_collect_enable=1;
    mgr->tcp_collect_enable=1;
    // 初始化默认寄存器数据
    mgr->regs[0] = 1;
    mgr->regs[1] = 2;
}

// ====================== 工业级终极清理函数 ======================
void gateway_cleanup(void)
{
    LOG_INFO("【工业清理】执行网关全资源释放...");

    // 直接操作全局mgr，去掉多余临时指针
    if (mgr.tcp_ctx) {
        modbus_close(mgr.tcp_ctx);
        modbus_free(mgr.tcp_ctx);
        mgr.tcp_ctx = NULL;
    }

    // 关闭RTU串口连接
    if (mgr.rtu_ctx) {
        modbus_close(mgr.rtu_ctx);
        modbus_free(mgr.rtu_ctx);
        mgr.rtu_ctx = NULL;
    }

    // 销毁锁资源
    pthread_mutex_destroy(&mgr.data_mutex);
    pthread_mutex_destroy(&mgr.bus_mutex);

    LOG_INFO("【工业清理】所有资源释放完成，零泄漏！");
}

void *modbus_tcp_read(void *arg)
{
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    // 记录上一次采集状态，用于去重日志
    int last_collect_state = mgr->tcp_collect_enable;

    // 线程永久常驻，仅受全局running标记控制
    while (mgr->running)
    {
        int jj = mgr->tcp_collect_enable;
        LOG_INFO("TCP采集开关状态:%d",jj);
        // ========== 云端启停控制核心逻辑（与RTU完全对称） ==========
        if (!mgr->tcp_collect_enable)
        {
            // 仅【状态从开启→关闭】瞬间执行一次释放+日志
            if (last_collect_state == 1)
            {
                // 采集关闭：主动释放TCP套接字，防止TIME_WAIT/端口泄漏
                if (mgr->tcp_ctx != NULL)
                {
                    modbus_close(mgr->tcp_ctx);
                    modbus_free(mgr->tcp_ctx);
                    mgr->tcp_ctx = NULL;
                }
                LOG_INFO("TCP:云端指令关闭采集，已释放网络连接资源");
                last_collect_state = 0;
            }

            // 低功耗休眠，避免空转耗CPU
            sleep(2);
            continue;
        }

        // 仅【状态从关闭→开启】打印启动日志
        if (last_collect_state == 0)
        {
            LOG_INFO("TCP:云端指令开启采集，恢复正常采集任务");
            last_collect_state = 1;
        }

        // ========== 采集开启：正常执行工业级重试+采集逻辑 ==========
        if (modbus_robust_read(&mgr->tcp_ctx, 0, 10, mgr->regs) == -1)
        {
            LOG_ERROR("TCP:所有热重试失败，60s冷休眠后重试");
            sleep(60);
            continue;
        }

        // 打印采集数据
        for (int i = 0; i < 2; i++)
        {
            LOG_INFO("TCP_REG[%d] = %d", i, mgr->regs[i]);
        }

        // 加锁安全上报CAN总线（和RTU共用总线锁，防止总线冲突）
        pthread_mutex_lock(&mgr->bus_mutex);
        if (my_can_send(0x123, 2, mgr->regs) != 0)
        {
            LOG_WARN("TCP:CAN数据发送失败");
        }
        pthread_mutex_unlock(&mgr->bus_mutex);

        // 正常1s采集周期
        sleep(1);
    }

    LOG_INFO("TCP采集线程全局退出");
    return NULL;
}


void *modbus_rtu_read(void *arg)
{
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    // 记录上一次采集状态，用于去重日志
    int last_collect_state = mgr->rtu_collect_enable;

    // 线程永久常驻，仅受全局running标记控制
    while (mgr->running)
    {
       jj = mgr->rtu_collect_enable;
       LOG_INFO("lsdisdnfilansf:%d",jj);
        // ========== 云端启停控制核心逻辑（去重日志优化） ==========
        if (!mgr->rtu_collect_enable)
        {
            // 仅【状态从开启→关闭】瞬间执行一次释放+日志
            if (last_collect_state == 1)
            {
                // 采集关闭：主动释放串口资源，防止句柄泄漏
                if (mgr->rtu_ctx != NULL)
                {
                    modbus_close(mgr->rtu_ctx);
                    modbus_free(mgr->rtu_ctx);
                    mgr->rtu_ctx = NULL;
                }
                LOG_INFO("RTU:云端指令关闭采集，已释放串口资源\n");
                last_collect_state = 0;
            }

            // 低功耗休眠，避免空转耗CPU
            sleep(2);
            continue;
        }

        // 仅【状态从关闭→开启】打印启动日志
        if (last_collect_state == 0)
        {
            LOG_INFO("RTU:云端指令开启采集，恢复正常采集任务\n");
            last_collect_state = 1;
        }

        // ========== 采集开启：正常执行原有重试+采集逻辑 ==========
        if (modbus_rtu_robust_read(&mgr->rtu_ctx, 0, 2, mgr->rtu_data) == -1)
        {
            LOG_ERROR("RTU:所有热重试失败，60s冷休眠后重试\n");
            sleep(60);
            continue;
        }

        // 打印采集数据
        for (int i = 0; i < 2; i++)
        {
            LOG_INFO("RTU_DATA[%d] = %d\n", i, mgr->rtu_data[i]);
        }

        // 加锁安全上报CAN总线
        pthread_mutex_lock(&mgr->bus_mutex);
        if (my_can_send(0x123, 2, mgr->rtu_data) != 0)
        {
            LOG_WARN("RTU:CAN数据发送失败\n");
        }
        pthread_mutex_unlock(&mgr->bus_mutex);

        // 正常1s采集周期
        sleep(1);
    }

    LOG_INFO("RTU采集线程全局退出\n");
    return NULL;
}

void *can_receive_pthread(void *arg) {
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    unsigned char data[8];
    int len;

    while (mgr->running) {
        if (can_receive(data, &len) == 0) {
            pthread_mutex_lock(&mgr->data_mutex);
            if (len >= 2) {
                mgr->latest_humidity = data[0];
                mgr->latest_temperature = data[1];
            }
            pthread_mutex_unlock(&mgr->data_mutex);
        }
        sleep(1);
    }
    return NULL;
}

void *MQTT_pthread(void *arg) {
    gateway_manager_t *mgr = (gateway_manager_t *)arg;
    while (mgr->running) {
        pthread_mutex_lock(&mgr->bus_mutex);
        MQTT_publish(mgr->latest_temperature, mgr->latest_humidity);
        pthread_mutex_unlock(&mgr->bus_mutex);
        sleep(1);
    }
    return NULL;
}

// ====================== 主函数 ======================
int main(void) {
  

    // rtu初始化随机种子
    srand((unsigned)time(NULL));
    // 注册全局终极清理
    atexit(gateway_cleanup);

    // 初始化全局mgr资源
    gateway_manager_init(&mgr);

    // 加载配置
    config_set_default(&cfg);
    config_load("./gateway.conf", &cfg);
    printf("Modbus port: %s\n", cfg.modbus_port);
    printf("can port: %s\n", cfg.can_interface);

    // 底层外设初始化
    mqtt_Init();
    can_Init();

    // 线程统一传全局mgr地址，全工程完全统一
    pthread_create(&mgr.threads[0], NULL, modbus_rtu_read, &mgr);
    pthread_create(&mgr.threads[1], NULL, modbus_tcp_read, &mgr);
    pthread_create(&mgr.threads[2], NULL, can_receive_pthread, &mgr);
    pthread_create(&mgr.threads[3], NULL, MQTT_pthread, &mgr);

    // 主线程循环
    while(1)
    {
        LOG_DEBUG("主线程运行中...");
        sleep(1);
    }
}
