
#ifndef GAME_H
#define GAME_H

#include "defines.h"

typedef struct game_state {
    f32 delta_time;
} game_state;

bool game_initialize();

bool game_update();

bool game_render();

void game_on_resize();

#endif
