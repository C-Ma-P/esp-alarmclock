#include "Config.h"
#include "MotorControl.h"
#include "LEDControl.h"
#include "Input.h"
#include "SDManager.h"
#include "Persistence.h"
#include "AudioManager.h"
#include "WiFiOTA.h"
#include "WebServerAPI.h"
#include "Helpers.h"
#include "WifiCreds.h"    // defines WIFI_SSID & WIFI_PASSWORD
#include <ArduinoOTA.h>
#include <time.h>

enum State { ACTIVE, CONFIG_WEB, CONFIG_OTA };
static State currentState = ACTIVE;

void setup() {
  Serial.begin(115200);
  delay(100);

  // GPIO setup
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BUTTON1_PIN,    INPUT_PULLUP);
  pinMode(BUTTON2_PIN,    INPUT_PULLUP);
  pinMode(LED_PIN,        OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ADC setup for pot
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Subsystems initialization
  initInput();
  initMotor();    // also calls initMotorPWM()
  initSD();
  loadSettings();
  loadAlarms();
  for (auto &a : alarms) {
    a.triggeredToday = false;
    a.snoozeTs       = 0;
  }
  initAudio();

  // Wi-Fi & NTP
  setupWiFi();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(2000);

  currentState = ACTIVE;
  digitalWrite(LED_PIN, LOW);
}

const int stepsPerRevolution = 1600*2;    // YOUR actual steps per full rev (e.g. 400 per quarter, so 1600 per rev)
const int stepsPerMovement   = 400;     // Steps for one quarter turn (change if you want smaller movements)
const int secondsPerMovement = 15; 

void loop() {
  unsigned long now = millis();

  if (currentState == ACTIVE && motorVarControlEnabled) {
    // ---- FAST MODE (PWM, continuous) ----
    // Make sure STEP_PIN is attached to PWM (detach any prior bit-bang)
    ledcAttachPin(STEP_PIN, 0);
    float pct = float(analogRead(POT_PIN)) / 4095.0f;
    setMotorSpeedHz(pct * motorMaxSpeed);
  }
 else if (currentState == ACTIVE) {
static unsigned long lastMove = 0;

unsigned long intervalMs = secondsPerMovement * 1000UL;

if (now - lastMove >= intervalMs) {
  lastMove = now;

  Serial.println("[CLOCK] Moving now");

  // Enable motor
  digitalWrite(EN_PIN, LOW);
  driver.microsteps(8);    // Or whatever your working microstep setting is
  delay(10);

  // Move the calculated number of steps
  for (int i = 0; i < stepsPerMovement; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(1000);
  }

  // Disable motor
  digitalWrite(EN_PIN, HIGH);
  Serial.println("[CLOCK] Move complete");
    } else {
      // Keep motor disabled when not moving
      digitalWrite(EN_PIN, HIGH);
    }
  }
  else {
    // ---- All other modes: Motor off ----
    ledcWrite(0, 0);
    ledcDetachPin(STEP_PIN);
    pinMode(STEP_PIN, OUTPUT);
    digitalWrite(STEP_PIN, LOW);
    digitalWrite(EN_PIN, HIGH);
  }


  // ── WEB / OTA ───────────────────────────────────────────────────────
  if (currentState == CONFIG_WEB) {
    server.handleClient();
  } else if (currentState == CONFIG_OTA) {
    ArduinoOTA.handle();
  }
  // ── TEST MODE ─────────────────────────────────────────────────────
  if (testModeEnabled) {
    if (testAlarmScheduled && !testAlarmRinging) {
      long msLeft = (long)testAlarmTs - (long)now;
      if (msLeft < 0) msLeft = 0;
      if (msLeft <= 5000 && msLeft > 0) {
        float ramp = (5000.0f - float(msLeft)) / 1000.0f;
        float rate = settings.defaultBlinkRate * (1.0f + ramp);
        setLEDBlink(rate);
      } else {
        stopLEDBlink();
        digitalWrite(LED_PIN, LOW);
      }
      if (now >= testAlarmTs) {
        testAlarmScheduled = false;
        testAlarmRinging   = true;
        Serial.println("[TEST] Ringing now");
        setLEDBlink(settings.defaultBlinkRate > 0 ? settings.defaultBlinkRate : 1.0f);
        startAudioPlayback(testAudioFile.c_str());
      }
    }
    if (testAlarmRinging) {
      handleAudioInLoop();
      if (buttonA_ShortPressed()) {
        Serial.println("[TEST] Snooze pressed");
        stopAudioPlayback();
        testAlarmRinging   = false;
        testAlarmTs        = now + (unsigned long)settings.defaultSnooze * 60000UL;
        testAlarmScheduled = true;
        stopLEDBlink();
        digitalWrite(LED_PIN, LOW);
      }
      if (isSwitchDown()) {
        Serial.println("[TEST] Disarmed - stopping alarm");
        stopAudioPlayback();
        testAlarmRinging = false;
        stopLEDBlink();
        digitalWrite(LED_PIN, LOW);
      }
    }
  }

  // ── CONFIG MODE TRANSITIONS ────────────────────────────────────────
  if (!testModeEnabled) {
    if (buttonA_LongPressed() && currentState == ACTIVE && sdMounted) {
      currentState = CONFIG_WEB;
      registerWebEndpoints();
      setLEDPulse(200, 200);
    }
    if (buttonB_ShortPressed()) {
      if (currentState == CONFIG_WEB) {
        currentState = CONFIG_OTA;
        server.stop();
        setupOTA();
        setLEDPulse(100, 100);
      } else if (currentState == CONFIG_OTA) {
        stopOTA();
        currentState = CONFIG_WEB;
        registerWebEndpoints();
        setLEDPulse(200, 200);
      }
    }
    if (buttonAB_Held2s()) {
      if (currentState == CONFIG_WEB)    server.stop();
      if (currentState == CONFIG_OTA)    stopOTA();
      currentState = ACTIVE;
      stopLEDBlink();
      stopLEDPulse();
      digitalWrite(LED_PIN, LOW);
    }
  }

  // ── ACTIVE ALARM CLOCK ─────────────────────────────────────────────
  if (currentState == ACTIVE) {
    dailyResetTriggeredFlags();
    bool inRamp = false;

    time_t tnow = time(nullptr);
    struct tm tmnow;
    localtime_r(&tnow, &tmnow);
    long secNow = tmnow.tm_hour * 3600L + tmnow.tm_min * 60L + tmnow.tm_sec;

    for (auto &a : alarms) {
      if (!a.enabled) continue;

      // Snooze branch
      if (a.snoozeTs > 0) {
        long msLeft = (long)a.snoozeTs - (long)now;
        if (msLeft <= 0 && !a.triggeredToday) {
          a.triggeredToday = true;
          Serial.println("[ALARM] Snooze → Ringing");
          setLEDBlink(a.blinkRate > 0 ? a.blinkRate : 1.0f);
          startAudioPlayback(a.audio.c_str());
        } else if (msLeft > 0 && msLeft <= 5000) {
          inRamp = true;
          float ramp = (5000.0f - float(msLeft)) / 1000.0f;
          float rate = a.blinkRate * (1.0f + ramp);
          setLEDBlink(rate);
        }
        continue;
      }

      // Exact-time branch
      int ah = atoi(a.time.substring(0,2).c_str());
      int am = atoi(a.time.substring(3,5).c_str());
      long alarmSec = ah * 3600L + am * 60L;
      long delta    = alarmSec - secNow;

      if (!a.triggeredToday) {
        if (delta == 0 && tmnow.tm_sec == 0) {
          a.triggeredToday = true;
          Serial.printf("[ALARM] %s → Ringing now\n", a.id.c_str());
          setLEDBlink(a.blinkRate > 0 ? a.blinkRate : 1.0f);
          startAudioPlayback(a.audio.c_str());
        } else if (delta > 0 && delta <= 5) {
          inRamp = true;
          float ramp = (5.0f - float(delta)) / 1.0f;
          float rate = a.blinkRate * (1.0f + ramp);
          setLEDBlink(rate);
        }
      }
    }

    if (!inRamp && !testAlarmRinging && !playingAudio) {
      digitalWrite(LED_PIN, LOW);
    }

    // Handle alarms ringing
    for (auto &a : alarms) {
      if (a.triggeredToday && a.snoozeTs == 0 && playingAudio && a.audio != "") {
        handleAudioInLoop();
        if (buttonA_ShortPressed()) {
          Serial.printf("[ALARM] %s → Snooze pressed\n", a.id.c_str());
          stopAudioPlayback();
          a.snoozeTs = now + (unsigned long)a.snooze * 60000UL;
          stopLEDBlink();
          digitalWrite(LED_PIN, LOW);
        }
        if (isSwitchDown()) {
          Serial.printf("[ALARM] %s → Disarmed\n", a.id.c_str());
          stopAudioPlayback();
          a.triggeredToday = true;
          stopLEDBlink();
          digitalWrite(LED_PIN, LOW);
        }
      }
    }
  }
}
