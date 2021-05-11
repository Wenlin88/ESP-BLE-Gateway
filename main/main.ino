#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include <string>
using namespace std;
#include <sstream>
#include <stdlib.h>
#include <Arduino.h>

#include "ruuvitag.h"
#include "mqtt_over_wifi.h"
#include "atom.h"
#include "M5Atom.h"
#include "flora.h"

TaskHandle_t hibernateTaskHandle = NULL;

#define SCAN_TIME  5 // seconds
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define SLEEP_TIME  10        /* Time ESP32 will go to sleep (in seconds) */
#define EMERGENCY_HIBERNATE 3 * 60
#define RETRY 3

// boot count is used to schedule task exicutions, RTC memory is powered during sleep and RTC_DATA_ATTR values are kept over deep sleep 
RTC_DATA_ATTR int bootCount = 0;

BLEScan *pBLEScan;
// float VBAT= 0; // battery voltage from ESP32 ADC read

String MAC_string;
String device_name;

// Variables for flower care sensor management
#define MAX_FLOWER_CARE_NUMBER 25
std::vector<String> flower_care_MACs;
int flower_care_sensor_count = 0;

bool in_array(String value, const std::vector<String> &array){
    return std::find(array.begin(), array.end(), value) != array.end();
}
// const uint8_t vbatPin = 34;

class MqttPublishAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  // Todo ---> Siirrä kaikki sensorikohtaiset datan purkamiset omiin kirjastoihin (ruuvitag, MJ_HT_V1 ja flower care), Jätä tänne vain sensorien tunnistus
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    MAC_string = advertisedDevice->getAddress().toString().c_str();
    device_name = advertisedDevice->getName().c_str();

    if (advertisedDevice->haveName())
    {
      if (device_name == "Flower care")
      {
        Serial.print("Flower care @ ");
        Serial.println(MAC_string);
        if (!in_array(MAC_string, flower_care_MACs))
        {
          flower_care_MACs.push_back(MAC_string);
          flower_care_sensor_count++;
        }
        else Serial.println("Flower care already at list!");
       
        
      }
    }
    if (advertisedDevice->haveServiceData()) {
      if (device_name == "MJ_HT_V1")
      {
        Serial.print("MJ_HT_V1 @ ");
        String topic = "MJ_HT_V1/" + MAC_string;
        Serial.println(topic);
        std::string strServiceData = advertisedDevice->getServiceData();
        uint8_t pData[255];
        strServiceData.copy((char *)pData, strServiceData.length(), 0);
        if (pData[0] == 0x50 & pData[1] == 0x20 & strServiceData.length() >= 14)
        {
          uint16_t rawTemperature;
          uint16_t rawHumidity;
          unsigned char battery = pData[14];
          switch (pData[11]) {
            case 0x0d:
              rawTemperature = (pData[15] << 8) + uint16_t(pData[14]);
              rawHumidity = (pData[17] << 8) + pData[16];
              publish_sensor_mqtt_msg(String(float(rawTemperature) / 10.0),topic,"temperature");
              publish_sensor_mqtt_msg(String(float(rawHumidity) / 10.0),topic,"humidity");
              Serial.printf("MJ-HT-V1 temperature is %.01f, humidity %.01f\n", float(rawTemperature) / 10.0, float(rawHumidity) / 10.0);
              break;
            case 0x0a:
              battery = pData[14];
              publish_sensor_mqtt_msg(String(battery),topic,"battery");
              Serial.printf("MJ-HT-V1 battery is %u\n", pData[14]);
              break;
            case 0x04:
              rawTemperature = (pData[15] << 8) + uint16_t(pData[14]);
              publish_sensor_mqtt_msg(String(float(rawTemperature) / 10.0),topic,"temperature");
              Serial.printf("MJ-HT-V1 temperature is %.01f\n", float(rawTemperature) / 10.0);
              break;
            case 0x06:
              rawHumidity = (pData[15] << 8) + uint16_t(pData[14]);
              publish_sensor_mqtt_msg(String(float(rawHumidity) / 10.0),topic,"humidity");
              Serial.printf("MJ-HT-V1 humidity is %.01f\n", float(rawHumidity) / 10.0);
              break;
        }
      }
      }

    } 
    if (advertisedDevice->haveManufacturerData()) {
      std::string strServiceData = advertisedDevice->getManufacturerData();
      uint8_t mData[255];
      strServiceData.copy((char *)mData, strServiceData.length(), 0);
      if (mData[0] == 0x99 && mData[1] == 0x04)
      {
        Serial.print("Ruuvitag detected @ ");
        String topic = "ruuvitag/" + MAC_string;
        Serial.println(topic);

        if (mData[0] == 0x99 && mData[1] == 0x04 && mData[2] == 0x05){
          short tempRaw = getShort(mData, 3);
          double temperature = (double)tempRaw * 0.005;
          unsigned short humRaw = getUShort(mData, 5);
          double humidity = (double)humRaw * 0.0025;
          double pressure = (getUShort(mData, 7) + 50000)/100.0;
          short accelX = getShort(mData, 9);
          short accelY = getShort(mData, 11);
          short accelZ = getShort(mData, 13);

          unsigned short voltRaw = mData[15] << 3 | mData[16] >> 5;
          unsigned char tPowRaw = mData[16] && 0x1F;
          double voltage = (voltRaw + 1600)/1000.0;
          
          char power = tPowRaw* 2 - 40;

          publish_sensor_mqtt_msg(String(temperature),topic,"temperature");
          publish_sensor_mqtt_msg(String(humidity),topic,"humidity");
          publish_sensor_mqtt_msg(String(pressure),topic,"pressure");
          publish_sensor_mqtt_msg(String(voltage),topic,"battery");

          Serial.printf("Ruuvitag temperature is %.01f, humidity %.01f, pressure %.01f, battery %.01f", float(temperature), float(humidity), float(pressure), float(voltage));
          Serial.println("");
        }
        if (mData[0] == 0x99 && mData[1] == 0x04 && mData[2] == 0x03)
        {
          short tempRaw = getUShortone(mData, 4) & 0b01111111;
          short tempRawsign = getUShortone(mData, 4) & 0b10000000;
          short tempRawdec = getUShortone(mData, 5);
          double temperature = (double)tempRaw + (double)tempRawdec / 100;
          if (tempRawsign==128){
          temperature = temperature * -1;
          }
          byte humRaw = getUShortone(mData, 3);
          short humidity = humRaw / 2;
          double pressure = (getUShort(mData, 6) + 50000)/100.0;
          unsigned int voltageraw = getUShort(mData, 14);
          double voltage = (short)voltageraw/1000.0;

          publish_sensor_mqtt_msg(String(temperature),topic,"temperature");
          publish_sensor_mqtt_msg(String(humidity),topic,"humidity");
          publish_sensor_mqtt_msg(String(pressure),topic,"pressure");
          publish_sensor_mqtt_msg(String(voltage),topic,"battery");

          Serial.printf("Ruuvitag temperature is %.01f, humidity %.01f, pressure %.01f, battery %.01f", float(temperature), float(humidity), float(pressure), float(voltage));
          Serial.println("");
        }

      }
     }
  }
};
class ScanAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    MAC_string = advertisedDevice->getAddress().toString().c_str();
    device_name = advertisedDevice->getName().c_str();
    Serial.print(MAC_string);
    Serial.print("\t");
    Serial.print(device_name);
    Serial.print("\t");
    if (advertisedDevice->haveServiceData()) {
      std::string strServiceData = advertisedDevice->getServiceData();
      uint8_t pData[255];
      strServiceData.copy((char *)pData, strServiceData.length(), 0);
      Serial.print("has service data");
      Serial.print("\n");
    } 
    if (advertisedDevice->haveManufacturerData()) {
      std::string strServiceData = advertisedDevice->getManufacturerData();
      uint8_t mData[255];
      strServiceData.copy((char *)mData, strServiceData.length(), 0);
      Serial.print("has manufacture data");
      Serial.print("\n");
    }
  }
};


void hibernate() {
  esp_sleep_enable_timer_wakeup(SLEEP_TIME * uS_TO_S_FACTOR);
  Serial.println("Going to sleep now.");
  delay(100);
  esp_deep_sleep_start();
}

void delayedHibernate(void *parameter) {
  delay(EMERGENCY_HIBERNATE * 1000); // delay for five minutes
  Serial.println("Something got stuck, entering emergency hibernate...");
  hibernate();
}

void setup() {
  // increase boot count
  bootCount++;
  // create a hibernate task in case something gets stuck
  xTaskCreate(delayedHibernate, "hibernate", 4096, NULL, 1, &hibernateTaskHandle);

  M5.begin(true, false, true);
  delay(50);
  
  Serial.begin(115200);
  Serial.setTimeout(500);// Set time out for
  int mqtt_connected = connect_to_mqtt();
  if (mqtt_connected)
  {
    initBluetooth();
    Serial.printf("Start BLE scan for %d seconds...\n", SCAN_TIME);
    BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME, false);
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory


    bool read_flora = ((bootCount % 6*5) == 0); // Read flora data in every 5 min if temp sensors are read 10 sec interval
    if (read_flora){
      for (int i = 0; i < flower_care_MACs.size(); i++)
      {
        MAC_string = flower_care_MACs[i];
        // Length (with one extra character for the null terminator)
        int str_len = MAC_string.length() + 1; 
        // Prepare the character array (the buffer) 
        char deviceMacAddress[str_len];

        // Copy it over 
        MAC_string.toCharArray(deviceMacAddress, str_len);

        BLEAddress floraAddress(deviceMacAddress);
        bool read_battery = ((bootCount % 6*60) == 0); // Read flora battery in every 1 hour if temp sensors are read 10 sec interval
        processFloraDevice(floraAddress, deviceMacAddress, read_battery, 1);
      }

    }
      
    draw_ok();
    publish_sensor_mqtt_msg(String(bootCount), "ESP-BLE-GTW-1", "Bootcount");
    
    int mqtt_disconnected = disconnect_from_mqtt();
  }
  else
  {
    draw_cross();
  }

  // Read battery voltage and send it to MQTT
  // VBAT = (float)(analogRead(vbatPin)) * 3600 / 4095 * 2;
  // float VBATOK = VBAT / 1000;
  // publish_sensor_mqtt_msg(String(VBATOK),"ESP_listener_1","battery_voltage");
  delay(1000);
  M5.dis.clear();
  delay(500);
  Serial.println("Setup ESP32 to sleep for every " + String(SLEEP_TIME) +" Seconds");
  Serial.flush(); 
  hibernate();
}
void loop() {
  
  Serial.println("We will never get here...");
  delay(1000);
}
void initBluetooth(){
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MqttPublishAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //I enabled it, because i want to detect all sensor name from device (active scan). Seems to work ok. Lets see--> If active scan is used MJ_HT_V1 data is not transfered correctly
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(100);  // less or equal setInterval value
}



