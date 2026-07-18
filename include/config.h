#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    // RTU配置
    char modbus_port[64];
    int modbus_baudrate;
    int modbus_slave_id;

    // TCP配置
    char modbus_ip[64];
    int  modbus_tcp_port;

    // CAN配置
    char can_interface[16];

    // MQTT配置
    char mqtt_broker[64];
    int  mqtt_port;
    char mqtt_topic[128];
    char mqtt_client_id[32];
    char mqtt_username[128];
    char mqtt_password[128];

    // ★★★ 继电器配置（新增） ★★★
    char relay_ip[64];       // 继电器IP地址
    int  relay_port;         // 继电器端口
    int  relay_slave_id;     // 继电器从站ID

} gateway_config_t;

extern gateway_config_t cfg;

int config_load(const char *filename, gateway_config_t *cfg);

#endif