// Config.h
#pragma once
#include <Arduino.h>
#include <TMCStepper.h>

// ─── Pin Definitions ───────────────────────────────────────────────────
#define LED_PIN           LED_BUILTIN
#define MODE_SWITCH_PIN   21   // pull-up: HIGH = armed
#define BUTTON1_PIN       13   // pull-up, LOW=pressed (Snooze/Config)
#define BUTTON2_PIN       14   // pull-up, LOW=pressed (Web/OTA toggle)
#define POT_PIN           36   // ADC12: volume/motor pot
#define STEP_PIN          33
#define DIR_PIN           32
#define EN_PIN            4
#define RX2_PIN           3
#define TX2_PIN           1
#define SD_CS             5
#define SD_SCK            18
#define SD_MISO           19
#define SD_MOSI           23

// ─── Motor driver settings ───────────────────────────────────────────
#define R_SENSE     0.11f
#define DRIVER_ADDR 0b00

// ─── Stepper geometry ────────────────────────────────────────────────
static constexpr int STEPS_PER_REV      = 200;
static constexpr int MICROSTEPS         = 16;
static constexpr float DEGREES_PER_STEP = 360.0f/(STEPS_PER_REV*MICROSTEPS);
static constexpr float TICK_DEGREES      = 6.0f;
static constexpr int   STEPS_TO_MOVE     = int(TICK_DEGREES/DEGREES_PER_STEP + 0.5f);

// ─── I²S Audio pins & DMA ─────────────────────────────────────────────
#define I2S_PORT       I2S_NUM_0
#define I2S_BCLK_PIN   26
#define I2S_LRC_PIN    25
#define I2S_DOUT_PIN   22
#define SAMPLE_RATE    44100
static constexpr int DMA_BUF_COUNT = 4;
static constexpr int DMA_BUF_LEN   = 512;

// ─── Globals instantiated in Config.cpp ──────────────────────────────
extern TMC2209Stepper driver;

// Motor globals
extern bool          motorVarControlEnabled;
extern int           motorMaxSpeed;
extern unsigned long lastMotorStepMsVar;
extern unsigned long lastStepMs;
