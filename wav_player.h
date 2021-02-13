#ifndef WAV_PLAYER_H_
#define WAV_PLAYER_H_

enum play_status {
  PLAY_ERROR,
  PLAY_SUCCESS
};

int play_wav(const char* song_name);

#endif
