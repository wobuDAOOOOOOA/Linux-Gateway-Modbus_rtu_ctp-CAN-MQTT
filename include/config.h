#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char modbus_port[64];
    int modbus_baudrate;
    int modbus_slave_id;
    char can_interface[16];
    char mqtt_broker[64];
    int mqtt_port;
    char mqtt_topic[128];
    char mqtt_client_id[32];
    char mqtt_username[128];     // 新增 用户名
    char mqtt_password[128];     // 新增 密钥
        //========= Modbus TCP 网络配置（新增） =========
    char modbus_ip[64];
    int  modbus_tcp_port;
} gateway_config_t;
extern gateway_config_t cfg;
// 加载配置文件，返回0成功，-1失败
int config_load(const char *filename, gateway_config_t *cfg);

#endif