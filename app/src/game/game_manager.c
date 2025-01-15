#include "game_manager.h"
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "game/tilemap.h"
#include "game/player.h"
#include "game/spawn.h"

static game_manager_system_state *state;

bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);

bool game_manager_initialize(Vector2 _screen_size, scene_type _scene_data) {
  if (state) return false;

  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  state->is_game_paused = true;
  state->scene_data = _scene_data;

  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "player_system_initialize() failed");
    return false;
  }
  if (!spawn_system_initialize()) {
    TraceLog(LOG_ERROR, "spawn_system_initialize() failed");
    return false;
  }

  create_tilemap(TILESHEET_TYPE_MAP, (Vector2) {0, 0}, 100, 16*3, WHITE, &state->map);
  if(!state->map.is_initialized) {
    TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  load_map_data(&state->map, &state->map_str_package);
  if (!state->map_str_package.is_success) {
    TraceLog(LOG_ERROR, "game_manager_initialize::game manager unable to load map");
    return false;
  }

  event_register(EVENT_CODE_PAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_RELOCATE_SPAWN_COLLISION, 0, game_manager_on_event);
  
  state->game_manager_initialized = true;
  return true;
}

void update_game_manager(scene_type _scene_data) {
  state->scene_data = _scene_data;
}

void _update_spawns() {
  update_spawns(get_player_position(true));
}
void _update_player() {
  update_player(state->scene_data);
}

void _render_player() {
  render_player();
}
void _render_spawns() {
  render_spawns();
}

void _render_map() {
  render_tilemap(&state->map);
}

Vector2 _get_player_position(bool centered) {
  return get_player_position(centered);
}

player_state* get_player_state_if_available() {
  if (get_player_state()) {
    return get_player_state();
  }

  return (player_state*) {0};
}

void _set_player_position(Vector2 position) {
  get_player_state()->position = position;
}
u16 _spawn_character(Character2D _character) {
  return spawn_character(_character);
}

game_manager_system_state* get_game_manager() {
  if (state) {
    return state;
  }

  return (game_manager_system_state*){0};
}

float get_time_elapsed(elapse_time_type type) {
  timer *time = get_timer();

  if (time->remaining == 0) {
    reset_time(type);
    return 0;
  };

  return time->remaining;
}

void damage_any_spawn(Character2D *projectile) {
  if (!projectile) {
    TraceLog(LOG_ERROR, 
    "game_manager::damage_any_spawn()::Recieved actor was NULL");
    return;
  }
  
  for (u16 i = 1; i <= state->spawn_collision_count; ++i) {
    if (state->spawn_collisions[i].is_active){
      if (CheckCollisionRecs(state->spawn_collisions[i].rect, projectile->collision)) {
        u16 result = damage_spawn(state->spawn_collisions[i].owner_id, projectile->damage);
      }
    }
  }
}

void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type) {
  if (!from_actor) {
    TraceLog(LOG_ERROR, 
    "game_manager::damage_any_collider_by_type()::Recieved actor was NULL");
    return;
  }
  switch (to_type) {
  case ENEMY: {
    for (u16 i = 1; i <= state->spawn_collision_count; ++i) {
      if (state->spawn_collisions[i].is_active){
        if (CheckCollisionRecs(state->spawn_collisions[i].rect, from_actor->collision)) {
          u16 result = damage_spawn(state->spawn_collisions[i].owner_id, from_actor->damage);
        }
      }
    }
    break;
  }
  case PLAYER: {
    if (CheckCollisionRecs(from_actor->collision, get_player_state()->collision)) {
      event_fire(EVENT_CODE_PLAYER_TAKE_DAMAGE, 0, (event_context) { .data.u8[0] = from_actor->damage});
      return;
    }
    break;
  }
  case PROJECTILE_ENEMY: break; 
  case PROJECTILE_PLAYER: break; 
  }
}

bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context) {
  switch (code) {
  case EVENT_CODE_PAUSE_GAME: {
    state->is_game_paused = true;

    return true;
    break;
  }
  case EVENT_CODE_UNPAUSE_GAME: {
    state->is_game_paused = false;

    return true;
    break;
  }

  case EVENT_CODE_RELOCATE_SPAWN_COLLISION: {
    for (int i = 1; i <= state->spawn_collision_count; ++i) {
      if (state->spawn_collisions[i].owner_id ==
          context.data.u16[0]) {
        state->spawn_collisions[i].rect.x = context.data.u16[1];
        state->spawn_collisions[i].rect.y = context.data.u16[2];
        return true;
      }
    }
    break;
  }
  default:
    break;
  }

  return false;
}


