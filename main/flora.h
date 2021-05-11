#ifndef _FLORA_H    // Put these two lines at the top of your file.
#define _FLORA_H    // (Use a suitable name, usually based on the file name.)

bool forceFloraServiceDataMode(BLERemoteService* floraService);
bool readFloraDataCharacteristic(BLERemoteService* floraService, String baseTopic);
bool readFloraBatteryCharacteristic(BLERemoteService* floraService, String baseTopic);
bool processFloraService(BLERemoteService* floraService, char* deviceMacAddress, bool readBattery);
bool processFloraDevice(BLEAddress floraAddress, char* deviceMacAddress, bool getBattery, int tryCount);

#endif // _FLORA_H    // Put this line at the end of your file.