/*
 * relay.h
 *
 *  Created on: 6 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#ifndef SRC_RELAY_H_
#define SRC_RELAY_H_

class Relay {
 public:
  Relay(const int PUMP_PIN, const int SET_PIN, const int RESET_PIN);
  void turnOn() { changeRelay(true); }
  void turnOff() { changeRelay(false); }
  void toggle() { changeRelay(!relayTurnedOn); }
  bool isOn() { return relayTurnedOn; }

 private:
  static constexpr int SWITCH_WAIT_TIME_MS = 20;

  const int pumpPin;
  const int setPin;
  const int resetPin;

  bool relayTurnedOn;

  void changeRelay(bool turnOn);
};

extern Relay relay;

#endif /* SRC_RELAY_H_ */
