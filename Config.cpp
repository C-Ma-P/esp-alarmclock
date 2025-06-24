// Config.cpp
#include "Config.h"

// Serial2 is provided by ESP32 core
TMC2209Stepper driver(&Serial2, R_SENSE, DRIVER_ADDR);

bool          motorVarControlEnabled = false;
int           motorMaxSpeed          = 50;
unsigned long lastMotorStepMsVar     = 0;
unsigned long lastStepMs             = 0;
