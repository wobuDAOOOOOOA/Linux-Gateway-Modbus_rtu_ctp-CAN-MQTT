#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

gateway_config_t cfg;

void config_set_default(gateway_config_t *cfg_ptr)
{
    // RTU默认配置
    strcpy(cfg_ptr->modbus_port, "/dev/ttyS3");
    cfg_ptr->modbus_baudrate = 4800;
    cfg_ptr->modbus_slave_id = 1;

    // TCP默认配置
    strcpy(cfg_ptr->modbus_ip, "192.168.2.1");
    cfg_ptr->modbus_tcp_port = 5020;

    // CAN默认配置
    strcpy(cfg_ptr->can_interface, "vcan0");

    // MQTT默认配置
    strcpy(cfg_ptr->mqtt_broker, "117.78.5.125");
    cfg_ptr->mqtt_port = 1883;
    strcpy(cfg_ptr->mqtt_topic, "$oc/devices/69fe6602cbb0cf6bb958fad5_TempHumi/sys/properties/report");
    strcpy(cfg_ptr->mqtt_client_id, "69fe6602cbb0cf6bb958fad5_TempHumi_0_0_2026070207");
    strcpy(cfg_ptr->mqtt_username, "69fe6602cbb0cf6bb958fad5_TempHumi");
    strcpy(cfg_ptr->mqtt_password, "cf059d90b349820ece3aa4487f92cd846758b82f5d4434fc078d117469b0364f");

    // ★★★ 继电器默认配置（新增） ★★★
    strcpy(cfg_ptr->relay_ip, "192.168.0.10");
    cfg_ptr->relay_port = 502;
    cfg_ptr->relay_slave_id = 1;
}

int config_load(const char *filename, gateway_config_t *cfg_ptr)
{
    config_set_default(cfg_ptr);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("config file not found, use default config\n");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '#' || line[0] == '\0')
            continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;

        char *end = key + strlen(key) - 1;
        while (end >= key && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }
        end = value + strlen(value) - 1;
        while (end >= value && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }

        // RTU配置
        if (strcmp(key, "modbus_port") == 0) strcpy(cfg_ptr->modbus_port, value);
        else if (strcmp(key, "modbus_baudrate") == 0) cfg_ptr->modbus_baudrate = atoi(value);
        else if (strcmp(key, "modbus_slave_id") == 0) cfg_ptr->modbus_slave_id = atoi(value);

        // TCP配置
        else if (strcmp(key, "modbus_ip") == 0) strcpy(cfg_ptr->modbus_ip, value);
        else if (strcmp(key, "modbus_tcp_port") == 0) cfg_ptr->modbus_tcp_port = atoi(value);

        // CAN配置
        else if (strcmp(key, "can_interface") == 0) strcpy(cfg_ptr->can_interface, value);

        // MQTT配置
        else if (strcmp(key, "mqtt_broker") == 0) strcpy(cfg_ptr->mqtt_broker, value);
        else if (strcmp(key, "mqtt_port") == 0) cfg_ptr->mqtt_port = atoi(value);
        else if (strcmp(key, "mqtt_topic") == 0) strcpy(cfg_ptr->mqtt_topic, value);
        else if (strcmp(key, "mqtt_client_id") == 0) strcpy(cfg_ptr->mqtt_client_id, value);
        else if (strcmp(key, "mqtt_username") == 0) strcpy(cfg_ptr->mqtt_username, value);
        else if (strcmp(key, "mqtt_password") == 0) strcpy(cfg_ptr->mqtt_password, value);

        // ★★★ 继电器配置（新增） ★★★
        else if (strcmp(key, "relay_ip") == 0) strcpy(cfg_ptr->relay_ip, value);
        else if (strcmp(key, "relay_port") == 0) cfg_ptr->relay_port = atoi(value);
        else if (strcmp(key, "relay_slave_id") == 0) cfg_ptr->relay_slave_id = atoi(value);
    }

    fclose(fp);
    return 0;
}