#include "game_manager.h"
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "game/tilemap.h"
#include "game/player.h"
#include "game/spawn.h"

#include "abilities/ability_manager.h"

static game_manager_system_state *state;

void update_collisions();

bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context);

bool game_manager_initialize(Vector2 _screen_size, scene_type _scene_data) {
  if (state) return false;

  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  state->is_game_paused = true;

  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "player_system_initialize() failed");
    return false;
  }
  state->p_player = get_player_state();
  _add_ability(ABILITY_TYPE_FIREBALL);

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
  event_register(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, game_manager_on_event);
  
  state->game_manager_initialized = true;
  return true;
}

void update_game_manager() {
  if (!state) {
    return;
  }

  update_collisions();
}
void update_collisions() {
  for (int i=0; i<state->projectile_count; ++i) { for (u16 j = 1; j <= state->spawn_collision_count; ++j) {
      if (state->spawn_collisions[j].is_active){ 
        if (CheckCollisionRecs(state->spawn_collisions[j].rect, state->projectiles[i].collision)) {
          u16 result = damage_spawn(state->spawn_collisions[j].owner_id, state->projectiles[i].damage);
          // TODO: Process result
        }
      }
    }
  }

  for (int i=0; i<state->spawn_collision_count; ++i) {if (state->spawn_collisions[i].is_active){ 
    Character2D* spawn = &get_spawn_system()->spawns[state->spawn_collisions[i].owner_id];
    damage_any_collider_by_type(spawn, PLAYER);
  }}
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
    if (CheckCollisionRecs(from_actor->collision, state->p_player->collision)) {
      event_fire(EVENT_CODE_PLAYER_TAKE_DAMAGE, 0, (event_context) { .data.u8[0] = from_actor->damage});
      return;
    }
    break;
  }
  case PROJECTILE_ENEMY: break; 
  case PROJECTILE_PLAYER: break; 
  }
}

// Exposed functions
u16 _spawn_character(Character2D _character) {
  return spawn_character(_character);
}
bool _add_ability(ability_type _type) {
  ability_package package = get_ability(_type);
  ability_play_system* system = &state->p_player->ability_system;

  switch (_type) {
    case ABILITY_TYPE_FIREBALL: {
      ability_fireball fb = package.data.fireball;
      if (fb.fire_ball_ball_count > MAX_ABILITY_COLLISION_SLOT || fb.fire_ball_ball_count <= 0) {
        TraceLog(LOG_WARNING, "ability_manager::add_ability()::Recieved fireball ability ball count is out of bound");
        return false;
      }
      for (int i=0; i<fb.fire_ball_ball_count; ++i) {
        state->projectiles[i].id = state->projectile_count;
        fb.projectiles[i].id = state->projectile_count;
        state->projectiles[i] = fb.projectiles[i];
        state->projectile_count++;
      }
      system->abilities[ABILITY_TYPE_FIREBALL].data.fireball = fb;
      system->abilities[ABILITY_TYPE_FIREBALL].data.fireball.is_active = true;
      system->abilities[ABILITY_TYPE_FIREBALL].type = ABILITY_TYPE_FIREBALL;

      return true;
    }

    default: {
      TraceLog(LOG_WARNING, "ability_manager::add_ability()::Unknown type of ability");
      return false;
    }
  }

  TraceLog(LOG_WARNING, "ability_manager::add_ability()::Function has been unexpectedly terminated");
  return false;

}
// Exposed functions

// GET / SET
Vector2 _get_player_position(bool centered) {
  return get_player_position(centered);
}
player_state* get_player_state_if_available() {
  if (get_player_state()) {
    return get_player_state();
  }

  return (player_state*) {0};
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
void _set_player_position(Vector2 position) {
  get_player_state()->position = position;
}
// GET / SET

// UPDATE / RENDER
void _update_spawns() {
  update_spawns(get_player_position(true));
}
void _update_player() {
  update_player();
  update_abilities(&state->p_player->ability_system, (Character2D) {.position = state->p_player->position});
}
void _render_player() {
  render_player();
  render_abilities(&state->p_player->ability_system);
}
void _render_spawns() {
  render_spawns();
}
void _render_map() {
  render_tilemap(&state->map);
}
// UPDATE / RENDER

bool game_manager_on_event(u16 code, void *sender, void *listener_inst, event_context context) {
  switch (code) {
  case EVENT_CODE_PAUSE_GAME: {
    state->is_game_paused = true;

    return true;
  }
  case EVENT_CODE_UNPAUSE_GAME: {
    state->is_game_paused = false;

    return true;
  }
  case EVENT_CODE_RELOCATE_SPAWN_COLLISION: {
    for (int i = 1; i <= state->spawn_collision_count; ++i) {
      if (state->spawn_collisions[i].owner_id == context.data.u16[0]) {
        state->spawn_collisions[i].rect.x = context.data.u16[1];
        state->spawn_collisions[i].rect.y = context.data.u16[2];
        return true;
      }
    }
    return false;
  }
  case EVENT_CODE_RELOCATE_PROJECTILE_COLLISION: {
    for (int i = 0; i < state->projectile_count; ++i) {
      if (state->projectiles[i].id == context.data.u16[0]) {
        state->projectiles[i].collision.x = context.data.u16[1];
        state->projectiles[i].collision.y = context.data.u16[2];
        return true;
      }
    }
    return false;
  }
  default: return false; // TODO: Log unknown code
  }

  // TODO: Log unexpected termination
  return false;
}


