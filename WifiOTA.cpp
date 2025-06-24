// WiFiOTA.cpp
#include "WiFiOTA.h"
#include "Config.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>
#include "WifiCreds.h"

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  uint32_t t0=millis();
  while (WiFi.status()!=WL_CONNECTED && millis()-t0<10000) delay(200);
  if (WiFi.status()!=WL_CONNECTED) { Serial.println("[WiFi] fail"); return; }
  Serial.printf("[WiFi] IP=%s\n", WiFi.localIP().toString().c_str());
  esp_wifi_set_ps(WIFI_PS_NONE);
  if (MDNS.begin("ac")) {
    MDNS.addService("http","tcp",80);
    Serial.println("[mDNS] ac.local");
  }
}

void setupOTA() {
  ArduinoOTA.setHostname("espclock");
  ArduinoOTA.begin();
}

void stopOTA() {
  ArduinoOTA.end();
}
