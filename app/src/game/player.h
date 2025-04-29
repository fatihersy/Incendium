

#ifndef PLAYER_H
#define PLAYER_H

#include "defines.h"

typedef struct player_update_results {
  Vector2 move_request = Vector2 {};
  bool is_success = false;
} player_update_results;

bool player_system_initialize(void);

void move_player(Vector2 new_pos);
void add_exp_to_player(u32 exp);
void take_damage(u16 damage);
void heal_player(u16 amouth);

player_state* get_player_state(void);
Vector2 get_player_position(bool centered);

player_update_results update_player(void);
bool render_player(void);


#endif
