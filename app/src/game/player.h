

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"

typedef struct player_update_results {
  Vector2 move_request;

  bool is_success;
} player_update_results;

bool player_system_initialize(void);

player_state* get_player_state(void);
Vector2 get_player_position(bool centered);

player_update_results update_player(void);
bool render_player(void);

void move_player(Vector2 new_pos);

#endif
