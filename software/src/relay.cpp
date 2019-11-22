/*
 * relay.cpp
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#include <Arduino.h>
#include "relay.h"

Relay::Relay(const int aPumpPin, const int aSetPin, const int aResetPin)
    : pumpPin(aPumpPin), setPin(aSetPin), resetPin(aResetPin) {
  pinMode(setPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  turnOff();
}

void Relay::changeRelay(bool turnOn) {
  digitalWrite(turnOn ? setPin : resetPin, 0);
  digitalWrite(pumpPin, 1);
  delay(SWITCH_WAIT_TIME_MS);
  digitalWrite(pumpPin, 0);
  digitalWrite(setPin, 1);
  digitalWrite(resetPin, 1);
  relayTurnedOn = turnOn;
}
