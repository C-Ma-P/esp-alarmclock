// Input.h
#pragma once
#include <Arduino.h>

// Must be called once in setup() to capture initial switch state
void initInput();

// Switch/button queries
bool isSwitchUp();
bool isSwitchDown();
bool switchToggled();
unsigned long getSwitchHoldDuration();
bool buttonA_LongPressed();
bool buttonA_ShortPressed();
bool buttonB_ShortPressed();
bool buttonAB_Held2s();
