#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <mosquitto.h>
#include"mqtt_huawei.h"

extern gateway_config_t cfg;

// 全局MQTT句柄
struct mosquitto *g_mosq = NULL;

// 【关键修复】全局网关结构体实体定义（解决链接报错 undefined reference to mgr）
extern gateway_manager_t mgr;

// ====================== 云端启停控制函数（操作gateway结构体成员） ======================
/**
//  * @brief  云端下发【开启RTU采集】
//  * @param  mgr: 全局网关结构体指针
//  */
// void cloud_rtu_start(gateway_manager_t *mgr)
// {
//     if(mgr != NULL)
//     {
//         mgr->rtu_collect_enable = 1;
//     }
// }

// /**
//  * @brief  云端下发【关闭RTU采集】
//  * @param  mgr: 全局网关结构体指针
//  */
// void cloud_rtu_stop(gateway_manager_t *mgr)
// {
//     if(mgr != NULL)
//     {   printf("xxxxxxxxxxxxxxxxxxxxx7527524534345635463463xxxxxxxxx\nxxxx\n");

//         mgr->rtu_collect_enable = 0;
//     }
// }

// ====================== MQTT下行指令接收回调 ======================
void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    if(msg == NULL || msg->payload == NULL || msg->payloadlen <= 0)
    {
        return;
    }

    char payload_buf[512] = {0};
    int copy_len = msg->payloadlen < (int)sizeof(payload_buf)-1 ? msg->payloadlen : (int)sizeof(payload_buf)-1;
    memcpy(payload_buf, msg->payload, copy_len);

    printf("收到云端下发指令: %s\n", payload_buf);

    // 识别云端下发指令
    if(strstr(payload_buf, "rtu_start") != NULL)
    {
        mgr.rtu_collect_enable = 1;
        printf("【MQTT下发】开启RTU采集成功！\n");
                printf("【MQTT下发】开启RTU采集成功！xxxxxxxxxxxxx:%d\n",mgr.rtu_collect_enable);

    }
    else if(strstr(payload_buf, "rtu_stop") != NULL)
    {
        
        mgr.rtu_collect_enable = 0;
        printf("【MQTT下发】关闭RTU采集成功！\n");
        printf("【MQTT下发】关闭RTU采集成功！xxxxxxxxxxxxx:%d\n",mgr.rtu_collect_enable);

    }
}

// ====================== 你原有全部上报函数（完全未改动） ======================
int mqtt_publish_data(float temperature, float humidity) {
    char payload[256];
    int ret;
 
    if (!g_mosq) {
        fprintf(stderr, "错误: MQTT客户端未初始化\n");
        return -1;
    }

    snprintf(payload, sizeof(payload), PAYLOAD_TEMPLATE, SERVICE_ID, PROP_TEMP, temperature, PROP_HUM, humidity);
    ret = mosquitto_publish(g_mosq, NULL, cfg.mqtt_topic, strlen(payload), payload, 1, false);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "错误: 数据发布失败，错误码: %d\n", ret);
        return -1;
    }
    
    printf("成功: 已上报数据 - 温度: %.1f, 湿度: %.1f\n", temperature, humidity);
    return 0;
}

void mqtt_disconnect_and_cleanup() {
    if (g_mosq) {
        mosquitto_disconnect(g_mosq);
        mosquitto_destroy(g_mosq);
        g_mosq = NULL;
    }
    mosquitto_lib_cleanup();
    printf("信息: MQTT 客户端已断开并清理资源。\n");
}

int mqtt_Init()
{
    int ret;

    // 初始化 libmosquitto 库
    mosquitto_lib_init();
    
    // 创建 MQTT 客户端实例
    g_mosq = mosquitto_new(MQTT_CLIENT_ID, true, NULL);
    if (!g_mosq) {
        fprintf(stderr, "错误: 创建 MQTT 客户端失败\n");
        return -1;
    }

    // 设置用户名和密码（密钥认证的关键）
    ret = mosquitto_username_pw_set(g_mosq, cfg.mqtt_username, cfg.mqtt_password);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "错误: 设置用户名/密码失败，错误码: %d\n", ret);
        return -1;
    }

    // 注册下行消息回调函数（开启云端指令监听）
    mosquitto_message_callback_set(g_mosq, mqtt_message_callback);

    // 连接到华为云 MQTT Broker
    ret = mosquitto_connect(g_mosq, cfg.mqtt_broker, cfg.mqtt_port, 60);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "错误: 连接失败，错误码: %d。\n", ret);
        fprintf(stderr, "请检查: 1. 主机地址和端口 2. 设备ID/密钥是否正确 3. 网络是否连通\n");
        return -1;
    }

    // 订阅云端下行命令主题
    ret = mosquitto_subscribe(g_mosq, NULL, MQTT_SUB_CMD_TOPIC, 1);
    if(ret != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "错误: 订阅下行指令主题失败！\n");
        return -1;
    }
    
    printf("成功: MQTT客户端已使用密钥方式连接华为云 IoTDA 平台，开启指令监听\n");
    mosquitto_loop_start(g_mosq);

    // 默认上电开启RTU采集
    mgr.rtu_collect_enable = 1;
    mgr.running = 1;

    return 0;
}
    

int MQTT_publish(float temperature, float humidity) {
    if (!g_mosq) {
        printf("MQTT not connected, skip publish\n");
        return -1;
    }
    mqtt_publish_data(temperature, humidity);
    return 0;
}