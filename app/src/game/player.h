

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"


bool player_system_initialize(void);

player_state* get_player_state(void);
Vector2 get_player_position(bool centered);

bool update_player(void);
bool render_player(void);

#endif
