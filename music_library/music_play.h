/*
Copyright (c) 2019-2023 Aysi 773917760@qq.com. All right reserved
Official site: www.mtf123.club

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

It under the terms of the Apache as published;
either version 2 of the License,
or (at your option) any later version.
*/

#ifndef _MUSIC_PLAY_H
#define _MUSIC_PLAY_H

#include "audio_port.h"

void MP3WAVplay(char *path);
void MP3WAVplay_exit(void);
void AUDIO_Demo(void);

int wav_init(char *path, int play_time);
void _WAV_Play2(void);

int mp3_init(char *path, int play_time);
void _mp3_Play2(void);

int music_init(char *path, int play_time);
void music_run(void);

#endif
