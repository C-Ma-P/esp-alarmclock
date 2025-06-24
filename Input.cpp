// Input.cpp
#include "Input.h"
#include "Config.h"

// module-local state
static bool          _prevSwitchDown;
static unsigned long _switchDownTs = 0;

void initInput() {
  _prevSwitchDown = (digitalRead(MODE_SWITCH_PIN)==LOW);
  _switchDownTs   = 0;
}

bool isSwitchUp()   { return digitalRead(MODE_SWITCH_PIN)==HIGH; }
bool isSwitchDown() { return digitalRead(MODE_SWITCH_PIN)==LOW; }

bool switchToggled() {
  bool down = isSwitchDown();
  bool toggled = (down != _prevSwitchDown);
  _prevSwitchDown = down;
  return toggled;
}

unsigned long getSwitchHoldDuration() {
  if (isSwitchDown()) {
    if (_switchDownTs==0) _switchDownTs=millis();
    return millis()-_switchDownTs;
  } else {
    _switchDownTs=0;
    return 0;
  }
}

bool buttonA_LongPressed() {
  static unsigned long start=0;
  static bool fired=false;
  bool p = (digitalRead(BUTTON1_PIN)==LOW);
  if (p) {
    if (start==0) start=millis();
    else if (!fired && millis()-start>=3000) { fired=true; return true; }
  } else {
    start=0; fired=false;
  }
  return false;
}

bool buttonA_ShortPressed() {
  static bool was=false;
  bool p = (digitalRead(BUTTON1_PIN)==LOW);
  if (p && !was) { was=true; delay(50); return true; }
  else if (!p) was=false;
  return false;
}

bool buttonB_ShortPressed() {
  static bool was=false;
  bool p = (digitalRead(BUTTON2_PIN)==LOW);
  if (p && !was) { was=true; delay(50); return true; }
  else if (!p) was=false;
  return false;
}

bool buttonAB_Held2s() {
  static unsigned long start=0;
  static bool fired=false;
  bool a = (digitalRead(BUTTON1_PIN)==LOW);
  bool b = (digitalRead(BUTTON2_PIN)==LOW);
  if (a && b) {
    if (start==0) start=millis();
    else if (!fired && millis()-start>=2000) { fired=true; return true; }
  } else {
    start=0; fired=false;
  }
  return false;
}
