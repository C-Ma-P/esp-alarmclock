// WebServerAPI.h
#pragma once
#include <WebServer.h>
#include <Arduino.h>


void handleGetAlarms();
void handlePostAlarm();
void handlePatchAlarm();
void handleDeleteAlarm();
void handleGetSettings();
void handlePutSettings();
void handlePatchMaster();
void handleFS();
void handleUpload();
void handleDeleteEntry();
void handlePostTest();
void handleGetTestStatus();
void handleSetTestAlarm();
void handleGetMotor();
void handlePostMotor();


// global server instance
extern WebServer server;

// test-mode globals
extern bool          testModeEnabled;
extern String        testAudioFile;
extern unsigned long testAlarmTs;
extern bool          testAlarmScheduled;
extern bool          testAlarmRinging;

// register all REST + file-endpoints (call when entering CONFIG_WEB)
void registerWebEndpoints();
