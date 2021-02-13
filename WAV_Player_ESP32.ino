#include "wav_player.h"
#include "SD_MMC.h"

void setup() {
  Serial.begin(115200);
  delay(1000);
  if (!SD_MMC.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  delay(1000);
  int status_ = play_wav("/song1.wav");
  if (status_ = PLAY_SUCCESS) {
    Serial.println("Works!");
  } else {
    Serial.println("Error!");
  }
}

void loop() {
}
