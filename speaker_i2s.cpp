#include "speaker_i2s.h"
#include <driver/i2s.h>

bool I2S_speaker_init(int sample_rate, int bits_per_sample) {
  if (bits_per_sample != I2S_BITS_PER_SAMPLE_16BIT) {
    Serial.println("Invalid bits per sample");
    return false;
  }
  // Configure I2S driver
  i2s_config_t i2s_config;
  memset(&i2s_config, 0, sizeof(i2s_config));
  i2s_config.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.sample_rate          = sample_rate;
  i2s_config.bits_per_sample      = (i2s_bits_per_sample_t) bits_per_sample;
  i2s_config.channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
  i2s_config.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  i2s_config.dma_buf_count        = 8;
  i2s_config.dma_buf_len          = 1024;
  i2s_config.use_apll             = true;
  i2s_config.tx_desc_auto_clear   = true;
  
  if (i2s_driver_install(I2S_CHANNEL, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("i2s_driver_install() error");
    return false;
  }
  // Configure I2S pins
  i2s_pin_config_t pins = {
    .bck_io_num = I2S_SPEAKER_PIN_BIT_CLOCK,
    .ws_io_num =  I2S_SPEAKER_PIN_WORD_SELECT,
    .data_out_num = I2S_SPEAKER_PIN_DATA_OUT,
    .data_in_num = I2S_SPEAKER_PIN_DATA_IN
  };
  if (i2s_set_pin(I2S_CHANNEL, &pins) != ESP_OK) {
    Serial.println("i2s_set_pin() error");
    return false;
  }
  return true;
}

void I2S_speaker_uninit() {
  if (i2s_driver_uninstall(I2S_CHANNEL) != ESP_OK) {
    Serial.println("i2s_driver_uninstall() error");
  }
}
