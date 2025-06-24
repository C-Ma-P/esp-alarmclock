// Helpers.h
#pragma once
#include <Arduino.h>

void streamSdFile(const char* path,const char* contentType);
long secondsSinceMidnight();
void dailyResetTriggeredFlags();
