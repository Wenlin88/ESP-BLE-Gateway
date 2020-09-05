#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include <string>
using namespace std;
#include <WiFi.h>
#include <PubSubClient.h>
#include <sstream>
#include <stdlib.h>

#include "esp_ble_sensor_config.h"

const char* wifi_ssid = WIFI_SSID; // Enter your WiFi name
const char* wifi_password =  WIFI_PASSWORD; // Enter WiFi wifi_password
const char* mqtt_server_address = MQTT_SERVER_ADDRESS;
const int   mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;

#define SCAN_TIME  5 // seconds

BLEScan *pBLEScan;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
String MAC_string;

// 16 bit
short getShort(unsigned char* data, int index)
{
  return (short)((data[index] << 8) + (data[index + 1]));
}

short getShortone(unsigned char* data, int index)
{
  return (short)((data[index]));
}

// 16 bit
unsigned short getUShort(unsigned char* data, int index)
{
  return (unsigned short)((data[index] << 8) + (data[index + 1]));
}

unsigned short getUShortone(unsigned char* data, int index)
{
  return (unsigned short)((data[index]));
}

void publish_sensor_mqtt_msg(String msg, String sensor_id, String meas_id)
{
  String publish_topic = "sensor/" + sensor_id + "/" + meas_id;
  client.publish(publish_topic.c_str(), msg.c_str());
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    MAC_string = advertisedDevice->getAddress().toString().c_str();
    if (advertisedDevice->haveServiceData()) {
      std::string strServiceData = advertisedDevice->getServiceData();
      uint8_t pData[255];
      strServiceData.copy((char *)pData, strServiceData.length(), 0);
      if (pData[0] == 0x50 & pData[1] == 0x20 & strServiceData.length() >= 16)
      {
        uint16_t rawTemperature;
        uint16_t rawHumidity;
        switch (pData[11]) {
          case 0x0d:
            rawTemperature = (pData[15] << 8) + uint16_t(pData[14]);
            rawHumidity = (pData[17] << 8) + pData[16];
            publish_sensor_mqtt_msg(String(float(rawTemperature) / 10.0),MAC_string,"temperature");
            publish_sensor_mqtt_msg(String(float(rawHumidity) / 10.0),MAC_string,"humidity");
            Serial.printf("MJ-HT-V1 temperature is %.01f, humidity %.01f\n", float(rawTemperature) / 10.0, float(rawHumidity) / 10.0);
            break;
          case 0x0a:
            publish_sensor_mqtt_msg(String(pData[14]),MAC_string,"battery");
            Serial.printf("MJ-HT-V1 battery is %u\n", pData[14]);
            break;
          case 0x04:
            rawTemperature = (pData[15] << 8) + uint16_t(pData[14]);
            publish_sensor_mqtt_msg(String(float(rawTemperature) / 10.0),MAC_string,"temperature");
            Serial.printf("MJ-HT-V1 temperature is %.01f\n", float(rawTemperature) / 10.0);
            break;
          case 0x06:
            rawHumidity = (pData[15] << 8) + uint16_t(pData[14]);
            publish_sensor_mqtt_msg(String(float(rawHumidity) / 10.0),MAC_string,"humidity");
            Serial.printf("MJ-HT-V1 humidity is %.01f\n", float(rawHumidity) / 10.0);
            break;
        }
      }
    } 
    if (advertisedDevice->haveManufacturerData()) {
      std::string strServiceData = advertisedDevice->getManufacturerData();
      uint8_t mData[255];
      strServiceData.copy((char *)mData, strServiceData.length(), 0);
      if (mData[0] == 0x99 && mData[1] == 0x04 && mData[2] == 0x05){

        short tempRaw = getShort(mData, 3);
        double temperature = (double)tempRaw * 0.005;
        unsigned short humRaw = getUShort(mData, 5);
        double humidity = (double)humRaw * 0.0025;
        unsigned int pressure = (getUShort(mData, 7) + 50000);
        short accelX = getShort(mData, 9);
        short accelY = getShort(mData, 11);
        short accelZ = getShort(mData, 13);

        unsigned short voltRaw = mData[15] << 3 | mData[16] >> 5;
        unsigned char tPowRaw = mData[16] && 0x1F;
        unsigned short voltage = voltRaw + 1600;
        char power = tPowRaw* 2 - 40;

        publish_sensor_mqtt_msg(String(temperature),MAC_string,"temperature");
        publish_sensor_mqtt_msg(String(humidity),MAC_string,"humidity");
        publish_sensor_mqtt_msg(String(pressure),MAC_string,"pressure");
        publish_sensor_mqtt_msg(String(voltage),MAC_string,"battery");
      }

      if (mData[0] == 0x99 && mData[1] == 0x04 && mData[2] == 0x03){

        short tempRaw = getUShortone(mData, 4) & 0b01111111;
        short tempRawsign = getUShortone(mData, 4) & 0b10000000;
        short tempRawdec = getUShortone(mData, 5);
        double temperature = (double)tempRaw + (double)tempRawdec / 100;
        if (tempRawsign==128){
        temperature = temperature * -1;
        }
        byte humRaw = getUShortone(mData, 3);
        short humidity = humRaw / 2;
        unsigned int pressure = (getUShort(mData, 6) + 50000);
        unsigned int voltageraw = getUShort(mData, 14);
        short voltage = (short)voltageraw;

        publish_sensor_mqtt_msg(String(temperature),MAC_string,"temperature");
        publish_sensor_mqtt_msg(String(humidity),MAC_string,"humidity");
        publish_sensor_mqtt_msg(String(pressure),MAC_string,"pressure");
        publish_sensor_mqtt_msg(String(voltage),MAC_string,"battery");
      }
    }
  }
};



void setup() {
  Serial.println("Henkan ensimmäisiä ESP bluethoot to MQTT readereita");
  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out for
  initWifi();
  client.setServer(mqtt_server_address, mqtt_port);
  initBluetooth();
}
void loop() {
  
  reconnect();
  Serial.printf("Start BLE scan for %d seconds...\n", SCAN_TIME);
  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME, false);
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(1000);
}
void initBluetooth(){
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(false); //If active scan is used MJ_HT_V1 data is not transfered correctly
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval valu
}
void initWifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_password)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("home-assistant/henkka/mood", "WooHoooo!");
      // // ... and resubscribe
      // client.subscribe(MQTT_SERIAL_RECEIVER_CH);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
bool in_array(const std::string &value, const std::vector<string> &array){
    return std::find(array.begin(), array.end(), value) != array.end();
}
