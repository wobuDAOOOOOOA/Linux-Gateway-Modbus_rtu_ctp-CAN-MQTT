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
#include"relay.h"
#include"bmp280.h"
#include"data_cache.h"
#define ture 1

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
    LOG_INFO("执行网关全资源释放...");

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

    LOG_INFO("所有资源释放完成！");
}
void init_tcp_devices(void)
{
    // 1. 先把整个数组清空（防止残留垃圾数据）
    //    相当于：把 tcp_devices[0] ~ tcp_devices[3] 全部置零
    memset(mgr.tcp_devices, 0, sizeof(mgr.tcp_devices));
   
    // 2. 从配置文件 cfg 里读取数据，填充数组
    //    比如你配置了 tcp_enable[0]=1, tcp_ip[0]="192.168.1.100" ...
    int count = 0;

    for (int i = 0; i < MAX_TCP_DEVICES; i++) {
        // 如果配置里这个设备是禁用状态，就跳过
        if (cfg.tcp_enable[i] == 0) {
            continue;
        }

        // ★★★ 核心：取数组元素 ★★★
        // tcp_devices[count] 就是第 count 个设备
        // 用 & 取地址，用 -> 访问结构体成员
        tcp_device_config_t *dev = &mgr.tcp_devices[count];

        // 3. 把配置拷贝到数组元素里
        strcpy(dev->ip, cfg.tcp_ip[i]);
        dev->port = cfg.tcp_port[i];
        dev->slave_id = cfg.tcp_slave_id[i];
        dev->timeout_ms = 500;
        dev->ctx = NULL;      // 连接句柄先设为空，读函数会自动创建

        // 4. 计数加1，指向下一个数组位置
        count++;
    }

    // 5. 记录实际启用的设备数量
    mgr.tcp_device_count = count;
}

void *modbus_tcp_read_generic(void *arg)
{
int idx = *(int *)arg;
    free(arg);  // 释放传入的索引内存（可选，但推荐）

    // 记录上一次采集状态，用于去重日志
    // gateway_manager_t *mgr = &mgr;  // 或者直接用全局 mgr
  tcp_device_config_t *dev = &mgr.tcp_devices[idx];

static time_t first_fail_time_tcp = 0;  // 首次故障时间戳
    // ★★★ 添加这一行：声明 dev 变量，指向第一个 TCP 设备 ★★★

    // 线程永久常驻，仅受全局running标记控制
    int last_collect_state = mgr.tcp_collect_enable;

    while (mgr.running)
    {

        int jj = mgr.tcp_collect_enable;
        LOG_INFO("TCP采集开关状态:%d",jj);

        // ========== 云端启停控制核心逻辑（与RTU完全对称） ==========
        if (!mgr.tcp_collect_enable)
        {
            // 仅【状态从开启→关闭】瞬间执行一次释放+日志
            if (last_collect_state == 1)
            {
                // 采集关闭：主动释放TCP套接字，防止TIME_WAIT/端口泄漏
                if (mgr.tcp_ctx != NULL)
                {
                    modbus_close(dev->ctx);
                    modbus_free(dev->ctx);
                    dev->ctx = NULL;
                }
                LOG_INFO("TCP:云端指令关闭采集，已释放网络连接资源");
                dev->last_reported_status = 0;
                //mqtt_publish_TCP_alarm("TCP采集未开启", "TCP采集未开启", "TCP采集未开启");
                                // ★★★ 只记录状态，不发MQTT ★★★

                dev->status = 1;  // 采集关闭
                snprintf(dev->alarm_msg, sizeof(dev->alarm_msg), "TCP采集已关闭");
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
       // ========== 采集执行 ==========
         printf(" IP=%s, 端口=%d, 从站=%d\n",
               dev->ip, dev->port, dev->slave_id);
        if (modbus_tcp_device_read(dev, &dev->ctx, 0, 10, dev->regs) == -1)
        {
            LOG_ERROR("TCP:所有热重试失败，60s冷休眠后重试");
            // ★★★ 记录故障状态 ★★★
            dev->status = 2;  // 离线故障
            if (dev->tcp_fail_time == 0) {
               dev->tcp_fail_time = time(NULL);
                char time_str[32];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&dev->tcp_fail_time));
                snprintf(dev->alarm_msg, sizeof(dev->alarm_msg),
                         "TCP离线，首次故障: %s", time_str);
            }
            sleep(60);
            continue;
        }
   // ★★★ 读取成功：清除故障记录 ★★★
        if (dev->status== 2) {
            dev->status = 0;
            dev->tcp_fail_time = 0;
            snprintf(dev->alarm_msg, sizeof(dev->alarm_msg), "TCP已恢复");
        }
        // 打印采集数据
        for (int i = 0; i < 2; i++)
        {
            LOG_INFO("TCP_REG[%d] = %d", i, dev->regs[i]);
            
        }
      
        //TCP正常运行中
           //mqtt_publish_TCP_alarm("tcp_running", "tcp_running", "tcp采集运行中");

        // 加锁安全上报CAN总线（和RTU共用总线锁，防止总线冲突）
        pthread_mutex_lock(&mgr.bus_mutex);   
        if (can_send(0x123, 2, dev->regs) != 0)
        {
            LOG_WARN("TCP:CAN数据发送失败");
            
        }
        pthread_mutex_unlock(&mgr.bus_mutex);

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
static time_t first_fail_time_rtu = 0;  // 首次故障时间戳

    // 线程永久常驻，仅受全局running标记控制
    while (mgr->running)
    {

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
                  // ★★★ 只记录状态，不发MQTT ★★★
                   mgr->rtu_status = 1;  // 采集关闭
                snprintf(mgr->rtu_alarm_msg, sizeof(mgr->rtu_alarm_msg), "RTU采集已关闭");
            }
          //mqtt_publish_RTU_alarm("RTU采集未开启", "RTU采集未开启", "RTU采集未开启");
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
      
// 重连失败达到MAX_RETRY时：
if (modbus_rtu_robust_read(&mgr->rtu_ctx, 0, 2, mgr->rtu_data) == -1) {
                LOG_ERROR("RTU:所有热重试失败，60s冷休眠后重试\n");

            // ★★★ 记录故障状态 ★★★
            mgr->rtu_status = 2;  // 离线故障
            if (mgr->rtu_fail_time == 0) {
                mgr->rtu_fail_time = time(NULL);
                char time_str[32];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&mgr->rtu_fail_time));
                snprintf(mgr->rtu_alarm_msg, sizeof(mgr->rtu_alarm_msg),
                         "RTU离线，首次故障: %s", time_str);
            }
            sleep(60);
            continue;
        }
  // ★★★ 读取成功：清除故障记录 ★★★
        if (mgr->rtu_status == 2) {
            mgr->rtu_status = 0;
            mgr->rtu_fail_time = 0;
            snprintf(mgr->rtu_alarm_msg, sizeof(mgr->rtu_alarm_msg), "RTU已恢复");
        }
        // 打印采集数据
        for (int i = 0; i < 2; i++)
        {
            LOG_INFO("RTU_DATA[%d] = %d\n", i, mgr->rtu_data[i]);
        }

        //mqtt_publish_RTU_alarm("rtu_running", "rtu_running", "rtu采集运行中");

        pthread_mutex_lock(&mgr->bus_mutex);
        if (can_send(0x123, 2, mgr->rtu_data) != 0)
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
    int idx = *(int *)arg;
    free(arg);  // 释放传入的索引内存（可选，但推荐）
    int last_rtu_status = -1, last_tcp_status = -1;
    int last_can_fault = -1;
    int last_can_reconnect_count = -1;  // ★★★ 新增：记录上次重连次数 ★★★


    // 记录上一次采集状态，用于去重日志
    // gateway_manager_t *mgr = &mgr;  // 或者直接用全局 mgr
    while (mgr.running) {
        mgr.press = BMP280_READ();
        mgr.mqtt_connect_states = mqtt_is_connected();

        if (mgr.mqtt_connect_states) {
            data_cache_flush();
            pthread_mutex_lock(&mgr.data_mutex);
            MQTT_publish(mgr.latest_temperature, mgr.latest_humidity, mgr.press);
            pthread_mutex_unlock(&mgr.data_mutex);
        } else {
            data_cache_push_telemetry(mgr.latest_temperature, mgr.latest_humidity, mgr.press);
            printf("【缓存】MQTT离线，数据已缓存\n");
        }

        // ===== RTU 状态上报 =====
        if (mgr.rtu_status != last_rtu_status) {
            if (mgr.rtu_status == 0) {
                mqtt_publish_RTU_alarm("rtu_running", "RTU", "RTU采集运行中");
            } else if (mgr.rtu_status == 1) {
                mqtt_publish_RTU_alarm("rtu_stopped", "RTU", "RTU采集已关闭");
            } else if (mgr.rtu_status == 2) {
                mqtt_publish_RTU_alarm("rtu_offline", "RTU", mgr.rtu_alarm_msg);
            }
            last_rtu_status = mgr.rtu_status;
        }

       // ===== TCP 设备状态上报（遍历所有设备） =====
// for (int i = 0; i < mgr.tcp_device_count; i++) {
//   tcp_device_config_t *dev = &mgr.tcp_devices[idx];
//     if (mgr.tcp_status != last_tcp_status) {
//         if (mgr.tcp_status == 0) {
//             mqtt_publish_alarm("TCP", i, "running", "TCP", "TCP采集运行中");
//         } else if (mgr.tcp_status == 1) {
//             mqtt_publish_alarm("TCP", i, "stopped", "TCP", "TCP采集已关闭");
//         } else if (mgr.tcp_status == 2) {
//             mqtt_publish_alarm("TCP", i, "offline", "TCP", mgr.tcp_alarm_msg);
//         }
//         last_tcp_status = mgr.tcp_status;
//     }
// }
// ===== TCP 设备状态上报（遍历所有设备） =====
for (int i = 0; i < mgr.tcp_device_count; i++) {
    tcp_device_config_t *dev = &mgr.tcp_devices[i];
    //if (dev->status != dev->last_reported_status) {
        if (dev->status == 0) {
            mqtt_publish_alarm("TCP", i, "running", "TCP", "TCP采集运行中");
        } else if (dev->status == 1) {
            mqtt_publish_alarm("TCP", i, "stopped", "TCP", "TCP采集已关闭");
        } else if (dev->status == 2) {
            mqtt_publish_alarm("TCP", i, "offline", "TCP", dev->alarm_msg);
        }
        dev->last_reported_status = dev->status;
   // }
}
        // ===== ★★★ CAN 状态上报（修正版） ★★★ =====
        can_status_t can_status;
        can_get_status(&can_status);

        // 条件1：状态变化（正常↔故障）
        // 条件2：故障中且重连次数变化
        if (can_status.is_in_fault != last_can_fault ||
            (can_status.is_in_fault == 1 && can_status.total_reconnect_count != last_can_reconnect_count)) {
            
            if (can_status.is_in_fault == 0) {
                mqtt_publish_CAN_alarm("can_recovered", "CAN", can_status.last_alarm_msg);
            } else {
                mqtt_publish_CAN_alarm("can_fault", "CAN", can_status.last_alarm_msg);
            }
            last_can_fault = can_status.is_in_fault;
            last_can_reconnect_count = can_status.total_reconnect_count;
        }

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
    modbus_relay_init();
    BMP280_READ_Init();
    data_cache_init();
    init_tcp_devices();
    // 线程统一传全局mgr地址，全工程完全统一
    pthread_create(&mgr.threads[0], NULL, modbus_rtu_read, &mgr);
// ===== 动态创建 TCP 设备采集线程 =====

for (int i = 0; i < mgr.tcp_device_count; i++) {
    tcp_device_config_t *dev = &mgr.tcp_devices[i];
    LOG_INFO("主程序:启动TCP设备 %s (%s:%d)", dev->ip, dev->ip, dev->port);
    
    // 每个线程传入设备索引，线程内部根据索引找到对应的设备
    int *idx_ptr = malloc(sizeof(int));
    *idx_ptr = i;
    pthread_create(&mgr.threads[4 + i], NULL, modbus_tcp_read_generic, idx_ptr);
}   

 pthread_create(&mgr.threads[2], NULL, can_receive_pthread, &mgr);
    pthread_create(&mgr.threads[3], NULL, MQTT_pthread, &mgr);

    // 主线程循环
    while(1)
    {
        LOG_DEBUG("主线程运行中...");
        sleep(1);
    }
}
