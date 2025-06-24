// Persistence.h
#pragma once
#include <Arduino.h>
#include <vector>
#include <SdFat.h>
#include <ArduinoJson.h>

struct Alarm {
  String id, time, audio;
  int snooze;
  float blinkRate;
  bool enabled, triggeredToday;
  unsigned long snoozeTs;
};
struct Settings {
  int defaultSnooze;
  float defaultBlinkRate;
  bool masterEnabled;
};

extern std::vector<Alarm> alarms;
extern Settings settings;
extern const char* ALARMS_JSON_PATH;
extern const char* SETTINGS_JSON_PATH;

void loadSettings();
void saveSettings();
void loadAlarms();
void saveAlarms();
String generateAlarmID();
