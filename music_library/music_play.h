#ifndef _MUSIC_PLAY_H
#define _MUSIC_PLAY_H

#include "audio_port.h"

void MP3WAVplay(char *path);
void MP3WAVplay_exit(void);
void AUDIO_Demo(void);

int wav_init(char *path, int play_time);
void _WAV_Play2(void);

#endif
