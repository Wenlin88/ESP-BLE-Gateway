#ifndef _MQTT_OVER_WIFI_H    // Put these two lines at the top of your file.
#define _MQTT_OVER_WIFI_H    // (Use a suitable name, usually based on the file name.)

int connect_to_mqtt();
void publish_sensor_mqtt_msg(String msg, String sensor_id, String meas_id);
int disconnect_from_mqtt();


#endif // _MQTT_OVER_WIFI_H    // Put this line at the end of your file.