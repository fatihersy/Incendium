
#include "scene_in_game.h"
#include <core/fmemory.h>
#include <core/fmath.h>

#include "core/event.h"
#include "defines.h"
#include "game/resource.h"
#include "game/game_manager.h"
#include "game/user_interface.h"
#include "raylib.h"

typedef struct scene_in_game_state {
  game_manager_system_state *p_game_manager;
} scene_in_game_state;

static scene_in_game_state *in_game_state;

bool initialize_scene_in_game(Vector2 _screen_size) {

  in_game_state = (scene_in_game_state*)allocate_memory_linear(sizeof(scene_in_game_state), true);

  // Game
  if (!game_manager_initialize(_screen_size, SCENE_IN_GAME)) { // Inits player & spawns
    TraceLog(LOG_ERROR, "game_manager_initialize() failed");
    return false;
  }

  user_interface_system_initialize();
  set_player_user_interface(get_player_state_if_available());

  in_game_state->p_game_manager = get_game_manager();

  _set_player_position((Vector2) {SCREEN_WIDTH_DIV2, SCREEN_HEIGHT_DIV2});

  for (u32 i = 0; i < 360; i += 20) {
    Vector2 position = get_a_point_of_a_circle(_get_player_position(false), 500, i);
    Texture2D *tex = get_texture_by_enum(ENEMY_TEXTURE);

    rectangle_collision rect_col = (rectangle_collision){.rect = (Rectangle)
    {
      .x = position.x,
      .y = position.y,
      .width = tex->width,
      .height = tex->height
    },
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
    in_game_state->p_game_manager->spawn_collisions[in_game_state->p_game_manager->spawn_collision_count] = rect_col;
    in_game_state->p_game_manager->spawn_collisions[in_game_state->p_game_manager->spawn_collision_count].is_active = true;
  }

  return true;
}

void update_scene_in_game() {
  _update_player();
  _update_spawns();

  event_fire(EVENT_CODE_SCENE_MANAGER_SET_TARGET, 0, (event_context) {
    .data.i16[0] = _get_player_position(false).x,
    .data.i16[1] = _get_player_position(false).y,
  });
}

void render_scene_in_game() {
  i32 ms = 1000;
  for (i16 i = -ms; i < ms; i += 16)
  { // X Axis
    DrawLine(i, -ms, i, i + (ms), (Color){21, 17, 71, 255});
  }
  for (i16 i = -ms; i < ms; i += 16) 
  { // Y Axis
    DrawLine(-ms, i, i + (ms), i, (Color){21, 17, 71, 255});
  }

  _render_player();
  _render_spawns();
}
