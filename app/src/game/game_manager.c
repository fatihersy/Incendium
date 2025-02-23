#include "game_manager.h"
#include <settings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/tilemap.h"
#include "game/ability.h"
#include "game/player.h"
#include "game/spawn.h"

typedef struct game_manager_system_state {
  rectangle_collision spawn_collisions[MAX_SPAWN_COUNT];
  projectile projectiles[MAX_ABILITY_PROJECTILE_SLOT * MAX_ABILITY_SLOT];
  tilemap map;
  tilemap_stringtify_package map_stringtify;
  u16 spawn_collision_count;
  u16 projectile_count;
  player_state* p_player;


  bool is_game_paused;
  bool game_manager_initialized;
} game_manager_system_state;

static game_manager_system_state *restrict state;

void update_collisions(void);

bool game_manager_on_event(u16 code, event_context context);

bool game_manager_initialize(camera_metrics* _camera_metrics) {
  if (state) return false;

  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  state->is_game_paused = true;

  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "player_system_initialize() failed");
    return false;
  }
  if (!ability_system_initialize(_camera_metrics, get_app_settings())) {
    TraceLog(LOG_ERROR, "ability_system_initialize() failed");
    return false;
  }
  state->p_player = get_player_state();
  _add_ability(state->p_player->starter_ability);

  if (!spawn_system_initialize()) {
    TraceLog(LOG_ERROR, "spawn_system_initialize() failed");
    return false;
  }

  for (int i=0; i<MAX_TILEMAP_LAYERS; ++i) {
    copy_memory(state->map.filename[i], TextFormat("map_layer%d.txt", i), sizeof(i8) * MAX_TILEMAP_FILENAME_LEN);
  }
  create_tilemap(TILESHEET_TYPE_MAP, (Vector2) {0, 0}, 100, 16*3, &state->map);
  if(!state->map.is_initialized) {
    TraceLog(LOG_WARNING, "WARNING::scene_in_game_edit::initialize_scene_in_game_edit()::tilemap initialization failed");
  }
  load_map_data(&state->map, &state->map_stringtify);
  if (!state->map_stringtify.is_success) {
    TraceLog(LOG_ERROR, "game_manager_initialize::game manager unable to load map");
    return false;
  }

  event_register(EVENT_CODE_PAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_RELOCATE_SPAWN_COLLISION, game_manager_on_event);
  event_register(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, game_manager_on_event);
  
  state->game_manager_initialized = true;
  return true;
}

void update_game_manager(void) {
  if (!state) {
    return;
  }
  update_collisions();
  update_player();
  update_abilities(&state->p_player->ability_system);
  update_spawns(get_player_position(true));

}
void update_collisions(void) {
  for (int i=0; i<state->projectile_count; ++i) { for (u16 j = 1; j <= state->spawn_collision_count; ++j) {
      if (state->spawn_collisions[j].is_active){ 
        if (CheckCollisionRecs(state->spawn_collisions[j].rect, state->projectiles[i].collision)) {
          u16 result = damage_spawn(state->spawn_collisions[j].owner_id, state->projectiles[i].damage);
          if (result <= 0 || result >= 100) {
            state->spawn_collisions[j].is_active = false;
          }
        }
      }
    }
  }

  for (int i=0; i<state->spawn_collision_count; ++i) {if (state->spawn_collisions[i].is_active){ 
    Character2D* spawn = &get_spawn_system()->spawns[state->spawn_collisions[i].owner_id];
    damage_any_collider_by_type(spawn, PLAYER);
  }}
}
void render_game(void) {
  render_tilemap(&state->map);
  render_player();
  render_spawns();
  render_abilities(&state->p_player->ability_system);
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
        damage_spawn(state->spawn_collisions[i].owner_id, projectile->damage);
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
          damage_spawn(state->spawn_collisions[i].owner_id, from_actor->damage);
        }
      }
    }
    break;
  }
  case PLAYER: {
    if (CheckCollisionRecs(from_actor->collision, state->p_player->collision)) {
      event_fire(EVENT_CODE_PLAYER_TAKE_DAMAGE, (event_context) { .data.u8[0] = from_actor->damage});
      return;
    }
    break;
  }
  case PROJECTILE_ENEMY: break; 
  case PROJECTILE_PLAYER: break; 
  }
}
void add_collision(rectangle_collision rect_col) {
  state->spawn_collision_count++;
  state->spawn_collisions[state->spawn_collision_count] = rect_col;
  state->spawn_collisions[state->spawn_collision_count].is_active = true;
}

// GET / SET
player_state* get_player_state_if_available(void) {
  if (get_player_state()) {
    return get_player_state();
  }

  return (player_state*) {0};
}
bool get_is_game_paused(void) {
  return state->is_game_paused;
}
void set_is_game_paused(bool _is_game_paused) {
  state->is_game_paused = _is_game_paused;
}
void toggle_is_game_paused(void) {
  state->is_game_paused = !state->is_game_paused;
}
// GET / SET

// Exposed functions
u16 _spawn_character(Character2D _character) {
  return spawn_character(_character);
}
ability _get_ability(ability_type type) {
  if (type <= ABILITY_TYPE_UNDEFINED || type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_get_ability()::Ability type is out of bound");
    return (ability) {0};
  }
  return get_ability(type);
}
bool _add_ability(ability_type _type) {
  if (_type <= ABILITY_TYPE_UNDEFINED || _type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_add_ability()::Ability type is out of bound");
    return false;
  }
  ability abl = get_ability(_type);
  ability_play_system* system = &state->p_player->ability_system;
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  abl.p_owner = state->p_player;
  abl.is_initialized = true;

  for (int i=0; i < abl.proj_count; ++i) {
    abl.projectiles[i].id = state->projectile_count;
    state->projectiles[state->projectile_count] = abl.projectiles[i];
    state->projectile_count++;
  }
  abl.is_active = true;

  system->abilities[_type] = abl;
  
  return true;
}
bool _upgrade_ability(ability* abl) {
  if (!abl->is_initialized) {
    TraceLog(LOG_WARNING, "game_manager::_upgrade_ability::Recieved ability has not initialized yet");
    return false;
  }
  ability_play_system* system = &state->p_player->ability_system;
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  u16 _proj_count = abl->proj_count;
  upgrade_ability(abl);
  for (int i=_proj_count; i<abl->proj_count; ++i) {
    abl->projectiles[i].id = state->projectile_count;
    state->projectiles[state->projectile_count] = abl->projectiles[i];
    state->projectile_count++;
  }

  return true;
}
ability _get_next_level(ability abl) {
  if (!abl.is_initialized || abl.type <= ABILITY_TYPE_UNDEFINED || abl.type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "game_manager::_get_next_level()::Recieved ability has not initialized yet");
    return (ability) {0};
  
  }
  return get_next_level(abl);
}
void _set_player_position(Vector2 position) {
  get_player_state()->position = position;
}
Vector2 _get_player_position(bool centered) {
  return get_player_position(centered);
}
// Exposed functions

bool game_manager_on_event(u16 code, event_context context) {
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


