#include "game_manager.h"
#include <settings.h>
#include "save_game.h"

#include "core/ftime.h"
#include "core/event.h"
#include "core/fmemory.h"

#include "game/ability.h"
#include "game/player.h"
#include "game/spawn.h"

typedef struct game_manager_system_state {

  ability* player_abilities;
  save_data* game_progression_data;
  camera_metrics* in_camera_metrics;
  player_state* p_player_public;
  Character2D* spawns;
  u32* p_spawn_system_spawn_count;

  worldmap_stage stage;
  player_state player_in_game_default;

  bool is_game_end;
  bool is_game_paused;
  bool game_manager_initialized;
} game_manager_system_state;

static game_manager_system_state *restrict state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

#define SPAWN_TRYING_LIMIT 15

bool game_manager_on_event(u16 code, event_context context);
void game_manager_reinit(void);
void game_manager_refresh_stats(character_stat* stats);
void game_manager_set_stats(character_stat* stat, u32 level);
void populate_map_with_spawns(void);

bool game_manager_initialize(camera_metrics* _camera_metrics) {
  if (state) {
    game_manager_reinit();
    return true;
  }
  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Gama manager state allocation failed");
    return false;
  }
  state->in_camera_metrics = _camera_metrics;

  if (!ability_system_initialize(_camera_metrics, get_app_settings())) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Ability system init returned false");
    return false;
  }
  if (!spawn_system_initialize(_camera_metrics)) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Spawn system init returned false");
    return false;
  }

  event_register(EVENT_CODE_END_GAME, game_manager_on_event);
  event_register(EVENT_CODE_PAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, game_manager_on_event);

  game_manager_reinit();
  return true;
}
void game_manager_reinit(void) {
  save_system_initialize();

  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Returned false");
  }
  state->player_in_game_default = *get_player_state();
  state->p_player_public = get_player_state();
  state->player_abilities = state->p_player_public->ability_system.abilities;
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);
  copy_memory(state->game_progression_data->p_player.stats, state->p_player_public->stats, sizeof(character_stat) * CHARACTER_STATS_MAX);

  parse_or_create_save_data_from_file(SAVE_SLOT_CURRENT_SESSION);
  gm_load_game();
  
  state->spawns = get_spawns();
  state->p_spawn_system_spawn_count = get_spawn_count();
  state->game_manager_initialized = true;
  state->is_game_paused = true;
}

void update_game_manager(void) {
  if (!state) {
    return;
  }

  if (!state->p_player_public->is_dead) {
    const player_update_results pur = update_player();
    if (pur.is_success) { // TODO: Player-Map collision detection system
      /*
      const Rectangle pl_col = state->p_player_public->collision;
      const Rectangle x0 = (Rectangle) {pl_col.x, pl_col.y + pur.move_request.y, pl_col.width, pl_col.height};
      const Rectangle y0 = (Rectangle) {pl_col.x + pur.move_request.x, pl_col.y, pl_col.width, pl_col.height};

      if(!CheckCollisionRecs(state->spawns[0].collision, x0)) {
        move_player(VECTOR2(0, pur.move_request.y));
      }
      if(!CheckCollisionRecs(state->spawns[0].collision, y0)) {
        move_player(VECTOR2(pur.move_request.x, 0));
      } 
      */
      move_player(pur.move_request);
    }
    update_abilities(&state->p_player_public->ability_system);
    update_spawns(get_player_position(true));

    if (*state->p_spawn_system_spawn_count < 50) {
      populate_map_with_spawns();
    }

    for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
      if (state->p_player_public->ability_system.abilities[i].is_active) {
        state->p_player_public->ability_system.abilities[i].ability_play_time += GetFrameTime();
      }
    }
  }
}
void update_game_manager_debug(void) {
  if (!state) {
    return;
  }

  const player_update_results pur = update_player();
  if (pur.is_success) { 
    move_player(pur.move_request);
  }
}
void render_game(void) {
  if (!state) {
    return;
  }
  if (!state->p_player_public->is_dead) {
    render_abilities(&state->p_player_public->ability_system);
    render_player();
    render_spawns();
  }
  else {
    render_player();
    render_spawns();
  }
}

// OPS
/**
 * @brief Start game before, then upgrade the stat
 */
void gm_start_game(worldmap_stage stage) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State returned null");
    return;
  }
  state->stage = stage;
  
  _add_ability(state->p_player_public->starter_ability);
  
  populate_map_with_spawns();

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
    .data.f32[1] = state->p_player_public->exp_perc,
  });
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
    .data.f32[1] = state->p_player_public->health_perc,
  });
}
void gm_reset_game(void) {
  clean_up_spawn_state();

  *state->p_player_public = state->player_in_game_default;
  state->is_game_end = false;
  state->is_game_paused = true;
  state->stage = (worldmap_stage){0};
}
void gm_save_game(void) {
  state->game_progression_data->p_player = state->player_in_game_default;
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void gm_load_game(void) {
  copy_memory(state->player_in_game_default.stats, state->game_progression_data->p_player.stats, sizeof(character_stat) * CHARACTER_STATS_MAX);
  refresh_player_stats();
}
void damage_any_spawn(Character2D *projectile) {
  if (!projectile) {
    TraceLog(LOG_ERROR, 
    "game_manager::damage_any_spawn()::Recieved actor was NULL");
    return;
  }
  
  for (u16 i = 0; i <= *state->p_spawn_system_spawn_count; ++i) {
    if (!state->spawns[i].is_dead){
      if (CheckCollisionRecs(state->spawns[i].collision, projectile->collision)) {
        damage_spawn(state->spawns[i].character_id, projectile->damage);
      }
    }
  }
}
void damage_any_collider_by_type(Character2D from_actor, actor_type to_type) {
  if (!from_actor.initialized) {
    TraceLog(LOG_ERROR, "game_manager::damage_any_collider_by_type()::Recieved actor was NULL");
    return;
  }
  switch (to_type) {
  case ACTOR_TYPE_SPAWN: {
    for (u32 i = 0; i < *state->p_spawn_system_spawn_count; ++i) {
      if (!state->spawns[i].is_dead){
        if (CheckCollisionRecs(state->spawns[i].collision, from_actor.collision)) {
          damage_spawn(state->spawns[i].character_id, from_actor.damage);
        }
      }
    }
    break;
  }
  case ACTOR_TYPE_PLAYER: {
    if (CheckCollisionRecs(from_actor.collision, state->p_player_public->collision)) {
      event_fire(EVENT_CODE_PLAYER_TAKE_DAMAGE, (event_context) { .data.u8[0] = from_actor.damage});
      return;
    }
    break;
  }
  case ACTOR_TYPE_PROJECTILE_SPAWN: break; 
  case ACTOR_TYPE_PROJECTILE_PLAYER: break; 
  }
}
void populate_map_with_spawns(void) {
  if (!state->spawns) {
    TraceLog(LOG_ERROR, "game_manager::populate_map_with_spawns()::Spawn accessor is not valid");
    return;
  }

  u32 spawn_trying_limit = SPAWN_TRYING_LIMIT;
  for (u16 i = 0; i < MAX_SPAWN_COUNT && (spawn_trying_limit <= SPAWN_TRYING_LIMIT && spawn_trying_limit != 0); ) 
  {
    Vector2 position = (Vector2) {
      get_random((i32)state->stage.spawning_areas[0].x, (i32)state->stage.spawning_areas[0].x + state->stage.spawning_areas[0].width),
      get_random((i32)state->stage.spawning_areas[0].y, (i32)state->stage.spawning_areas[0].y + state->stage.spawning_areas[0].height)
    };
    spawn_character((Character2D) {
      .character_id = 0,
      .buffer.u16[0] = get_random(SPAWN_TYPE_UNDEFINED+1, SPAWN_TYPE_MAX-1),
      .buffer.u16[1] = state->p_player_public->level,
      .buffer.u16[2] = get_random(0, 100),
      .scale = 1,
      .position = position,
      .w_direction = WORLD_DIRECTION_LEFT,
      .type = ACTOR_TYPE_SPAWN,
      .rotation = 0,
      .health = 0,
      .damage = 0,
      .speed = 1,
      .is_damagable = true,
      .initialized = false
    }) ? ++i : --spawn_trying_limit;
  }
}
void upgrade_player_stat(character_stat* stat) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::State is not valid");
    return;
  }
  if (!state->p_player_public->is_initialized) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::state->p_player_public returned null");
    return;
  }
  if (!stat) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::stat is not valid");
    return;
  }
  if (stat->id >= CHARACTER_STATS_MAX || stat->id <= CHARACTER_STATS_UNDEFINED) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::stat is out of bound");
    stat->level = 1;
  }
  if (stat->level >= MAX_PASSIVE_UPGRADE_TIER) {
    return;
  } 
  stat->level++;

  game_manager_set_stats(stat, stat->level);
}
void upgrade_default_player_stat(character_stats stat) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_default_player_stat()::State is not valid");
    return;
  }

  upgrade_player_stat(&state->player_in_game_default.stats[stat]);
}
void game_manager_refresh_stats(character_stat* stats) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State returned null");
    return;
  }
  if (!state->p_player_public->is_initialized) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::stat returned null");
    return;
  }

  for (int i=0; i<CHARACTER_STATS_MAX; ++i) {
    game_manager_set_stats(&stats[i], stats[i].level);
  }
}
void refresh_player_stats() {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State returned null");
    return;
  }
  if (!state->p_player_public->is_initialized) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::stat returned null");
    return;
  }

  game_manager_refresh_stats(state->p_player_public->stats);
}
void game_manager_set_stats(character_stat* stat, u32 level) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::State is not valid");
    return;
  }
  if (!stat) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::stat is not valid");
    return;
  }
  if (stat->id >= CHARACTER_STATS_MAX || stat->id <= CHARACTER_STATS_UNDEFINED) {
    return;
  }
  if (level > MAX_PASSIVE_UPGRADE_TIER) {
    return;
  }

  u32 next_curve_value = level_curve[level];
  stat->upgrade_cost = next_curve_value;
  switch (stat->id) {
    case CHARACTER_STATS_HEALTH:{
      const u32 value = next_curve_value * 2.f;

      stat->buffer.u32[0] = value;
      state->p_player_public->health_max = value;
      state->p_player_public->health_current = state->p_player_public->health_max;
      break; 
    }
    case CHARACTER_STATS_HP_REGEN:{ 
      const u32 value = next_curve_value / 1000.f;

      stat->buffer.u32[0] = value;
      state->p_player_public->health_regen = value;
      break;
    }
    case CHARACTER_STATS_MOVE_SPEED:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[0] = value;
      state->p_player_public->move_speed_multiply = value;
      break;
    }
    case CHARACTER_STATS_AOE:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[0] = value;
      state->p_player_public->damage_area_multiply = value;
      break;
    }
    case CHARACTER_STATS_DAMAGE:{
      const u32 value = next_curve_value * .1f;

      stat->buffer.u32[0] = value;
      state->p_player_public->damage = value;
      break;
    }
    case CHARACTER_STATS_ABILITY_CD:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[0] = value;
      state->p_player_public->cooldown_multiply = value;
      break;
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH:{
      const u16 value = stat->level;

      stat->buffer.u16[0] = value;
      state->p_player_public->projectile_amouth = value;
      break;
    }
    case CHARACTER_STATS_EXP_GAIN:{
      const u32 value = next_curve_value / 1000.f;

      stat->buffer.u32[0] = value;
      state->p_player_public->exp_gain_multiply = value;
      break;
    }
  default:{
    TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::Unsuppported stat id");
    break;
  }
  }
}
void currency_souls_add(i32 value) {
  state->game_progression_data->currency_souls_player_have += value;
}
// OPS

// GET / SET
Character2D* get_spawn_info(u16 spawn_id) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::get_spawn_info()::State is not valid");
    return 0;
  }
  if (spawn_id > *state->p_spawn_system_spawn_count) {
    TraceLog(LOG_ERROR, "game_manager::get_spawn_info()::spawn id is out of bound");
    return 0;
  }
  if (!state->spawns[spawn_id].initialized) {
    return 0;
  }

  return &state->spawns[spawn_id];
}
u32 get_currency_souls(void) {
  return state->game_progression_data->currency_souls_player_have;
}
bool get_b_player_have_upgrade_points(void) {
  return state->p_player_public->is_player_have_ability_upgrade_points;
}
void set_player_have_ability_upgrade_points(bool _b) {
  state->p_player_public->is_player_have_ability_upgrade_points = _b;
}
ability* get_player_ability(ability_type type) {
  return &state->p_player_public->ability_system.abilities[type];
}
character_stat* get_player_in_game_stat(character_stats stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return 0;
  }
  
  return &state->p_player_public->stats[stat];
}
character_stat* get_player_default_stat(character_stats stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return 0;
  }
  
  return &state->player_in_game_default.stats[stat];
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
bool get_is_game_end(void) {
  return state->is_game_end;
}
void set_is_game_end(bool _is_game_end) { 
  state->is_game_end = _is_game_end; 
}
void toggle_is_game_end(void) {
  state->is_game_end = !state->is_game_end;
}
u16 get_remaining_enemies(void) {
  return *state->p_spawn_system_spawn_count;
}
void set_currency_souls(i32 value) {
  state->game_progression_data->currency_souls_player_have = value;
}
Vector2 gm_get_mouse_pos_world(void) {
  return GetScreenToWorld2D((Vector2) {
    GetMousePosition().x * get_app_settings()->scale_ratio.x,
    GetMousePosition().y * get_app_settings()->scale_ratio.y
    }, 
    state->in_camera_metrics->handle
  );
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
  ability_play_system* system = &state->p_player_public->ability_system;
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  abl.p_owner = state->p_player_public;
  abl.is_initialized = true;

  abl.proj_count += state->p_player_public->projectile_amouth;

  for (i32 i=0; i < abl.proj_count; ++i) {
    abl.projectiles[i].is_active = true;
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
  ability_play_system* system = &state->p_player_public->ability_system;
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  u16 _proj_count = abl->proj_count;
  upgrade_ability(abl);
  for (int i=_proj_count; i<abl->proj_count; ++i) {
    abl->projectiles[i].is_active = true;
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
  state->p_player_public->position = position;
  state->p_player_public->collision.x = position.x;
  state->p_player_public->collision.y = position.y;
}
Vector2 _get_player_position(bool centered) {
  return get_player_position(centered);
}
player_state* _get_player_in_game_state(void) {
  return state->p_player_public;
}
player_state* _get_player_default_state(void) {
  return &state->player_in_game_default;
}
// Exposed functions


bool game_manager_on_event(u16 code, event_context context) {
  switch (code) {
    case EVENT_CODE_END_GAME: {
      state->is_game_end = true;
      return true;
    }
    case EVENT_CODE_PAUSE_GAME: {
      state->is_game_paused = true;
      return true;
    }
    case EVENT_CODE_UNPAUSE_GAME: {
      state->is_game_paused = false;
      return true;
    }
    case EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE: {
      Character2D from_actor = (Character2D) {
        .collision = (Rectangle) {
          context.data.i16[0], context.data.i16[1], 
          context.data.i16[2], context.data.i16[3]
        },
        .damage = context.data.i16[4],
        .initialized = true
      };
      damage_any_collider_by_type(from_actor, ACTOR_TYPE_PLAYER);

      return true;
    }
    case EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE: {
      Character2D from_actor = (Character2D) {
        .collision = (Rectangle) {
          context.data.i16[0], context.data.i16[1], 
          context.data.i16[2], context.data.i16[3]
        },
        .damage = context.data.i16[4],
        .initialized = true
      };
      damage_any_collider_by_type(from_actor, ACTOR_TYPE_SPAWN);

      return true;
    }
    default: {
      TraceLog(LOG_WARNING, "game_engine::game_manager_on_event()::Unsuppported code.");
      return false;
    }
  }

  TraceLog(LOG_WARNING, "game_engine::game_manager_on_event()::Fire event ended unexpectedly");
  return false;
}

#undef SPAWN_TRYING_LIMIT
