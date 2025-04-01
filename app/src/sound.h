#ifndef SOUND_H
#define SOUND_H

#include "defines.h"

void sound_system_initialize(void);

void update_sound_system(void);

void play_sound(sound_id id);
void play_music(music_id id);
void reset_music(music_id id);
void reset_sound(sound_id id);

#endif
