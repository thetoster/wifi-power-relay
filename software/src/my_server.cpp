/*
 BSD 3-Clause License

 Copyright (c) 2017, The Tosters
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of the copyright holder nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 WifiServer.cpp
 Created on: 27 Nov 2019
 Author: Bartłomiej Żarnowski (Toster)
 */
#include "my_server.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266TrueRandom.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266mDNS.h>
#include <sha256.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "prefs.h"
#include "relay.h"
#include "utils.h"

const String versionString = "1.0.0";

static const char rootHtml[] PROGMEM =
#include "www/index.html"
    ;

AsyncWebServer httpServer(80);
MyServer myServer;
static const char* www_realm = "Authentication Failed";

namespace {

bool checkAuth(AsyncWebServerRequest& request) {
  if (not request.authenticate(prefs.storage.username, prefs.storage.userPassword, www_realm,
                               false)) {
    Serial.println("requestAuthentication");
    request.requestAuthentication(www_realm, true);
    return false;
  };
  Serial.println("CheckAuth: OK");
  return true;
}

bool checkExtAuth(AsyncWebServerRequest& request) {
  if ((not request.hasHeader("nonce")) or (not request.hasHeader("HMac"))) {
    return checkAuth(request);
  };
  String nonce = request.header("nonce");
  Sha256.initHmac(prefs.storage.securityKey, sizeof(prefs.storage.securityKey));
  Sha256.print(nonce);
  for (size_t t = 0; t < request.args(); t++) {
    Sha256.print(request.argName(t));
    Sha256.print(request.arg(t));
  }
  Sha256.print(nonce);

  String hmac;
  hmac.reserve(64);
  uint8_t* hash = Sha256.resultHmac();
  for (int t = 0; t < 32; t++) {
    hmac.concat((char)(65 + (hash[t] >> 4)));
    hmac.concat((char)(65 + (hash[t] & 0xF)));
  }
  Serial.print("nonce:");
  Serial.println(nonce);
  Serial.print("   my hmac:");
  Serial.println(hmac);
  Serial.print("other hmac:");
  Serial.println(request.header("HMac"));
  return hmac.compareTo(request.header("HMac")) == 0;
}

void handleRelay(AsyncWebServerRequest& request, bool turnOn) {
  if (checkAuth(request) == false) {
    return;
  }
  if (turnOn) {
    relay.turnOn();

  } else {
    relay.turnOff();
  }
  request.send(200, "text/plain", "200: OK");
}

void handleFactoryConfig(AsyncWebServerRequest* request) {
  if (checkAuth(*request) == false) {
    return;
  }
  prefs.defaultValues();
  prefs.save();
  request->send(200, "text/plain", "200: OK");
  myServer.switchToConfigMode();
}

void handleVersion(AsyncWebServerRequest* request) {
  if (checkAuth(*request) == false) {
    return;
  }
  StaticJsonDocument<50> root;
  root["version"] = versionString;
  String response;
  serializeJson(root, response);
  request->send(200, "application/json", response);
}

String getStringArg(AsyncWebServerRequest& request, String argName, int maxLen, bool* isError) {
  String result = "";
  *isError = false;
  if (request.hasArg(argName.c_str())) {
    result = request.arg(argName);
    if (result.length() >= (unsigned int)maxLen) {
      String resp = "406: Not Acceptable, '" + argName + "' to long.";
      request.send(406, "text/plain", resp);
      *isError = true;
    }
  }
  return result;
}

int getIntArg(AsyncWebServerRequest& request, String argName, int maxValue, bool* isError) {
  int result = -1;
  *isError = false;
  if (request.hasArg(argName.c_str())) {
    result = request.arg(argName).toInt();
    if (result >= maxValue) {
      String resp = "406: Not Acceptable, '" + argName + "' to big.";
      request.send(406, "text/plain", resp);
      *isError = true;
    }
  }
  return result;
}

bool emplaceChars(AsyncWebServerRequest& request, char* ptr, String argName, int maxLen) {
  bool fail;
  String tmp = getStringArg(request, argName, maxLen, &fail);
  if (not fail) {
    strncpy(ptr, tmp.c_str(), maxLen);
  }
  return fail;
}

bool emplaceHex(AsyncWebServerRequest& request, uint8_t* ptr, String argName, int maxLen) {
  bool fail;
  String tmp = getStringArg(request, argName, maxLen * 2 + 2, &fail);
  if (not fail) {
    hexStringToArray(tmp, ptr, maxLen);
  }
  return fail;
}

bool handleNetworkConfig(AsyncWebServerRequest& request, SavedPrefs& p) {
  bool fail = false;

  fail |= emplaceChars(request, p.ssid, "ssid", sizeof(p.ssid));
  fail |= emplaceChars(request, p.password, "wifiPassword", sizeof(p.password));
  fail |= emplaceChars(request, p.inNetworkName, "inNetworkName", sizeof(p.inNetworkName));
  fail |= emplaceChars(request, p.username, "username", sizeof(p.username));
  fail |= emplaceChars(request, p.userPassword, "userPassword", sizeof(p.userPassword));
  fail |= emplaceHex(request, p.securityKey, "securityKey", sizeof(p.securityKey));

  return fail;
}

template <typename T>
bool isMemZero(T& ptr) {
  return std::count(std::begin(ptr), std::end(ptr), 0) ==
         std::distance(std::begin(ptr), std::end(ptr));
}

void applyNetConfig(SavedPrefs& p, bool& changed, bool& doNetRestart) {
  if (not equalAsStr(prefs.storage.inNetworkName, p.inNetworkName)) {
    std::copy(std::begin(p.inNetworkName), std::end(p.inNetworkName),
              std::begin(prefs.storage.inNetworkName));
    doNetRestart = true;
    changed = true;
  }

  if (not equalAsStr(prefs.storage.password, p.password)) {
    std::copy(std::begin(p.password), std::end(p.password), std::begin(prefs.storage.password));
    doNetRestart = true;
    changed = true;
  }

  if (not equalAsStr(prefs.storage.ssid, p.ssid)) {
    std::copy(std::begin(p.ssid), std::end(p.ssid), std::begin(prefs.storage.ssid));
    doNetRestart = true;
    changed = true;
  }

  if (not equalAsStr(prefs.storage.username, p.username)) {
    std::copy(std::begin(p.username), std::end(p.username), std::begin(prefs.storage.username));
    changed = true;
  }

  if (not equalAsStr(prefs.storage.userPassword, p.userPassword)) {
    std::copy(std::begin(p.userPassword), std::end(p.userPassword),
              std::begin(prefs.storage.userPassword));
    changed = true;
  }

  if ((not isMemZero(p.securityKey)) and
      (not std::equal(std::begin(p.securityKey), std::end(p.securityKey),
                      std::begin(prefs.storage.securityKey)))) {
    std::copy(std::begin(p.securityKey), std::end(p.securityKey),
              std::begin(prefs.storage.securityKey));
    changed = true;
  }
}

template <typename T>
void applyIfChanged(T& from, T& to, bool& changed) {
  if (from != to) {
    to = from;
    changed = true;
  }
}

bool applyPrefsChange(SavedPrefs& p, bool& restartNetwork) {
  bool changed = false;
  applyNetConfig(p, changed, restartNetwork);

  return changed | restartNetwork;
}

void handleSetConfig(AsyncWebServerRequest* request) {
  if (checkAuth(*request) == false) {
    return;
  }

  SavedPrefs p = {0};

  if (handleNetworkConfig(*request, p)) {
    return;
  }

  // now apply new values
  bool restartNetwork = false;
  bool changed = applyPrefsChange(p, restartNetwork);

  String result = "200: OK";
  if (changed) {
    prefs.save();
    result += ", Config Saved";
  }

  delay(200);
  if (restartNetwork) {
    result += ", Network restarted";
  }
  request->send(200, "text/plain", result);
  if (restartNetwork) {
    myServer.restart();
  }
}

void handleRoot(AsyncWebServerRequest* request) {
  if (checkAuth(*request) == false) {
    return;
  }
  // put config inside
  String html = FPSTR(rootHtml);

  // network
  Serial.print("SSID:");
  Serial.println(prefs.storage.ssid);
  html.replace("${ssid}", prefs.storage.ssid);
  html.replace("${inNetworkName}", prefs.storage.inNetworkName);
  html.replace("${username}", prefs.storage.username);
  html.replace("${secureKey}",
               toHexString(prefs.storage.securityKey, sizeof(prefs.storage.securityKey)));

  request->send(200, "text/html", html);
}

void handleUpdate(AsyncWebServerRequest* request) {
  if (checkAuth(*request) == false) {
    return;
  }
  bool fail = false;
  String url = getStringArg(*request, "url", 1024, &fail);
  if (fail == false && url.length() > 0) {
    request->send(200, "text/plain", "200: OK");
    ESPhttpUpdate.rebootOnUpdate(true);
    WiFiClient client;
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, url);
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.print("HTTP_UPDATE_FAILED");
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.print("HTTP_UPDATE_NO_UPDATES");
        break;

      default:
      case HTTP_UPDATE_OK:
        Serial.print("HTTP_UPDATE_OK");
        break;
    }

  } else {
    request->send(400, "text/plain", "400: BAD REQUEST");
  }
  delay(100);
  request->client()->stop();
}

}  // namespace

MyServer::MyServer() : needsConfig(true) {
  httpServer.on("/", handleRoot);
  httpServer.on("/config", HTTP_POST, handleSetConfig);
  httpServer.on("/update", HTTP_POST, handleUpdate);
  httpServer.on("/version", HTTP_GET, handleVersion);
  httpServer.on("/turnOn", HTTP_POST,
                [&](AsyncWebServerRequest* request) { handleRelay(*request, true); });

  httpServer.on("/turnOff", HTTP_POST,
                [&](AsyncWebServerRequest* request) { handleRelay(*request, false); });

  httpServer.on("/toggle", HTTP_POST,
                [&](AsyncWebServerRequest* request) { handleRelay(*request, not relay.isOn()); });
  httpServer.on("/state", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", relay.isOn() ? "ON" : "OFF");
  });
  httpServer.onNotFound(
      [](AsyncWebServerRequest* request) { request->send(404, "text/plain", "Not found"); });
}

void MyServer::switchToConfigMode() {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false);
  WiFi.enableAP(false);
  WiFi.enableSTA(false);
  delay(500);
  memset(prefs.storage.ssid, 0, sizeof(prefs.storage.ssid));
  generatePasswords();
  needsConfig = true;
  enableSoftAP();
}

void MyServer::connectToAccessPoint() {
  WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& arg) {
    Serial.print("New IP:");
    Serial.println(arg.ip.toString());
  });
  WiFi.softAPdisconnect(false);
  WiFi.begin(prefs.storage.ssid, prefs.storage.password);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
}

void MyServer::generatePasswords() {
  std::fill(std::begin(prefs.storage.password), std::end(prefs.storage.password), 0);
  std::fill(std::begin(prefs.storage.userPassword), std::end(prefs.storage.userPassword), 0);
  std::string("Karamba!").copy(prefs.storage.userPassword, sizeof(prefs.storage.userPassword));

  std::generate(std::begin(prefs.storage.securityKey), std::end(prefs.storage.securityKey),
                []() { return static_cast<uint8_t>(ESP8266TrueRandom.random(256)); });
}

String MyServer::getServerIp() {
  return needsConfig ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

bool MyServer::isServerConfigured() { return needsConfig == false; }

String MyServer::getPassword() {
  return prefs.storage.userPassword[0] == 0 ? "<-->" : String(prefs.storage.userPassword);
}

void MyServer::enableSoftAP() { WiFi.softAP(prefs.storage.inNetworkName, prefs.storage.password); }

void MyServer::restart() {
  httpServer.end();
  WiFi.softAPdisconnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.setAutoConnect(false);
  WiFi.disconnect(false);
  WiFi.enableAP(false);
  WiFi.enableSTA(false);

  needsConfig = not prefs.hasPrefs();
  if (needsConfig) {
    generatePasswords();
    enableSoftAP();
    Serial.println("AP Mode");
    Serial.print("User:");
    Serial.println(prefs.storage.username);
    Serial.print("Pass:");
    Serial.println(prefs.storage.userPassword);

  } else {
    connectToAccessPoint();
  }

  httpServer.begin();
  MDNS.notifyAPChange();
  MDNS.begin(prefs.storage.inNetworkName);
  MDNS.addService("http", "tcp", 80);
}

void MyServer::update() {
  MDNS.update();
}
