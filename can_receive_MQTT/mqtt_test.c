// mqtt_publish.c
// 编译：gcc -o mqtt_publish mqtt_publish.c -lmosquitto
// 运行：./mqtt_publish

#include <stdio.h>
#include <string.h>
#include <mosquitto.h>

// 1. 回调函数：当与 Broker 的连接成功时被调用
void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        printf("✅ 连接成功\n");
    } else {
        printf("❌ 连接失败，错误码: %d\n", rc);
    }
}

// 2. 回调函数：当消息成功发布后被调用
void on_publish(struct mosquitto *mosq, void *obj, int mid) {
    printf("📤 消息已发送，mid=%d\n", mid);
}

int main() {
    struct mosquitto *mosq = NULL;

    // 初始化 libmosquitto 库
    mosquitto_lib_init();

    // 创建 MQTT 客户端实例，参数：
    //    NULL    - 不使用 SSL
    //    "test_client" - 客户端 ID（需要唯一，不唯一时 Broker 会踢掉旧的）
    //    NULL    - 不使用用户名/密码（此处）
    mosq = mosquitto_new("test_client", true, NULL);
    if (!mosq) {
        printf("❌ 创建客户端失败\n");
        return 1;
    }

    // 绑定回调函数
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_publish_callback_set(mosq, on_publish);

    // 连接到公共 MQTT Broker（EMQX 官方提供，免费使用）
    // 参数：host, port, keepalive_sec
    mosquitto_connect(mosq, "broker.emqx.io", 1883, 60);

    // 启动网络循环（非阻塞模式，配合下面的循环使用）
    mosquitto_loop_start(mosq);

    // 每隔 5 秒发布一条消息
    int count = 0;
    while (1) {
        char payload[64];
        snprintf(payload, sizeof(payload), "Hello from gateway, count=%d", count++);

        // 发布消息到主题 "gateway/test"
        // 参数：
        //    NULL     - 不指定消息 ID（自动分配）
        //    "gateway/test" - 主题（Topic）
        //    2        - payload 的字节长度
        //    payload  - 消息内容
        //    0        - QoS（此处用 QoS 0，最多一次）
        //    false    - 不保留消息（retain = false）
        mosquitto_publish(mosq, NULL, "gateway/test", strlen(payload), payload, 0, false);
        printf("📝 发布: %s\n", payload);
        sleep(5);
    }

    // 清理资源（实际不会执行到这里，因为 while 无限循环）
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}