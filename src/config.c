#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

// 全局配置变量定义（整个程序只有这一个真身）
gateway_config_t cfg;

void config_set_default(gateway_config_t *cfg_ptr) {
    strcpy(cfg_ptr->modbus_port, "/dev/ttyS3");
    cfg_ptr->modbus_baudrate = 4800;
    cfg_ptr->modbus_slave_id = 1;
    strcpy(cfg_ptr->can_interface, "vcan0");
    strcpy(cfg_ptr->mqtt_broker, "117.78.5.125");
    cfg_ptr->mqtt_port =1883;
    strcpy(cfg_ptr->mqtt_topic, "$oc/devices/69fe6602cbb0cf6bb958fad5_TempHumi/sys/properties/report");
    strcpy(cfg_ptr->mqtt_client_id, "69fe6602cbb0cf6bb958fad5_TempHumi_0_0_2026070207");
    strcpy(cfg_ptr->mqtt_username, "69fe6602cbb0cf6bb958fad5_TempHumi");
    strcpy(cfg_ptr->mqtt_password, "cf059d90b349820ece3aa4487f92cd846758b82f5d4434fc078d117469b0364f");
}

int config_load(const char *filename, gateway_config_t *cfg_ptr) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("config file not found, using defaults\n");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        // 去掉末尾换行和回车
        line[strcspn(line, "\r\n")] = '\0';

        // 跳过空行和注释
        if (line[0] == '#' || line[0] == '\0') continue;

        // 找到 '=' 的位置
        char *eq = strchr(line, '=');
        if (!eq) continue;  // 没等号，跳过

        // 把 key 和 value 分开
        *eq = '\0';  // 把 '=' 变成 '\0'，把一行切成两段
        char *key = line;
        char *value = eq + 1;

        // 去掉 key 和 value 前后的空格
        while (*key == ' ' || *key == '\t') key++;
        while (*value == ' ' || *value == '\t') value++;
        char *end = key + strlen(key) - 1;
        while (end >= key && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }
        end = value + strlen(value) - 1;
        while (end >= value && (*end == ' ' || *end == '\t')) { *end = '\0'; end--; }

        // 赋值
        if (strcmp(key, "modbus_port") == 0) strcpy(cfg_ptr->modbus_port, value);
        else if (strcmp(key, "modbus_baudrate") == 0) cfg_ptr->modbus_baudrate = atoi(value);
        else if (strcmp(key, "modbus_slave_id") == 0) cfg_ptr->modbus_slave_id = atoi(value);
        else if (strcmp(key, "can_interface") == 0) strcpy(cfg_ptr->can_interface, value);
        else if (strcmp(key, "mqtt_broker") == 0) strcpy(cfg_ptr->mqtt_broker, value);
        else if (strcmp(key, "mqtt_port") == 0) cfg_ptr->mqtt_port = atoi(value);
        else if (strcmp(key, "mqtt_topic") == 0) strcpy(cfg_ptr->mqtt_topic, value);
        else if (strcmp(key, "mqtt_client_id") == 0) strcpy(cfg_ptr->mqtt_client_id, value);
        else if (strcmp(key, "mqtt_username") == 0) strcpy(cfg_ptr->mqtt_topic, value);
        else if (strcmp(key, "mqtt_password") == 0) strcpy(cfg_ptr->mqtt_client_id, value);
    }

    fclose(fp);
    return 0;
}