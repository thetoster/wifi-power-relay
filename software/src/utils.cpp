/*
 * utils.cpp
 *
 *  Created on: 27 Nov 2019
 *      Author: bartlomiejzarnowski
 */

#include "utils.h"
#include <algorithm>

namespace {

bool handleSingleHex(const char& c, uint8_t& val) {
  if (c >= '0' && c <= '9') {
    val += c - '0';
  } else if (c >= 'A' && c <= 'F') {
    val += 10 + c - 'A';
  } else if (c >= 'a' && c <= 'f') {
    val += 10 + c - 'a';
  } else {
    return false;
  }

  return true;
}

}

String toHexString(uint8_t* data, int len) {
  String result;
  result.reserve(2 * len + 1);
  for (int t = 0; t < len; t++) {
    uint8_t b = (data[t] >> 4) & 0x0F;
    char c = (b < 0xA) ? '0' + b : 'A' + b - 0xA;
    result.concat(c);

    b = data[t] & 0x0F;
    c = (b < 0xA) ? '0' + b : 'A' + b - 0xA;
    result.concat(c);
  }

  return result;
}

int hexStringToArray(String hex, uint8_t* data, size_t mLen) {
  size_t hexLen = hex.length() / 2;
  if ((hex.length() % 2 != 0) or (hexLen > mLen)) {
    std::fill(data, data + mLen, 0);
    return 0;
  }

  int index = 0;
  for (auto it = hex.begin(); it != hex.end();) {
    uint8_t val = 0;

    bool success = handleSingleHex(*it, val);
    it++;
    val <<= 4;

    success &= handleSingleHex(*it, val);
    it++;

    if (not success) {
      std::fill(data, data + mLen, 0);
      return 0;
    }

    data[index] = val;
    index++;
  }
  return mLen;
}
