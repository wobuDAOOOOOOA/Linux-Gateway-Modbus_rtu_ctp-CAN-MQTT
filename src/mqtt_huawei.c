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


// ====================== MQTT下行指令接收回调 ======================
void mqtt_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    if(msg == NULL || msg->payload == NULL || msg->payloadlen <= 0) return;


           // 解析 request_id
    char request_id[64] = {0};
    const char *topic = msg->topic;
    const char *prefix = "request_id=";
    char *p = strstr(topic, prefix);
    if(p != NULL) {
        p += strlen(prefix);
        char *end = strchr(p, '/');
        if(end == NULL) end = p + strlen(p);
        int len = end - p;
        if(len > 0 && len < (int)sizeof(request_id)) {
            memcpy(request_id, p, len);
        }
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
           mqtt_send_command_response(request_id, 0, NULL);

    }
    else if(strstr(payload_buf, "rtu_stop") != NULL)
    {
        
        mgr.rtu_collect_enable = 0;
        printf("【MQTT下发】关闭RTU采集成功！\n");
        printf("【MQTT下发】关闭RTU采集成功！xxxxxxxxxxxxx:%d\n",mgr.rtu_collect_enable);
                   mqtt_send_command_response(request_id, 0, NULL);

    }



       if(strstr(payload_buf, "tcp_start") != NULL)
    {
        mgr.tcp_collect_enable = 1;
        printf("【MQTT下发】开启TCP采集成功！\n");
        printf("【MQTT下发】开启TCP采集成功！xxxxxxxxxxxxx:%d\n",mgr.tcp_collect_enable);
           mqtt_send_command_response(request_id, 0, NULL);

    }
   else if(strstr(payload_buf, "tcp_stop") != NULL)
    {
        
        mgr.tcp_collect_enable = 0;
        printf("【MQTT下发】关闭TCP采集成功！\n");
        printf("【MQTT下发】关闭TCP采集成功！xxxxxxxxxxxxx:%d\n",mgr.tcp_collect_enable);
                   mqtt_send_command_response(request_id, 0, NULL);

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

/**
 * @brief 发送命令执行响应给华为云
 * @param request_id  必须和下行命令中的一致
 * @param result_code 0表示成功，非0表示失败
 * @param paras       可选，额外的响应参数，如无则传NULL
 */
void mqtt_send_command_response(const char *request_id, int result_code, const char *paras)
{
    if(!g_mosq || !request_id) return;

    char response_topic[256];
    char response_payload[128];

    // 构造响应Topic: $oc/devices/{device_id}/sys/commands/response/request_id={request_id}
    snprintf(response_topic, sizeof(response_topic), 
             "$oc/devices/%s/sys/commands/response/request_id=%s", 
             cfg.mqtt_username, request_id);

    // 构造响应Payload，最简单的成功/失败
    if(paras != NULL) {
        snprintf(response_payload, sizeof(response_payload),
                 "{\"result_code\":%d,\"paras\":%s}", result_code, paras);
    } else {
        snprintf(response_payload, sizeof(response_payload),
                 "{\"result_code\":%d}", result_code);
    }

    // 发送响应（QoS 1，确保平台能收到）
    int ret = mosquitto_publish(g_mosq, NULL, response_topic, 
                                strlen(response_payload), response_payload, 1, false);
    if(ret == MOSQ_ERR_SUCCESS) {
        printf("命令响应已发送: %s\n", response_payload);
    } else {
        fprintf(stderr, "命令响应发送失败，错误码: %d\n", ret);
    }
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