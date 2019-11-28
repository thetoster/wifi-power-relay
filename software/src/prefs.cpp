/*
 * prefs.cpp
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#include <prefs.h>
#include <EEPROM.h>
#include <algorithm>
#include <iterator>

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

  if (not hasPrefs()) {
    defaultValues();
  }
}

bool Prefs::hasPrefs() {
  return (storage.crc == calcCRC()) && (not isZeroPrefs()) && (prefs.storage.ssid[0] != 0);
}

bool Prefs::isZeroPrefs() {
  const uint8_t* data = reinterpret_cast<uint8_t*>(&storage);
  const uint8_t* dataEnd = data + sizeof(storage);
  return std::count(data, dataEnd, 0) == sizeof(storage);
}

void Prefs::defaultValues() {
  Serial.println("Reset prefs to default");
  storage = {};
  std::string("Relay").copy(storage.inNetworkName, sizeof(storage.inNetworkName));
  std::string("Lampster").copy(storage.username, sizeof(storage.username));
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
