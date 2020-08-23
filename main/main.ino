#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include <string>
using namespace std;
#include <WiFi.h>
#include <PubSubClient.h>
#include <sstream>

#include "esp_ble_sensor_config.h"

const char* wifi_ssid = WIFI_SSID; // Enter your WiFi name
const char* wifi_password =  WIFI_PASSWORD; // Enter WiFi wifi_password
const char* mqtt_server_address = MQTT_SERVER_ADDRESS;
const int mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;

#define SCAN_TIME  5 // seconds

BLEScan *pBLEScan;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
long lastMsg = 0;
char msg[50];
int value = 0;


float  current_humidity = -100;
float  previous_humidity = -100;
float current_temperature = -100;
float previous_temperature = -100;

std::vector<std::string> MJ_HT_V1 {"58:2d:34:3a:37:b0", "58:2d:34:38:bd:5d"};

String convertFloatToString(float f)
{
  String s = String(f,1);
  return s;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice *advertisedDevice)
    {
      if (in_array(advertisedDevice->getAddress().toString(), MJ_HT_V1))  {
        const uint8_t * pData = (const uint8_t*)(advertisedDevice->getServiceData().data());
        if(pData[11] == 0x0d) {
        uint16_t rawTemperature = (pData[15] << 8) + uint16_t(pData[14]);
        uint16_t rawHumidity = (pData[17] << 8) + pData[16];
        Serial.printf("MJ-HT-V1 temperature is %.01f, humidity %.01f\n", float(rawTemperature) / 10.0, float(rawHumidity) / 10.0);
      } else if(pData[11] == 0x0a) {
        Serial.printf("MJ-HT-V1 battery is %u\n", pData[14]);
      } else if(pData[11] == 0x04) {
        uint16_t rawTemperature = (pData[15] << 8) + uint16_t(pData[14]);
        Serial.printf("MJ-HT-V1 temperature is %.01f\n", float(rawTemperature) / 10.0);
      } else if(pData[11] == 0x06) {
        uint16_t rawHumidity = (pData[15] << 8) + uint16_t(pData[14]);
        Serial.printf("MJ-HT-V1 humidity is %.01f\n", float(rawHumidity) / 10.0);
      } else {
        Serial.printf("MJ-HT-V1 unknown type %u\n", pData[11]);
    }
  }
    // }
    // //
    // if (advertisedDevice.haveName() && advertisedDevice.haveServiceData() && !advertisedDevice.getName().compare("MJ_HT_V1")) {
    //
    // // else if (raw_data.substring(0, 4) == "9904"){
    // //   Serial.println("Yee ruuvitag");
    // // }
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
