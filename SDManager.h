// SDManager.h
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>

bool initSD();

extern SPIClass  sdSPI;
extern SdFat     sd;
extern SdFile    uploadFile;
extern bool      sdMounted;
