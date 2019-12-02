/*
 * relay.cpp
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#include "relay.h"
#include <Arduino.h>

Relay::Relay(const int aPumpPin, const int aSetPin, const int aResetPin)
    : resetPin(aResetPin),
      setPin(aSetPin),
      voltagePump(aPumpPin),
      state(State::IDLE),
      relayTurnedOn(false) {
  pinMode(setPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
}

void Relay::update() {
  voltagePump.update();

  switch (state) {
    case State::TURN_ON:
      if (voltagePump.isCharged()) {
        Serial.println("Relay: Turn on");
        digitalWrite(setPin, 1);
        digitalWrite(resetPin, 0);
        relayTurnedOn = true;
        state = State::SWITCHING;
        beginCumulate();
      }
      break;

    case State::TURN_OFF:
      if (voltagePump.isCharged()) {
        Serial.println("Relay: Turn off");
        digitalWrite(setPin, 0);
        digitalWrite(resetPin, 1);
        relayTurnedOn = false;
        state = State::SWITCHING;
        beginCumulate();
      }
      break;

    case State::SWITCHING:
      if (cumulateTime(SWITCH_WAIT_TIME_MS)) {
        Serial.println("Relay: Switch done");
        state = State::IDLE;
        digitalWrite(resetPin, 0);
        digitalWrite(setPin, 0);
        voltagePump.discharge();
      }
      break;

    case State::IDLE:
    default:
      break;
  }
}

VoltagePump::VoltagePump(const int pin) : pumpPin(pin), state(State::PUMP_LOW) {
  pinMode(pumpPin, OUTPUT);
}

void VoltagePump::update() {
  switch (state) {
    case State::PUMP_LOW:
      if (cumulateTime(PUMP_TIME_MS)) {
        Serial.println("VoltagePump: Done lower pump");
        digitalWrite(pumpPin, 1);
        state = State::PUMP_HIGH;
        beginCumulate();
      }
      break;

    case State::PUMP_HIGH:
      if (cumulateTime(PUMP_TIME_MS)) {
        Serial.println("VoltagePump: Done upper pump, charged");
        state = State::CHARGED;
      }
      break;

    case State::CHARGED:
      // Do nothing
      break;
  }
}

void VoltagePump::discharge() {
  Serial.println("VoltagePump: Start charge");
  state = State::PUMP_LOW;
  digitalWrite(pumpPin, 0);
  beginCumulate();
}

bool VoltagePump::isCharged() { return state == State::CHARGED; }
