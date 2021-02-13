#ifndef SPEAKER_I2S_H_
#define SPEAKER_I2S_H_

#include "Arduino.h"
#include <driver/i2s.h>

// I2S perhiperhal number
#define I2S_CHANNEL                         I2S_NUM_0

// I2S speaker pins
#define I2S_SPEAKER_PIN_BIT_CLOCK           26
#define I2S_SPEAKER_PIN_WORD_SELECT         25
#define I2S_SPEAKER_PIN_DATA_OUT            27
#define I2S_SPEAKER_PIN_DATA_IN             I2S_PIN_NO_CHANGE

bool I2S_speaker_init(int sample_rate, int bits_per_sample);
void I2S_speaker_uninit();

#endif
