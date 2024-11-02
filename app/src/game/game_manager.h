

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "defines.h"

bool game_manager_initialize(Vector2 screen_size);

void load_scene();

void set_player_position(i16 x, i16 y);
void set_current_scene_type(scene_type type);
Vector2 get_player_position();
Character2D* get_actor_by_id(u16 ID);
float get_time_elapsed(elapse_time_type type);
scene_type get_current_scene_type();

bool damage_any_collade(Character2D* _character);

void update_game_manager();
void render_game_manager();

#endif
