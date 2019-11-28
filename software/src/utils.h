/*
 * utils.h
 *
 *  Created on: 27 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <Arduino.h>
#include <cstddef>

String toHexString(uint8_t* data, int len);
int hexStringToArray(String hex, uint8_t* data, size_t mLen);

template <typename T>
bool equalAsStr(T& a, T& b) {
  String s1(a);
  String s2(b);
  return s1 && s2 && s1 == s2;
}

#endif /* SRC_UTILS_H_ */
