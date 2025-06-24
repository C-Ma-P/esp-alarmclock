// LEDControl.cpp
#include "LEDControl.h"
#include "Config.h"

// state
bool          ledBlinkEnabled    = false;
float         ledBlinkHz         = 0.0f;
unsigned long ledLastToggle      = 0;
bool          ledPulseMode       = false;
unsigned long ledPulseOnDuration = 0;
unsigned long ledPulseOffDuration= 0;
unsigned long ledPulseLastToggle = 0;

void setLEDPulse(unsigned long onMs, unsigned long offMs) {
  digitalWrite(LED_PIN, HIGH);
  ledPulseMode       = true;
  ledBlinkEnabled    = false;
  ledPulseOnDuration = onMs;
  ledPulseOffDuration= offMs;
  ledPulseLastToggle = millis();
}
void stopLEDPulse() {
  ledPulseMode = false;
}
void setLEDBlink(float hz) {
  ledBlinkHz      = hz;
  ledBlinkEnabled = true;
  ledPulseMode    = false;
  ledLastToggle   = millis();
}
void stopLEDBlink() {
  ledBlinkEnabled = false;
}

void updateLED(unsigned long now) {
  if (ledPulseMode) {
    if (digitalRead(LED_PIN) == HIGH) {
      if (now - ledPulseLastToggle >= ledPulseOnDuration) {
        digitalWrite(LED_PIN, LOW);
        ledPulseLastToggle = now;
      }
    } else {
      if (now - ledPulseLastToggle >= ledPulseOffDuration) {
        digitalWrite(LED_PIN, HIGH);
        ledPulseLastToggle = now;
      }
    }
  }
  else if (ledBlinkEnabled) {
    unsigned long interval = (unsigned long)(500.0f/ledBlinkHz);
    if (now - ledLastToggle >= interval) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      ledLastToggle = now;
    }
  }
}
