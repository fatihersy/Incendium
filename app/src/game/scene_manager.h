
#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "defines.h"

bool scene_manager_initialize(game_manager_system_state* _game_manager);
void update_scene_manager();


void set_current_scene_type(scene_type type);
scene_type get_current_scene_type();
void load_scene();

void set_player_position(i16 x, i16 y);
Vector2 get_spectator_position();
Vector2 get_player_position(bool centered);
Vector2 get_player_dimentions();
Vector2 get_player_dimentions_div2();
Character2D* get_actor_by_id(u16 ID);

void update_scene_main_menu();
void update_scene_in_game();
void update_scene_in_game_edit();
void render_scene_main_menu();
void render_scene_in_game();
void render_scene_in_game_edit();

#endif