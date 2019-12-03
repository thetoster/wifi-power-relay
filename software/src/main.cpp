//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//
#include <Arduino.h>
#include <ArduinoOTA.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "my_server.h"
#include "prefs.h"
#include "relay.h"

Relay relay(D5, D6, D7);

void notFound(AsyncWebServerRequest *request) { request->send(404, "text/plain", "Not found"); }

void setup() {
  Serial.begin(115200);

  prefs.load();

  if (prefs.hasPrefs()) {
    Serial.print("SSID:");
    Serial.println(prefs.storage.ssid);
    Serial.print("In Net Name:");
    Serial.println(prefs.storage.inNetworkName);
    Serial.print("Password:");
    Serial.println(prefs.storage.password);

  } else {
    Serial.println("No prefs!");
  }
  relay.turnOff();
  myServer.restart();
}

void loop() {
  relay.update();
  myServer.update();
  ArduinoOTA.handle();
  delay(250);
}
