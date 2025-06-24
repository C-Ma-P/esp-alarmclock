// MotorControl.h
#pragma once

#include <Arduino.h>
#include <TMCStepper.h>
#include "Config.h" // RX2_PIN, TX2_PIN, EN_PIN, STEP_PIN, DIR_PIN, STEPS_TO_MOVE, motorVarControlEnabled, motorMaxSpeed, driver


// Initialize the TMC2209 driver and PWM subsystem
void initMotor();
void initMotorPWM();

// In test (variable) mode: set step rate in steps/sec.
// Passing 0 → stop PWM & disable driver.
void setMotorSpeedHz(float stepsPerSec);

// In clock mode: execute one-second burst via bit-bang.
void stepMotorFixed(unsigned long now);
