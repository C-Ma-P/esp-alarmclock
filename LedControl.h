// LEDControl.h
#pragma once
#include <Arduino.h>

// Blink/pulse control
void setLEDPulse(unsigned long onMs, unsigned long offMs);
void stopLEDPulse();
void setLEDBlink(float hz);
void stopLEDBlink();

// Call each loop to update LED
void updateLED(unsigned long now);
