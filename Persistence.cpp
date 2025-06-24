// Persistence.cpp
#include "Persistence.h"
#include "SDManager.h"
#include <memory> 

const char* ALARMS_JSON_PATH   = "/alarms.json";
const char* SETTINGS_JSON_PATH = "/settings.json";

std::vector<Alarm> alarms;
Settings settings;

void loadSettings() {
  if (!sdMounted || !sd.exists(SETTINGS_JSON_PATH)) {
    settings = {5, 1.0f, true};
    SdFile f;
    if (f.open(SETTINGS_JSON_PATH, O_CREAT|O_WRITE|O_TRUNC)) {
      StaticJsonDocument<256> doc;
      doc["defaultSnooze"]    = settings.defaultSnooze;
      doc["defaultBlinkRate"] = settings.defaultBlinkRate;
      doc["masterEnabled"]    = settings.masterEnabled;
      serializeJson(doc, f);
      f.close();
    }
    return;
  }
  SdFile f;
  if (f.open(SETTINGS_JSON_PATH, O_READ)) {
    size_t sz = f.fileSize();
    std::unique_ptr<char[]> buf(new char[sz+1]);
    f.read(buf.get(), sz);
    buf[sz]='\0'; f.close();
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, buf.get())) {
      settings.defaultSnooze    = doc["defaultSnooze"]|5;
      settings.defaultBlinkRate = doc["defaultBlinkRate"]|1.0f;
      settings.masterEnabled    = doc["masterEnabled"]|true;
    } else {
      settings = {5,1.0f,true};
    }
  }
}

void saveSettings() {
  if (!sdMounted) return;
  SdFile f;
  if (f.open(SETTINGS_JSON_PATH, O_CREAT|O_WRITE|O_TRUNC)) {
    StaticJsonDocument<256> doc;
    doc["defaultSnooze"]    = settings.defaultSnooze;
    doc["defaultBlinkRate"] = settings.defaultBlinkRate;
    doc["masterEnabled"]    = settings.masterEnabled;
    serializeJson(doc, f);
    f.close();
  }
}

void loadAlarms() {
  alarms.clear();
  if (!sdMounted || !sd.exists(ALARMS_JSON_PATH)) {
    SdFile f;
    if (f.open(ALARMS_JSON_PATH, O_CREAT|O_WRITE|O_TRUNC)) {
      StaticJsonDocument<512> doc;
      doc.to<JsonArray>();
      serializeJson(doc, f);
      f.close();
    }
    return;
  }
  SdFile f;
  if (f.open(ALARMS_JSON_PATH, O_READ)) {
    size_t sz = f.fileSize();
    std::unique_ptr<char[]> buf(new char[sz+1]);
    f.read(buf.get(), sz);
    buf[sz]='\0'; f.close();
    StaticJsonDocument<1024> doc;
    if (!deserializeJson(doc, buf.get())) {
      JsonArray arr = doc.as<JsonArray>();
      for (auto obj : arr) {
        Alarm a;
        a.id            = obj["id"].as<const char*>();
        a.time          = obj["time"].as<const char*>();
        a.audio         = obj["audio"].as<const char*>();
        a.snooze        = obj["snooze"].as<int>();
        a.blinkRate     = obj["blinkRate"].as<float>();
        a.enabled       = obj["enabled"].as<bool>();
        a.triggeredToday= false;
        a.snoozeTs      = 0;
        alarms.push_back(a);
      }
    }
  }
}

void saveAlarms() {
  if (!sdMounted) return;
  SdFile f;
  if (f.open(ALARMS_JSON_PATH, O_CREAT|O_WRITE|O_TRUNC)) {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (auto &a : alarms) {
      JsonObject o = arr.createNestedObject();
      o["id"]        = a.id;
      o["time"]      = a.time;
      o["audio"]     = a.audio;
      o["snooze"]    = a.snooze;
      o["blinkRate"] = a.blinkRate;
      o["enabled"]   = a.enabled;
    }
    serializeJson(doc, f);
    f.close();
  }
}

String generateAlarmID() {
  static uint32_t ctr=0;
  ++ctr;
  char buf[32];
  snprintf(buf,sizeof(buf), "%lu-%u", millis(), ctr);
  return String(buf);
}
