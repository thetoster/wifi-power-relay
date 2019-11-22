/*
 * prefs.cpp
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#include <prefs.h>
#include <EEPROM.h>

Prefs prefs;

void Prefs::load() {
  EEPROM.begin(512);
  EEPROM.get(0, storage);
  EEPROM.end();
  uint8_t expectedCrc = calcCRC();

  Serial.print("StorageCrc:");
  Serial.println(storage.crc);
  Serial.print("Expected CRC:");
  Serial.println(expectedCrc);
  Serial.print("Is zero prefs:");
  Serial.println(isZeroPrefs());
  Serial.flush();

  //if (storage.crc != expectedCrc || isZeroPrefs()) {
  if (not hasPrefs()) {
    defaultValues();
  }
}

bool Prefs::hasPrefs() {
  return (storage.crc == calcCRC()) && (not isZeroPrefs()) && (prefs.storage.ssid[0] != 0);
}

bool Prefs::isZeroPrefs() {
  const uint8_t* data = (uint8_t*)&storage;
  for(size_t t = 0; t < sizeof(storage); t++) {
    if (*data != 0) {
      return false;
    }
    data++;
  }
  return true;
}

void Prefs::defaultValues() {
  Serial.println("Reset prefs to default");
  memset(&storage.ssid[0], 0, sizeof(storage.ssid));
  memset(&storage.password[0], 0, sizeof(storage.password));
  memset(&storage.inNetworkName[0], 0, sizeof(storage.inNetworkName));
  strcpy(&storage.inNetworkName[0], "Relay");
  memset(&storage.securityKey[0], 0, sizeof(storage.securityKey));
}

void Prefs::save() {
  storage.crc = calcCRC();
  EEPROM.begin(512);
  EEPROM.put(0, storage);
  EEPROM.commit();
  EEPROM.end();
}

uint8_t Prefs::calcCRC() {
  const uint8_t* data = (uint8_t*)&storage;
  data++; //skip crc field
  int len = sizeof(storage) - 1;
  uint8_t crc = 0x00;
  while (len--) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}
