#include "wav_player.h"
#include "speaker_i2s.h"
#include "FS.h"
#include "SD_MMC.h"
#include "Arduino.h"

struct wav_format_t {
  uint32_t data_position;
  uint32_t data_size;
  uint32_t sampling_rate;
  uint16_t bits_per_sample;
  bool is_supported;
} ;

struct wav_info_t {
  wav_format_t wav_format;
  bool status;
};

static wav_info_t extract_wav_info(const char* song_name) {
  Serial.println("Extracting file info");
  wav_info_t wav_info;
  memset(&wav_info, 0, sizeof(wav_info));
  wav_info.status = true;

  if (song_name == NULL) {
    Serial.println("song_name is null");
    wav_info.status = false;
    return wav_info;
  }

  File wav_file = SD_MMC.open(song_name);
  if (wav_file == NULL) {
    Serial.println("Failed to open file");
    wav_info.status = false;
    return wav_info;
  }

  uint8_t CHUNK_ID[4];
  wav_file.read(CHUNK_ID, 4);
  if (CHUNK_ID[0] != 'R' || CHUNK_ID[1] != 'I' || CHUNK_ID[2] != 'F' || CHUNK_ID[3] != 'F') {
    Serial.println("Chunk ID is not RIFF");
    return wav_info;
  }

  uint8_t CHUNK_SIZE[4];
  wav_file.read(CHUNK_SIZE, 4);

  uint8_t FORMAT[4];
  wav_file.read(FORMAT, 4);
  if (FORMAT[0] != 'W' || FORMAT[1] != 'A' || FORMAT[2] != 'V' || FORMAT[3] != 'E') {
    Serial.println("FORMAT is not WAVE");
    return wav_info;
  }

  uint8_t FMT_SUBCHUNK[4];
  wav_file.read(FMT_SUBCHUNK, 4);
  while (!(FMT_SUBCHUNK[0] == 'f' && FMT_SUBCHUNK[1] == 'm' && FMT_SUBCHUNK[2] == 't' && FMT_SUBCHUNK[3] == ' ')) {
    uint8_t SUBCHUNK_SIZE[4];
    wav_file.read(SUBCHUNK_SIZE, 4);
    uint32_t subchunk_size = SUBCHUNK_SIZE[0] | SUBCHUNK_SIZE[1] << 8 | SUBCHUNK_SIZE[2] << 16 | SUBCHUNK_SIZE[3] << 24;
    wav_file.seek(subchunk_size, SeekCur );
    wav_file.read(FMT_SUBCHUNK, 4);
  }

  uint8_t FMT_SUBCHUNK_SIZE[4];
  wav_file.read(FMT_SUBCHUNK_SIZE, 4);
  uint32_t fmt_subchunk_size = FMT_SUBCHUNK_SIZE[0] | FMT_SUBCHUNK_SIZE[1] << 8 | FMT_SUBCHUNK_SIZE[2] << 16 | FMT_SUBCHUNK_SIZE[3] << 24;

  uint8_t AUDIO_FORMAT[2];
  wav_file.read(AUDIO_FORMAT, 2);
  if (!( (AUDIO_FORMAT[0] == 0x01 && AUDIO_FORMAT[1] == 0x00) || (AUDIO_FORMAT[0] == 0xFE && AUDIO_FORMAT[1] == 0xFF))) {
    Serial.println("File is not standard PCM");
    return wav_info;
  }

  uint8_t NUM_CHANNELS[2];
  wav_file.read(NUM_CHANNELS, 2);
  if (!(NUM_CHANNELS[0] == 0x01 && NUM_CHANNELS[1] == 0x00)) {
    Serial.println("File is not mono");
    return wav_info;
  }

  uint8_t SAMPLING_RATE[4];
  wav_file.read(SAMPLING_RATE, 4);
  uint32_t sampling_rate = SAMPLING_RATE[0] | SAMPLING_RATE[1] << 8 | SAMPLING_RATE[2] << 16 | SAMPLING_RATE[3] << 24;

  uint8_t BYTE_RATE[4];
  wav_file.read(BYTE_RATE, 4);

  uint8_t BLOCK_ALIGN[2];
  wav_file.read(BLOCK_ALIGN, 2);

  uint8_t BITS_PER_SAMPLE[2];
  wav_file.read(BITS_PER_SAMPLE, 2);
  uint16_t bits_per_sample = BITS_PER_SAMPLE[1] << 8 | BITS_PER_SAMPLE[0];
  if (bits_per_sample != 16) {
    Serial.println("Bits per sample should be 16");
    return wav_info;
  }

  if (fmt_subchunk_size > 16) {
    size_t bits_until_fmt_end = fmt_subchunk_size - 16;
    wav_file.seek(bits_until_fmt_end, SeekCur);
  }

  uint8_t DATA_SUBCHUNK[4];
  wav_file.read(DATA_SUBCHUNK, 4);
  while (!(DATA_SUBCHUNK[0] == 'd' && DATA_SUBCHUNK[1] == 'a' && DATA_SUBCHUNK[2] == 't' && DATA_SUBCHUNK[3] == 'a')) {
    uint8_t SUBCHUNK_SIZE[4];
    wav_file.read(SUBCHUNK_SIZE, 4);
    uint32_t subchunk_size = SUBCHUNK_SIZE[0] | SUBCHUNK_SIZE[1] << 8 | SUBCHUNK_SIZE[2] << 16 | SUBCHUNK_SIZE[3] << 24;
    wav_file.seek(subchunk_size, SeekCur);
    wav_file.read(DATA_SUBCHUNK, 4);
  }

  uint8_t DATA_SUBCHUNK_SIZE[4];
  wav_file.read(DATA_SUBCHUNK_SIZE, 4);
  uint32_t data_subchunk_size = DATA_SUBCHUNK_SIZE[0] | DATA_SUBCHUNK_SIZE[1] << 8 | DATA_SUBCHUNK_SIZE[2] << 16 | DATA_SUBCHUNK_SIZE[3] << 24;
  if (data_subchunk_size == 0) {
    Serial.println("Data should not be empty");
    return wav_info;
  }

  wav_info.wav_format.data_position = wav_file.position();
  wav_info.wav_format.data_size = data_subchunk_size;
  wav_info.wav_format.sampling_rate = sampling_rate;
  wav_info.wav_format.bits_per_sample = bits_per_sample;
  wav_info.wav_format.is_supported = true;

  wav_file.close();
  return wav_info;
}

int play_wav(const char* song_name) {
  Serial.print("Playing ");
  Serial.println(song_name);
  wav_info_t wav_info = extract_wav_info(song_name);

  if (wav_info.status == false) {
    Serial.println("Error during wav data extraction");
    return PLAY_ERROR;
  }

  if (wav_info.wav_format.is_supported == false) {
    Serial.println("This file is not supported");
    return PLAY_ERROR;
  }

  // Open file
  String fullpath = String("/sdcard") + String(song_name);
  FILE* audio_file = fopen(fullpath.c_str(), "rb");
  if (audio_file == NULL) {
    Serial.println("Error opening file");
    return PLAY_ERROR;
  }

  if (fseek (audio_file, wav_info.wav_format.data_position, SEEK_SET) != 0) {
    Serial.println("Error during file position change");
    fclose(audio_file);
    return PLAY_ERROR;
  }

  // Initialize I2S
  if (!I2S_speaker_init(wav_info.wav_format.sampling_rate, wav_info.wav_format.bits_per_sample)) {
    Serial.println("Error during I2S initialization");
    fclose(audio_file);
    return PLAY_ERROR;
  }

  uint32_t bytes_written;
  const size_t BUFFER_SIZE = 500;
  const size_t LOOP_COUNT = wav_info.wav_format.data_size / BUFFER_SIZE;
  uint8_t* buf = (uint8_t*)malloc(BUFFER_SIZE);
  size_t BYTES_READ = 0;
  for (int i = 0; i < LOOP_COUNT; ++i) {
    if (read(fileno(audio_file), buf, BUFFER_SIZE) == -1) {
      Serial.println("error in read(fileno(audio_file), buf, BUFFER_SIZE)");
      free(buf);
      I2S_speaker_uninit();
      fclose(audio_file);
      return PLAY_ERROR;
    }
    if (i2s_write(I2S_CHANNEL, buf, BUFFER_SIZE, &bytes_written, portMAX_DELAY) != ESP_OK) {
      Serial.println("i2s_write() error");
      free(buf);
      I2S_speaker_uninit();
      fclose(audio_file);
      return PLAY_ERROR;
    }
  }

  const size_t DATA_REMAINDER_SIZE = wav_info.wav_format.data_size % BUFFER_SIZE;
  if (DATA_REMAINDER_SIZE != 0) {
    if (read(fileno(audio_file), buf, DATA_REMAINDER_SIZE) == -1) {
      Serial.println("error in read(fileno(audio_file), buf, BUFFER_SIZE)");
      free(buf);
      I2S_speaker_uninit();
      fclose(audio_file);
      return PLAY_ERROR;

    }
    if (i2s_write(I2S_CHANNEL, buf, DATA_REMAINDER_SIZE, &bytes_written, portMAX_DELAY) != ESP_OK) {
      Serial.println("i2s_write() error");
      free(buf);
      I2S_speaker_uninit();
      fclose(audio_file);
      return PLAY_ERROR;
    }
  }

  free(buf);
  I2S_speaker_uninit();
  fclose(audio_file);
  return PLAY_SUCCESS;
}
