#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_ble_sensor_config.h"

const char* wifi_ssid = WIFI_SSID; // Enter your WiFi name
const char* wifi_password =  WIFI_PASSWORD; // Enter WiFi wifi_password
const char* mqtt_server_address = MQTT_SERVER_ADDRESS;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void publish_sensor_mqtt_msg(String msg, String sensor_id, String meas_id)
{
  String publish_topic = "sensor/" + sensor_id + "/" + meas_id;
  client.publish(publish_topic.c_str(), msg.c_str());
}

int connect_to_mqtt() {
  int retry_count = 0;
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry_count += 1;
    if (retry_count >= 10)
    {
      Serial.println("");
      Serial.println("Connection failed!");
      return 0;
    }
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server_address, mqtt_port);

  retry_count = 0;  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_password)) {
      Serial.println("connected!");
      //Once connected, publish an announcement...
      // // ... and resubscribe
      // client.subscribe(MQTT_SERIAL_RECEIVER_CH);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      retry_count += 1;
      if (retry_count >= 10)
      {
        Serial.println("Connection failed!");
        return 0;
      }
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  return 1;
}
int disconnect_from_mqtt() {
    client.disconnect();
    WiFi.disconnect(true);
    return 1;
}