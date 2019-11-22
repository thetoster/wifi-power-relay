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
#include "prefs.h"
#include "relay.h"

AsyncWebServer server(80);
Relay relay(10, 11, 12);

const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

const char *PARAM_MESSAGE = "message";
const char *hostName = "blabla";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void enableOTA() {
  // Send OTA events to the browser
  //  ArduinoOTA.onStart([]() {  });
  //  ArduinoOTA.onEnd([]() {  });
  //  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //    char p[32];
  //    sprintf(p, "Progress: %u%%\n", (progress / (total / 100)));
  //  });
  //  ArduinoOTA.onError([](ota_error_t error) {
  //    if (error == OTA_AUTH_ERROR)
  //    else if (error == OTA_BEGIN_ERROR)
  //    else if (error == OTA_CONNECT_ERROR)
  //    else if (error == OTA_RECEIVE_ERROR)
  //    else if (error == OTA_END_ERROR)
  //  });
  ArduinoOTA.setHostname(hostName);
  ArduinoOTA.begin();
}

void setupRelayPages() {
  server.on("/turnOn", HTTP_POST, [&](AsyncWebServerRequest *request) {
    relay.turnOn();
    request->send(200, "text/plain", "OK");
  });

  server.on("/turnOff", HTTP_POST, [&](AsyncWebServerRequest *request) {
    relay.turnOff();
    request->send(200, "text/plain", "OK");
  });

  server.on("/toggle", HTTP_POST, [&](AsyncWebServerRequest *request) {
    relay.toggle();
    request->send(200, "text/plain", "OK");
  });

  server.on("/state", HTTP_GET, [&](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", relay.isOn() ? "ON" : "OFF");
  });
}

void setup() {
  Serial.begin(115200);

  prefs.load();

  if (prefs.hasPrefs()) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(prefs.storage.ssid, prefs.storage.password);
  }

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");

  } else {
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "commands: \n"
        "POST: /turnOn, /turnOff, /toggle, /factoryReset, /configure \n"
        "GET: /state");
  });
  setupRelayPages();

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  enableOTA();
  server.begin();
}

void loop() { ArduinoOTA.handle(); }
