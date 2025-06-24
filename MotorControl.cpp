// MotorControl.cpp
#include "MotorControl.h"
#include <driver/ledc.h>

static const int STEP_CH = 0;              // LEDC channel for PWM
static unsigned long nextPulseMicros = 0;  // next bit-bang pulse time

void initMotor() {
  // 1) UART for TMC2209
  Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);

  // 2) TMC2209 core setup (reduced currents)
  driver.begin();
  driver.toff(5);
  driver.blank_time(24);
  driver.rms_current(600);     // 600 mA RMS instead of 800
  driver.microsteps(8);        // full-step by default
  driver.intpol(true);
  driver.en_spreadCycle(true);
  driver.TCOOLTHRS(0);
  driver.TPOWERDOWN(1);        // fastest auto‐power-down after step
  delay(50);

  // 3) Hold vs run currents
  driver.ihold(4);   // ≈ 100 mA hold
  driver.irun(12);   // ≈ 230 mA run
  driver.iholddelay(2);

  // 4) GPIO directions & defaults
  pinMode(EN_PIN,   OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN,  OUTPUT);

  digitalWrite(EN_PIN, HIGH);  // driver disabled until first pulse
  digitalWrite(DIR_PIN, HIGH); // default direction

  // 5) Seed bit-bang timer
  nextPulseMicros = micros();
}

void initMotorPWM() {
  ledcSetup(STEP_CH, 1000, 8);
  ledcAttachPin(STEP_PIN, STEP_CH);
  ledcWrite(STEP_CH, 0);
}

void setMotorSpeedHz(float stepsPerSec) {
  if (stepsPerSec <= 0) {
    // stop PWM & disable driver
    ledcWrite(STEP_CH, 0);
    digitalWrite(EN_PIN, HIGH);
    return;
  }

  // enable driver + start PWM at desired rate
  digitalWrite(EN_PIN, LOW);
  uint32_t freq = min<uint32_t>((uint32_t)stepsPerSec, 150000);
  ledcSetup(STEP_CH, freq, 8);
  ledcAttachPin(STEP_PIN, STEP_CH);
  ledcWrite(STEP_CH, 128);  // 50% duty → one pulse per cycle
}
