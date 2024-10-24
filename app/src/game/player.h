

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"

typedef struct player_state
{
    bool initialized;
    Vector2 position;
    Texture2D player_texture;
    const char* texture_path;
    
} player_state;

bool player_system_initialize();

player_state* get_player_state();
void set_player_position(i16 x, i16 y);

bool update_player();
bool render_player();

#endif