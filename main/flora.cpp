#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include <Arduino.h>
#include <string>
using namespace std;
#include <sstream>
#include <stdlib.h>
#include <Arduino.h>
#include "mqtt_over_wifi.h"


// the remote service we wish to connect to
static BLEUUID serviceUUID("00001204-0000-1000-8000-00805f9b34fb");

// the characteristic of the remote service we are interested in
static BLEUUID uuid_version_battery("00001a02-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_sensor_data("00001a01-0000-1000-8000-00805f9b34fb");
static BLEUUID uuid_write_mode("00001a00-0000-1000-8000-00805f9b34fb");

String baseTopic = "Testi";

float temperature;
int moisture;
int battery;
int light;
int conductivity;

BLEClient* getFloraClient(BLEAddress floraAddress) {
  BLEClient* floraClient = BLEDevice::createClient();
  
  if (!floraClient->connect(floraAddress)) {
    Serial.println("- Connection failed, skipping");
    return nullptr;
  }

  Serial.println("- Connection successful");
  return floraClient;
}

BLERemoteService* getFloraService(BLEClient* floraClient) {
  BLERemoteService* floraService = nullptr;

  try {
    floraService = floraClient->getService(serviceUUID);
  }
  catch (...) {
    // something went wrong
  }
  if (floraService == nullptr) {
    Serial.println("- Failed to find data service");
    Serial.println(serviceUUID.toString().c_str());
  }
  else {
    Serial.println("- Found data service");
  }

  return floraService;
}

bool forceFloraServiceDataMode(BLERemoteService* floraService) {
  BLERemoteCharacteristic* floraCharacteristic;

  // get device mode characteristic, needs to be changed to read data
  Serial.println("- Force device in data mode");
  floraCharacteristic = nullptr;
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_write_mode);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }

  // write the magic data
  uint8_t buf[2] = {0xA0, 0x1F};
  floraCharacteristic->writeValue(buf, 2, true);

  delay(500);
  return true;
}

bool readFloraDataCharacteristic(BLERemoteService* floraService, String baseTopic) {
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the main device data characteristic
  Serial.println("- Access characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_sensor_data);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping device");
    return false;
  }
  
  // read characteristic value
  Serial.println("- Read value from characteristic");
  std::string value;
  try {
    value = floraCharacteristic->readValue();
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping device");
    return false;
  }
  const char *val = value.c_str();

  Serial.print("Hex: ");
  for (int i = 0; i < 16; i++) {
    Serial.print((int)val[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");

  char buffer[64];
  
  int16_t* temp_raw = (int16_t*)val;
  temperature = (*temp_raw) / ((float)10.0);
  Serial.print("-- Temperature:  ");
  Serial.print(temperature);
  Serial.println("°C");


  moisture = val[7];
  Serial.print("-- Moisture:     ");
  Serial.print(moisture);
  Serial.println(" %");

  
  light = val[3] + val[4] * 256;
  Serial.print("-- Light:        ");
  Serial.print(light);
  Serial.println(" lux");

  
  conductivity = val[8] + val[9] * 256;
  Serial.print("-- Conductivity: ");
  Serial.print(conductivity);
  Serial.println(" uS/cm");

  
  return true;
}

bool readFloraBatteryCharacteristic(BLERemoteService* floraService, String baseTopic) {
  BLERemoteCharacteristic* floraCharacteristic = nullptr;

  // get the device battery characteristic
  Serial.println("- Access battery characteristic from device");
  try {
    floraCharacteristic = floraService->getCharacteristic(uuid_version_battery);
  }
  catch (...) {
    // something went wrong
  }
  if (floraCharacteristic == nullptr) {
    Serial.println("-- Failed, skipping battery level");
    return false;
  }

  // read characteristic value
  Serial.println("- Read value from characteristic");
  std::string value;
  try {
    value = floraCharacteristic->readValue();
  }
  catch (...) {
    // something went wrong
    Serial.println("-- Failed, skipping battery level");
    return false;
  }
  const char *val2 = value.c_str();
  battery = val2[0];

  char buffer[64];
  Serial.print("-- Battery:      ");
  Serial.print(battery);
  Serial.println(" %");
  return true;
}

bool processFloraService(BLERemoteService* floraService, char* deviceMacAddress, bool readBattery) {
  // set device in data mode
  if (!forceFloraServiceDataMode(floraService)) {
    return false;
  }
  String topic = String("flora/") + String(deviceMacAddress);
  Serial.println(topic);
  bool dataSuccess = readFloraDataCharacteristic(floraService, baseTopic);
  if (dataSuccess) 
  {
    if (temperature!=0 && temperature>-20 && temperature<40) {
      publish_sensor_mqtt_msg(String(temperature),topic,"temperature");
      Serial.println("Temperature sent >>");
    }
    else {
      Serial.println("   >> Skip temperature");
    }
    if (moisture<=100 && moisture>=0) {
        publish_sensor_mqtt_msg(String(moisture),topic,"moisture");
        Serial.println("Moisture sent >>");
    }
    else {
      Serial.println("   >> Skip moisture");
    }
    if (light>=0) {
        publish_sensor_mqtt_msg(String(light),topic,"light");
        Serial.println("Light sent >>");
    }
    else {
      Serial.println("   >> Skip light");
    }
    if (conductivity>=0 && conductivity<5000) { 
        publish_sensor_mqtt_msg(String(conductivity),topic,"conductivity");
        Serial.println("Conductivity sent >>");
    }
    else {
      Serial.println("   >> Skip conductivity");
    }
  }
  

  bool batterySuccess = true;
  if (readBattery) {
    batterySuccess = readFloraBatteryCharacteristic(floraService, baseTopic);
    publish_sensor_mqtt_msg(String(battery),topic,"battery");
    Serial.println("Battery sent >>");
  }

  return dataSuccess && batterySuccess;
}

bool processFloraDevice(BLEAddress floraAddress, char* deviceMacAddress, bool getBattery, int tryCount) {
  Serial.print("Processing Flora device at ");
  Serial.print(floraAddress.toString().c_str());
  Serial.print(" (try ");
  Serial.print(tryCount);
  Serial.println(")");

  // connect to flora ble server
  BLEClient* floraClient = getFloraClient(floraAddress);
  if (floraClient == nullptr) {
    return false;
  }

  // connect data service
  BLERemoteService* floraService = getFloraService(floraClient);
  if (floraService == nullptr) {
    floraClient->disconnect();
    return false;
  }

  // process devices data
  bool success = processFloraService(floraService, deviceMacAddress, getBattery);

  // disconnect from device
  floraClient->disconnect();

  return success;
}