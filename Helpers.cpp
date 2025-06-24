// Helpers.cpp
#include "Helpers.h"
#include "WebServerAPI.h"
#include "Persistence.h"
#include "SDManager.h"
#include <time.h>

void streamSdFile(const char* path,const char* contentType) {
  SdFile f;
  if (!f.open(path,O_READ)) {
    server.send(404,"text/plain","Not found");
    return;
  }
  uint32_t sz = f.fileSize();
  WiFiClient cli = server.client();
  cli.printf("HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\nConnection: close\r\n\r\n",contentType,sz);
  uint8_t buf[512];
  while (sz>0) {
    size_t toRead = min((uint32_t)sizeof(buf),sz);
    size_t r = f.read(buf,toRead);
    if (r==0) break;
    cli.write(buf,r);
    sz-=r;
  }
  f.close();
}

long secondsSinceMidnight() {
  time_t t=time(nullptr);
  tm tm; localtime_r(&t,&tm);
  return tm.tm_hour*3600L+tm.tm_min*60L+tm.tm_sec;
}

void dailyResetTriggeredFlags() {
  static int lastDay=-1;
  time_t t=time(nullptr);
  tm tm; localtime_r(&t,&tm);
  if (tm.tm_mday!=lastDay) {
    lastDay=tm.tm_mday;
    for (auto &a:alarms) {
      a.triggeredToday=false;
      a.snoozeTs=0;
    }
  }
}
