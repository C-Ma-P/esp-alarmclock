// AudioManager.cpp
#include "AudioManager.h"
#include "Config.h"
#include "SDManager.h"
#include <driver/i2s.h>

FsFile audioFile;
bool  playingAudio = false;

void initAudio() {
  i2s_config_t cfg = {
    .mode           = i2s_mode_t(I2S_MODE_MASTER|I2S_MODE_TX),
    .sample_rate    = SAMPLE_RATE,
    .bits_per_sample= I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S|I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags= ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count  = DMA_BUF_COUNT,
    .dma_buf_len    = DMA_BUF_LEN,
    .use_apll       = true,
    .tx_desc_auto_clear = false
  };
  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCLK_PIN,
    .ws_io_num    = I2S_LRC_PIN,
    .data_out_num = I2S_DOUT_PIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_PORT,&cfg,0,nullptr);
  i2s_set_pin     (I2S_PORT,&pins);
  i2s_set_clk     (I2S_PORT,SAMPLE_RATE,I2S_BITS_PER_SAMPLE_16BIT,I2S_CHANNEL_STEREO);
  i2s_zero_dma_buffer(I2S_PORT);
  i2s_start       (I2S_PORT);
}

void startAudioPlayback(const char* fn) {
  if (audioFile) { audioFile.close(); playingAudio=false; i2s_zero_dma_buffer(I2S_PORT); }
  String path = String("/alarms/")+fn;
  audioFile = sd.open(path.c_str(),O_RDONLY);
  if (!audioFile) { Serial.println("[AUDIO] fail "+path); return; }
  if (path.endsWith(".wav")) audioFile.seek(44);
  playingAudio=true;
  Serial.println("[AUDIO] play "+path);
}

void stopAudioPlayback() {
  if (audioFile) {
    audioFile.close();
    playingAudio=false;
    i2s_zero_dma_buffer(I2S_PORT);
    Serial.println("[AUDIO] stopped");
  }
}

void handleAudioInLoop() {
  if (!playingAudio) return;
  const int chunk=256;
  int16_t buf[chunk];
  size_t toRead=chunk*sizeof(int16_t);
  int r = audioFile.read(buf,toRead);
  if (r>0) {
    int vol = analogRead(POT_PIN);
    for (int i=0;i<r/2;i++) buf[i]=(buf[i]*vol)>>12;
    size_t written;
    i2s_write(I2S_PORT,buf,r,&written,portMAX_DELAY);
  } else stopAudioPlayback();
}
