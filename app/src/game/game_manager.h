

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "defines.h"

bool game_manager_initialize(Vector2 screen_size);

void load_scene();

void set_player_position(i16 x, i16 y);
void set_current_scene_type(scene_type type);
Vector2 get_player_position(bool centered);
Vector2 get_player_dimentions();
Vector2 get_player_dimentions_div2();
Character2D* get_actor_by_id(u16 ID);
float get_time_elapsed(elapse_time_type type);
scene_type get_current_scene_type();

void damage_any_spawn(Character2D *projectile);
void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type);

void update_game_manager();
void render_game_manager();

#endif
