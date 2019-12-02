/*
 * relay.h
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#ifndef SRC_RELAY_H_
#define SRC_RELAY_H_

#include "time_acumulator.h"

class VoltagePump : public TimeAcumulator {
 public:
  VoltagePump(const int pin);
  void update();
  bool isCharged();
  void discharge();

 private:
  static constexpr int PUMP_TIME_MS = 2000;
  enum class State { PUMP_LOW, PUMP_HIGH, CHARGED };

  const int pumpPin;
  State state;
};

class Relay : public TimeAcumulator {
 public:
  Relay(const int PUMP_PIN, const int SET_PIN, const int RESET_PIN);
  void turnOn() { state = State::TURN_ON; }
  void turnOff() { state = State::TURN_OFF; }
  void toggle() { state = relayTurnedOn ? State::TURN_OFF : State::TURN_ON; }
  bool isOn() { return relayTurnedOn; }
  void update();

 private:
  static constexpr int SWITCH_WAIT_TIME_MS = 40;

  enum class State { IDLE, TURN_ON, TURN_OFF, SWITCHING };

  const int resetPin;
  const int setPin;
  VoltagePump voltagePump;

  State state;
  bool relayTurnedOn;

  void changeRelay(bool turnOn);
};

extern Relay relay;

#endif /* SRC_RELAY_H_ */
