// AudioManager.h
#pragma once
#include <Arduino.h>
#include <SdFat.h>

void initAudio();
void startAudioPlayback(const char* filename);
void stopAudioPlayback();
void handleAudioInLoop();

extern bool playingAudio;
