// SDManager.cpp
#include "SDManager.h"
#include "Config.h"

SPIClass sdSPI(VSPI);
SdFat    sd;
SdFile   uploadFile;
bool     sdMounted = false;

bool initSD() {
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!sd.begin(SD_CS, SPI_HALF_SPEED)) {
    sdMounted = false;
    Serial.println("[SD] init failed");
    return false;
  }
  sdMounted = true;
  Serial.println("[SD] mounted");
  if (!sd.exists("/web"))    { sd.mkdir("/web");    Serial.println("[SD] /web"); }
  if (!sd.exists("/alarms")) { sd.mkdir("/alarms"); Serial.println("[SD] /alarms"); }
  return true;
}
