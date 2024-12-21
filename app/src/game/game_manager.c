#include "game_manager.h"

#include "core/window.h"
#include "core/event.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "defines.h"
#include "game/player.h"
#include "game/spawn.h"

static game_manager_system_state *game_manager_state;

bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);

bool game_manager_initialize(Vector2 _screen_size, scene_type _scene_data) {
  if (game_manager_state) return false;

  game_manager_state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  game_manager_state->is_game_paused = true;
  game_manager_state->scene_data = _scene_data;

  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "player_system_initialize() failed");
    return false;
  }
  if (!spawn_system_initialize()) {
    TraceLog(LOG_ERROR, "spawn_system_initialize() failed");
    return false;
  }

  event_register(EVENT_CODE_PAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, 0, game_manager_on_event);
  event_register(EVENT_CODE_RELOCATE_SPAWN_COLLISION, 0, game_manager_on_event);
  
  game_manager_state->game_manager_initialized = true;
  return true;
}

void update_game_manager(scene_type _scene_data) {
  game_manager_state->scene_data = _scene_data;
}

void _update_spawns() {
  update_spawns(get_player_position(true));
}
void _update_player() {
  update_player(game_manager_state->scene_data);
}

void _render_player() {
  render_player();
}
void _render_spawns() {
  render_spawns();
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

Vector2 _get_screen_half_size() {
  return get_screen_half_size();
}

void _set_player_position(Vector2 position) {
  get_player_state()->position = position;
}
u16 _spawn_character(Character2D _character) {
  return spawn_character(_character);
}

game_manager_system_state* get_game_manager() {
  if (game_manager_state) {
    return game_manager_state;
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
  if (!projectile) TraceLog(LOG_ERROR, 
  "ERROR::game_manager::check_any_collide()::Tried to check collide with uninitialized actor");
  
  for (u16 i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
    if (game_manager_state->spawn_collisions[i].is_active){
      if (CheckCollisionRecs(game_manager_state->spawn_collisions[i].rect, projectile->collision)) {
        u16 result = damage_spawn(game_manager_state->spawn_collisions[i].owner_id, projectile->damage);
      }
    }
  }
}

void damage_any_collider_by_type(Character2D *from_actor, actor_type to_type) {
  if (!from_actor) {
    TraceLog(LOG_ERROR, "ERROR::game_manager::check_any_collide()::Tried to "
                        "check collide with uninitialized actor");
  }
  switch (to_type) {
  case ENEMY: {
    for (u16 i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
      if (game_manager_state->spawn_collisions[i].is_active){
        if (CheckCollisionRecs(game_manager_state->spawn_collisions[i].rect, from_actor->collision)) {
          u16 result = damage_spawn(game_manager_state->spawn_collisions[i].owner_id, from_actor->damage);
        }
      }
    }
    break;
  }
  case PLAYER: {
    if (CheckCollisionRecs(from_actor->collision, get_player_state()->collision)) {
      event_fire(EVENT_CODE_PLAYER_DEAL_DAMAGE, 0, (event_context) { .data.u8[0] = from_actor->damage});
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
    game_manager_state->is_game_paused = true;

    return true;
    break;
  }
  case EVENT_CODE_UNPAUSE_GAME: {
    game_manager_state->is_game_paused = false;

    return true;
    break;
  }

  case EVENT_CODE_RELOCATE_SPAWN_COLLISION: {
    for (int i = 1; i <= game_manager_state->spawn_collision_count; ++i) {
      if (game_manager_state->spawn_collisions[i].owner_id ==
          context.data.u16[0]) {
        game_manager_state->spawn_collisions[i].rect.x = context.data.u16[1];
        game_manager_state->spawn_collisions[i].rect.y = context.data.u16[2];
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


