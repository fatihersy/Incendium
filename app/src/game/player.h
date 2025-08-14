

#ifndef PLAYER_H
#define PLAYER_H

#include "game_types.h"

typedef struct player_update_results {
  Vector2 move_request;
  bool is_success;
  player_update_results(void) {
    this->move_request = ZEROVEC2;
    this->is_success = false;
  }
  player_update_results(Vector2 move_req, bool is_success) : player_update_results() {
    this->move_request = move_req;
    this->is_success = is_success;
  }
} player_update_results;

bool player_system_initialize(const camera_metrics* in_camera_metrics,const app_settings* in_app_settings,const ingame_info* in_ingame_info);

void player_move_player(Vector2 new_pos);
void player_add_exp_to_player(i32 raw_exp);
void player_take_damage(i32 damage);
void player_heal_player(i32 amouth);

player_state* get_player_state(void);
const player_state* get_default_player(void);

player_update_results update_player(void);
bool render_player(void);


#endif
