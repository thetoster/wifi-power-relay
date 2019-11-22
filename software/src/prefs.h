/*
 * prefs.h
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#ifndef Prefs_hpp
#define Prefs_hpp
#include <Arduino.h>

struct SavedPrefs {
    uint8_t crc;

    //Used by net manager
    char ssid[64];
    char password[64];  //AP password
    char inNetworkName[20];
    uint8_t securityKey[32];   //used to get signed operations
};

class Prefs {
  public:
    SavedPrefs storage;

    void save();
    void defaultValues();
    bool hasPrefs();
    void load();
  private:
    uint8_t calcCRC();

    bool isZeroPrefs();
};

extern Prefs prefs;
#endif /* Prefs_hpp */
