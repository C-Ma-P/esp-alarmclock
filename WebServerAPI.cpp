#include "WebServerAPI.h"
#include "Persistence.h"
#include "SDManager.h"
#include "Helpers.h"
#include "AudioManager.h"
#include "Config.h"
#include "Input.h"

#include <ArduinoJson.h> // for JSON handling
// HTTPUpload is declared inside WebServer.h on ESP32

// the global server instance
WebServer server(80);

// test-mode globals (also extern in header)
bool testModeEnabled = false;
String testAudioFile = "";
unsigned long testAlarmTs = 0;
bool testAlarmScheduled = false;
bool testAlarmRinging = false;

void registerWebEndpoints()
{
  // Static file endpoints
  server.on("/", HTTP_GET, []()
            { streamSdFile("/web/index.html", "text/html"); });
  server.on("/fs", HTTP_GET, handleFS);
  server.on("/upload", HTTP_POST, []()
            { server.send(200); }, handleUpload);
  server.on("/deleteEntry", HTTP_POST, handleDeleteEntry);

  // Alarms CRUD
  server.on("/api/alarms", HTTP_GET, handleGetAlarms);
  server.on("/api/alarms", HTTP_POST, handlePostAlarm);
  server.on("/api/alarms/<id>", HTTP_PATCH, handlePatchAlarm);
  server.on("/api/alarms/<id>", HTTP_DELETE, handleDeleteAlarm);

  // Settings
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_PUT, handlePutSettings);
  server.on("/api/settings/master", HTTP_PATCH, handlePatchMaster);

  // Test mode
  server.on("/api/test", HTTP_POST, handlePostTest);
  server.on("/api/test/status", HTTP_GET, handleGetTestStatus);
  server.on("/api/test/set", HTTP_POST, handleSetTestAlarm);

  // Motor API
  server.on("/api/motor", HTTP_GET, handleGetMotor);
  server.on("/api/motor", HTTP_POST, handlePostMotor);

  server.begin();
  Serial.println("[WEB] API endpoints registered");
}

// ---- Alarm Handlers ----
void handleGetAlarms()
{
  StaticJsonDocument<1024> doc;
  JsonArray arr = doc.to<JsonArray>();
  for (auto &a : alarms)
  {
    JsonObject o = arr.createNestedObject();
    o["id"] = a.id;
    o["time"] = a.time;
    o["audio"] = a.audio;
    o["snooze"] = a.snooze;
    o["blinkRate"] = a.blinkRate;
    o["enabled"] = a.enabled;
  }
  String resp;
  serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handlePostAlarm()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  String body = server.arg("plain");
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, body))
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  const char *t = doc["time"];
  const char *fn = doc["audio"];
  int snooze = doc["snooze"] | settings.defaultSnooze;
  float blinkRt = doc["blinkRate"] | settings.defaultBlinkRate;
  if (!t || !fn)
  {
    server.send(400, "text/plain", "Missing fields");
    return;
  }
  Alarm a;
  a.id = generateAlarmID();
  a.time = String(t);
  a.audio = String(fn);
  a.snooze = snooze;
  a.blinkRate = blinkRt;
  a.enabled = true;
  a.triggeredToday = false;
  a.snoozeTs = 0;
  alarms.push_back(a);
  saveAlarms();
  server.send(201, "application/json", String("{\"status\":\"ok\",\"id\":\"") + a.id + "\"}");
}

void handlePatchAlarm()
{
  String id = server.pathArg(0);
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, body))
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  bool found = false;
  for (auto &a : alarms)
  {
    if (a.id == id)
    {
      if (doc.containsKey("enabled"))
      {
        a.enabled = doc["enabled"].as<bool>();
      }
      found = true;
      break;
    }
  }
  if (!found)
  {
    server.send(404, "text/plain", "Alarm not found");
  }
  else
  {
    saveAlarms();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }
}

void handleDeleteAlarm()
{
  String id = server.pathArg(0);
  bool found = false;
  for (size_t i = 0; i < alarms.size(); ++i)
  {
    if (alarms[i].id == id)
    {
      alarms.erase(alarms.begin() + i);
      found = true;
      break;
    }
  }
  if (!found)
  {
    server.send(404, "text/plain", "Alarm not found");
  }
  else
  {
    saveAlarms();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  }
}

// ---- Settings Handlers ----
void handleGetSettings()
{
  StaticJsonDocument<256> doc;
  doc["defaultSnooze"] = settings.defaultSnooze;
  doc["defaultBlinkRate"] = settings.defaultBlinkRate;
  doc["masterEnabled"] = settings.masterEnabled;
  String resp;
  serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handlePutSettings()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, body))
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  if (doc.containsKey("defaultSnooze"))
    settings.defaultSnooze = doc["defaultSnooze"].as<int>();
  if (doc.containsKey("defaultBlinkRate"))
    settings.defaultBlinkRate = doc["defaultBlinkRate"].as<float>();
  saveSettings();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handlePatchMaster()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  String body = server.arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, body))
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  if (!doc.containsKey("masterEnabled"))
  {
    server.send(400, "text/plain", "Missing masterEnabled");
    return;
  }
  settings.masterEnabled = doc["masterEnabled"].as<bool>();
  saveSettings();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ---- File-browser Handlers ----
void handleFS()
{
  if (!server.hasArg("path"))
  {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No path\"}");
    return;
  }
  String p = server.arg("path");
  if (!p.startsWith("/"))
    p = "/" + p;
  if (!sdMounted || !sd.exists(p.c_str()))
  {
    server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Not found\"}");
    return;
  }
  SdFile dir;
  if (!dir.open(p.c_str(), O_READ))
  {
    server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Cannot open\"}");
    return;
  }
  String json = "{\"directories\":[";
  bool first = true;
  SdFile entry;
  while (entry.openNext(&dir, O_READ))
  {
    if (entry.isDir())
    {
      char name[64];
      entry.getName(name, sizeof(name));
      if (!first)
        json += ",";
      json += "\"" + String(name) + "\"";
      first = false;
    }
    entry.close();
  }
  dir.rewind();
  json += "],\"files\":[";
  first = true;
  while (entry.openNext(&dir, O_READ))
  {
    if (!entry.isDir())
    {
      char name[64];
      entry.getName(name, sizeof(name));
      if (!first)
        json += ",";
      json += "\"" + String(name) + "\"";
      first = false;
    }
    entry.close();
  }
  json += "]}";
  dir.close();
  server.send(200, "application/json", json);
}

void handleUpload()
{
  String dest = "/";
  if (server.hasArg("path"))
  {
    dest = server.arg("path");
    if (!dest.startsWith("/"))
      dest = "/" + dest;
  }
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String fn = upload.filename;
    String full = dest.endsWith("/") ? dest + fn : dest + "/" + fn;
    if (uploadFile.isOpen())
      uploadFile.close();
    if (!uploadFile.open(full.c_str(), O_CREAT | O_TRUNC | O_WRITE))
    {
      server.send(500, "text/plain", "Failed to open");
      return;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (uploadFile.isOpen())
    {
      uploadFile.write(upload.buf, upload.currentSize);
      uploadFile.sync();
    }
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile.isOpen())
    {
      uploadFile.close();
      server.send(200, "text/plain", "Upload complete");
    }
  }
}

void handleDeleteEntry()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "No body");
    return;
  }
  String b = server.arg("plain");
  int p = b.indexOf("\"path\""), i = b.indexOf("\"isDir\"");
  if (p < 0 || i < 0)
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  int c1 = b.indexOf(":", p), q1 = b.indexOf("\"", c1 + 1), q2 = b.indexOf("\"", q1 + 1);
  String path = b.substring(q1 + 1, q2);
  int c2 = b.indexOf(":", i), cb = b.indexOf(",", c2 + 1);
  if (cb < 0)
    cb = b.indexOf("}", c2 + 1);
  String isDirStr = b.substring(c2 + 1, cb);
  isDirStr.trim();
  bool isDir = (isDirStr == "true");
  if (!sdMounted || !sd.exists(path.c_str()))
  {
    server.send(404, "text/plain", "Not found");
    return;
  }
  if (isDir)
  {
    SdFile dir;
    dir.open(path.c_str(), O_READ);
    SdFile e;
    bool notEmpty = false;
    while (e.openNext(&dir, O_READ))
    {
      notEmpty = true;
      e.close();
      break;
    }
    dir.close();
    if (notEmpty)
    {
      server.send(400, "text/plain", "Directory not empty");
      return;
    }
    sd.rmdir(path.c_str());
    server.send(200, "text/plain", "Directory deleted");
  }
  else
  {
    sd.remove(path.c_str());
    server.send(200, "text/plain", "File deleted");
  }
}

// ---- Test-Mode Handlers ----
void handlePostTest()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  String b = server.arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, b))
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  bool en = doc["enabled"].as<bool>();
  const char *fn = doc["audio"];
  if (en && strlen(fn) == 0)
  {
    server.send(400, "text/plain", "Audio required");
    return;
  }
  testModeEnabled = en;
  testAudioFile = String(fn);
  testAlarmScheduled = false;
  testAlarmRinging = false;
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleGetTestStatus()
{
  long countdown = -1;
  if (testModeEnabled && testAlarmScheduled)
  {
    long msLeft = (long)testAlarmTs - (long)millis();
    if (msLeft < 0)
      msLeft = 0;
    countdown = msLeft / 1000;
  }
  String sw = isSwitchUp() ? "armed" : "disarmed";
  StaticJsonDocument<128> doc;
  doc["switchState"] = sw;
  doc["countdown"] = countdown;
  String resp;
  serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handleSetTestAlarm()
{
  if (!testModeEnabled)
  {
    server.send(400, "text/plain", "Enable Test Mode first");
    return;
  }
  testAlarmTs = millis() + 10000UL;
  testAlarmScheduled = true;
  testAlarmRinging = false;
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Test alarm set for 10 s\"}");
}

// ---- Motor API Handlers ----
void handleGetMotor()
{
  StaticJsonDocument<128> doc;
  doc["variableControl"] = motorVarControlEnabled;
  doc["maxSpeed"] = motorMaxSpeed;
  String resp;
  serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handlePostMotor()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "Missing body");
    return;
  }
  String b = server.arg("plain");
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, b))
  {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }
  if (doc.containsKey("variableControl"))
    motorVarControlEnabled = doc["variableControl"].as<bool>();
  if (doc.containsKey("maxSpeed"))
    motorMaxSpeed = doc["maxSpeed"].as<int>();
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}