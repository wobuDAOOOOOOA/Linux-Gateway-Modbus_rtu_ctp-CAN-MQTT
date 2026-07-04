#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <mosquitto.h>
#include"config.h"
extern gateway_config_t cfg;  // 告诉编译器：cfg 在别的文件里定义好了

// ==================== 请修改这里 ====================
// 华为云MQTT接入地址（在控制台总览页查看）
//#define MQTT_HOST       "5a470957e4.st1.iotda-device.cn-north-4.myhuaweicloud.com"
//#define MQTT_PORT       1883   // 改为非加密端口

// 设备信息（在控制台注册“密钥”设备后获取）
//#define MQTT_CLIENT_ID  "69fe6602cbb0cf6bb958fad5_TempHumi_0_0_2026050908"  // 替换成你的设备ID
#define MQTT_USERNAME   "69fe6602cbb0cf6bb958fad5_TempHumi"               // 替换成你的设备ID
#define MQTT_PASSWORD   "4897317957bb35698971f91ad52af3a6abedb507fcd044fa1749ff24b43de744"                                      // 替换成你的设备密钥
#define MQTT_CLIENT_ID  "69fe6602cbb0cf6bb958fad5_TempHumi_0_0_2026070207"
// 发布主题（注意：不同实例可能有差异）
//#define MQTT_PUB_TOPIC  "$oc/devices/69fe6602cbb0cf6bb958fad5_TempHumi/sys/properties/report"

// 产品模型定义
#define SERVICE_ID      "Telemetry" 
#define PROP_TEMP       "temperature"
#define PROP_HUM        "humidity"
#define PAYLOAD_TEMPLATE "{\"services\":[{\"service_id\":\"%s\",\"properties\":{\"%s\":%.1f,\"%s\":%.1f}}]}"
int max_publish = 5;
int publish_count = 0;
static struct mosquitto *g_mosq = NULL;

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

    // 连接到华为云 MQTT Broker
    ret = mosquitto_connect(g_mosq, cfg.mqtt_broker, cfg.mqtt_port, 60);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "错误: 连接失败，错误码: %d。\n", ret);
        fprintf(stderr, "请检查: 1. 主机地址和端口 2. 设备ID/密钥是否正确 3. 网络是否连通\n");
        return -1;
    }
    
    printf("成功: MQTT客户端已使用密钥方式连接华为云 IoTDA 平台。\n");
    mosquitto_loop_start(g_mosq);
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