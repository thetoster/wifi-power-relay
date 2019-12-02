/*
 * TimeAcumulator.h
 *
 *  Created on: 2 Dec 2019
 *      Author: bartlomiejzarnowski
 */

#ifndef SRC_TIME_ACUMULATOR_H_
#define SRC_TIME_ACUMULATOR_H_

#include <Arduino.h>

class TimeAcumulator {
public:
  bool cumulateTime(const unsigned long value) {
    const unsigned long mil = millis();

    //overflow check
    if (lastTime > mil) {
      lastTime = mil;
      return cumulatedTime >= value;
    }

    cumulatedTime += mil - lastTime;
    lastTime = mil;
    return cumulatedTime >= value;
  }

  void beginCumulate() {
    lastTime = millis();
    cumulatedTime = 0;
  }

  bool cumulated(const unsigned long value) {
    return cumulatedTime >= value;
  }

private:
  unsigned long lastTime = 0;
  unsigned long cumulatedTime = 0;
};

#endif /* SRC_TIME_ACUMULATOR_H_ */
