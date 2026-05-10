#ifndef _MQTT_HUAWEI_H_
#define _MQTT_HUAWEI_H_

int mqtt_Init();
int mqtt_publish_data(float temperature, float humidity);
void mqtt_disconnect_and_cleanup();
int MQTT_publish(float temperature, float humidity);

#endif