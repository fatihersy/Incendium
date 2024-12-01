

#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "defines.h"

bool game_manager_initialize(Vector2 screen_size);

Vector2 focus_at();
scene_type get_active_scene();
float get_time_elapsed(elapse_time_type type);

void damage_any_spawn(Character2D *projectile);
void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type);

void update_game_manager();
void render_game_manager();

#endif
