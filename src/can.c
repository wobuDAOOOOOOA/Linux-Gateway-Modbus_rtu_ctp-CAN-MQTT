#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "log.h"
#include "config.h"
#include "mqtt_huawei.h"

extern gateway_config_t cfg;

#define CAN_MAX_RETRY     3
#define CAN_BASE_DELAY    5
#define CAN_ALARM_INTERVAL 30  // 告警上报间隔（秒）

static int s = -1;
static struct ifreq ifr;
static struct sockaddr_can addr;
static struct can_frame frame;
static struct can_filter filter[1];

// ====================== 重连统计结构 ======================
typedef struct {
    int total_count;
    time_t first_fail_time;
    time_t last_success_time;
    int is_in_fault;
    time_t last_alarm_time;      // 上次上报告警的时间
    int last_reported_count;     // 上次上报时的重连次数
} can_fault_stats_t;

static can_fault_stats_t fault_stats = {0};

// ====================== 内部函数 ======================
static void can_reset_stats(void)
{
    fault_stats.total_count = 0;
    fault_stats.first_fail_time = 0;
    fault_stats.last_success_time = time(NULL);
    fault_stats.is_in_fault = 0;
    fault_stats.last_alarm_time = 0;
    fault_stats.last_reported_count = 0;
}

static void can_mark_fault_start(void)
{
    if (fault_stats.is_in_fault == 0) {
        fault_stats.first_fail_time = time(NULL);
        fault_stats.is_in_fault = 1;
        fault_stats.last_alarm_time = 0;  // 复位，允许立即上报
        fault_stats.last_reported_count = 0;
        LOG_WARN("CAN:进入故障状态");
    }
    fault_stats.total_count++;
}

static void can_mark_fault_end(void)
{
    if (fault_stats.is_in_fault == 1) {
        fault_stats.last_success_time = time(NULL);
        fault_stats.is_in_fault = 0;
        LOG_INFO("CAN:故障恢复，总重连次数 %d", fault_stats.total_count);
    }
}

// ====================== 告警上报（带间隔控制） ======================
static void can_try_report_fault(void)
{
    if (!fault_stats.is_in_fault) {
        return;
    }

    time_t now = time(NULL);

    // 控制上报间隔：每30秒上报一次，或者重连次数变化时上报
    if (fault_stats.last_alarm_time > 0 &&
        (now - fault_stats.last_alarm_time) < CAN_ALARM_INTERVAL &&
        fault_stats.last_reported_count == fault_stats.total_count) {
        return;  // 未到上报间隔且次数未变，不上报
    }

    char msg[256];
    int duration = (int)(now - fault_stats.first_fail_time);

    snprintf(msg, sizeof(msg),
        "CAN故障中: 重连%d次, 持续%d秒(约%.1f分钟)",
        fault_stats.total_count,
        duration,
        duration / 60.0);

    mqtt_publish_CAN_alarm("CAN_FAULT", "CAN_Module", msg);

    fault_stats.last_alarm_time = now;
    fault_stats.last_reported_count = fault_stats.total_count;

    LOG_INFO("CAN:上报告警 - %s", msg);
}

static void can_try_report_recovered(void)
{
    if (fault_stats.total_count == 0) {
        return;
    }

    // 恢复上报只报一次（通过 last_alarm_time 控制）
    time_t now = time(NULL);
    if (fault_stats.last_alarm_time > 0 &&
        (now - fault_stats.last_alarm_time) < 60) {
        return;  // 刚报过故障，短时间内不重复上报恢复
    }

    char msg[128];
    snprintf(msg, sizeof(msg),
        "CAN已恢复, 本次故障期间重连%d次", fault_stats.total_count);
    mqtt_publish_CAN_alarm("CAN_RECOVERED", "CAN_Module", msg);
    LOG_INFO("CAN:上报恢复 - %s", msg);

    // 恢复后重置统计，避免重复上报
    can_reset_stats();
}

// ====================== CAN 初始化 ======================
int can_Init(void)
{
    LOG_INFO("CAN:初始化接口 %s", cfg.can_interface);

    can_reset_stats();

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        LOG_ERROR("CAN:创建socket失败");
        return -1;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    strcpy(ifr.ifr_name, cfg.can_interface);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        LOG_ERROR("CAN:获取接口索引失败");
        close(s);
        s = -1;
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("CAN:bind失败");
        close(s);
        s = -1;
        return -1;
    }

    int recv_own_msgs = 1;
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));
    LOG_INFO("CAN:初始化完成，自收发已开启");

    return 0;
}

// ====================== CAN 重连 ======================
static int can_reinit(void)
{
    LOG_INFO("CAN:尝试重新初始化...");

    can_mark_fault_start();

    if (s >= 0) {
        close(s);
        s = -1;
    }

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        LOG_ERROR("CAN:重连socket创建失败");
        return -1;
    }

    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    strcpy(ifr.ifr_name, cfg.can_interface);
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        LOG_ERROR("CAN:重连获取接口索引失败");
        close(s);
        s = -1;
        return -1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("CAN:重连bind失败");
        close(s);
        s = -1;
        return -1;
    }

    int recv_own_msgs = 1;
    setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));

    can_mark_fault_end();
    LOG_INFO("CAN:重连成功，自收发已开启");

    return 0;
}

// ====================== CAN 发送 ======================
int can_send(u_int16_t id, u_int16_t dlc, unsigned short *data)
{
    int retry = 0;
    struct can_frame tx_frame;
    srand((unsigned)time(NULL));

    while (retry < CAN_MAX_RETRY) {
        if (s < 0) {
            LOG_ERROR("CAN:socket无效，尝试重连");
            if (can_reinit() < 0) {
                int delay = CAN_BASE_DELAY * (1 << retry);
                int jitter = rand() % 4 - 2;
                delay += jitter;
                if (delay < 1) delay = 1;
                retry++;
                if (retry < CAN_MAX_RETRY) {
                    LOG_ERROR("CAN:重连失败，等待%ds", delay);
                    sleep(delay);
                }
                continue;
            }
        }

        tx_frame.can_id = 0x123;
        tx_frame.can_dlc = dlc;
        for (int i = 0; i < dlc; i++) {
            tx_frame.data[i] = data[i];
        }

        if (write(s, &tx_frame, sizeof(tx_frame)) == sizeof(tx_frame)) {
            LOG_INFO("CAN:发送成功 ID=0x%x, DLC=%d", id, dlc);
            return 0;
        }

        LOG_ERROR("CAN:发送失败，尝试重连");
        if (s >= 0) {
            close(s);
            s = -1;
        }

        retry++;
        if (retry < CAN_MAX_RETRY) {
            int delay = CAN_BASE_DELAY * (1 << retry);
            int jitter = rand() % 4 - 2;
            delay += jitter;
            if (delay < 1) delay = 1;
            LOG_ERROR("CAN:等待%ds后重试", delay);
            sleep(delay);
        }
    }

    LOG_ERROR("CAN:发送失败，重试耗尽");
    return -1;
}

// ====================== CAN 接收（非阻塞 + 告警上报） ======================
int can_receive(unsigned char *buffer, int *len)
{
    if (s < 0) {
        LOG_WARN("CAN:接收时socket无效");
        // 触发重连
        can_reinit();
        // 上报故障状态
        can_try_report_fault();
        return -1;
    }

    int nbytes = read(s, &frame, sizeof(frame));

    if (nbytes < 0) {
        if (errno != EAGAIN) {
            perror("can read");
            LOG_ERROR("CAN:读取错误");
            // 重连
            can_reinit();
            // 上报故障
            can_try_report_fault();
            return -1;
        }
        // 非阻塞无数据，正常返回
        return 1;
    }

    if (nbytes == sizeof(frame)) {
        // 收到数据，说明CAN正常
        // 如果之前是故障状态，上报恢复
     
            can_try_report_recovered();
        
        *len = frame.can_dlc;
        for (int i = 0; i < frame.can_dlc; i++) {
            buffer[i] = frame.data[i];
        }
        LOG_DEBUG("CAN:收到 ID=0x%x, DLC=%d", frame.can_id, frame.can_dlc);
        return 0;
    }

    return 1;
}

// ====================== CAN 清理 ======================
void can_cleanup(void)
{
    if (s >= 0) {
        close(s);
        s = -1;
        LOG_INFO("CAN:资源已清理");
    }
}