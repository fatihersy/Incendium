
#include "scene_in_game.h"

#include <core/fmath.h>

#include "game/game_manager.h"
#include "game/resource.h"
#include "game/user_interface.h"

typedef struct scene_in_game_state {
  game_manager_system_state *p_game_manager;
} scene_in_game_state;

static scene_in_game_state *in_game_state;

bool initialize_scene_in_game(Vector2 _screen_size, Vector2 _screen_half_size) {

  // Game
  if (!game_manager_initialize(_screen_size)) {
    TraceLog(LOG_ERROR, "game_manager_initialize() failed");
    return false;
  }

  user_interface_system_initialize(); // Requires player

  in_game_state->p_game_manager = get_game_manager();

  for (u32 i = 0; i < 360; i += 20) {
    _set_player_position(get_game_manager()->screen_half_size);

    Vector2 position =
        get_a_point_of_a_circle(_get_player_position(false), 500, i);
    Texture2D *tex = get_texture_by_enum(ENEMY_TEXTURE);

    rectangle_collision rect_col =
        (rectangle_collision){.rect = (Rectangle){.x = position.x,
                                                  .y = position.y,
                                                  .width = tex->width,
                                                  .height = tex->height},
                              .owner_type = ENEMY};

    rect_col.owner_id = _spawn_character((Character2D){
        .character_id = 0,
        .tex = tex,
        .initialized = false,
        .collision = rect_col.rect,
        .position = position,
        .w_direction = LEFT,
        .type = ENEMY,
        .rotation = 0,
        .health = 100,
        .damage = 10,
        .speed = 1,
    });
    in_game_state->p_game_manager->spawn_collision_count++;
    in_game_state->p_game_manager->spawn_collisions
        [in_game_state->p_game_manager->spawn_collision_count] = rect_col;
    in_game_state->p_game_manager
        ->spawn_collisions[in_game_state->p_game_manager->spawn_collision_count]
        .is_active = true;
  }

  return true;
}

void update_scene_in_game() {
  game_manager_system_state *game_manager_state = get_game_manager();

  _update_player();
  _update_spawns();
}

void render_scene_in_game() {
  _render_player();
  _render_spawns();

  /*   for (i16 i = -ms; i < ms; i += 16)
    { // X Axis
      DrawLine(i, -ms, i, i + (ms), (Color){21, 17, 71, 255});
  }
      for (i16 i = -ms; i < ms; i += 16) { // Y Axis
        DrawLine(-ms, i, i + (ms), i, (Color){21, 17, 71, 255});
      } */
}
