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

  save_data* game_progression_data;
  camera_metrics* in_camera_metrics;
  Character2D* in_spawns;
  u32* p_spawn_system_spawn_count;
  
  player_state* player_state_dynamic;
  worldmap_stage stage;
  player_state player_state_static;
  Vector2 mouse_pos_world;
  u16 remaining_waves_to_spawn_boss;

  bool is_game_end;
  bool is_game_paused;
  bool game_manager_initialized;
} game_manager_system_state;

static game_manager_system_state * state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

#define SPAWN_TRYING_LIMIT 15
#define WAVE_COUNT_TO_SPAWN_BOSS 3

bool game_manager_on_event(u16 code, event_context context);
void game_manager_reinit(void);
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

  if (!save_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Save system init returned false");
    return false;
  }
  if (!ability_system_initialize(_camera_metrics, get_app_settings())) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Ability system init returned false");
    return false;
  }
  if (!spawn_system_initialize(_camera_metrics)) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Spawn system init returned false");
    return false;
  }
  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Returned false");
    return false;
  }

  state->player_state_dynamic = get_player_state();
  state->in_spawns = get_spawns();
  state->p_spawn_system_spawn_count = get_spawn_count();
  state->in_camera_metrics = _camera_metrics;

  event_register(EVENT_CODE_END_GAME, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, game_manager_on_event);

  game_manager_reinit();
  return true;
}
void game_manager_reinit(void) {
  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Returned false");
    return;
  }
  state->player_state_static = *get_player_state();
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);
  copy_memory(state->game_progression_data->p_player.stats, state->player_state_dynamic->stats, sizeof(character_stat) * CHARACTER_STATS_MAX); // INFO: To get default stat values

  parse_or_create_save_data_from_file(SAVE_SLOT_CURRENT_SESSION);
  gm_load_game();

  state->remaining_waves_to_spawn_boss = WAVE_COUNT_TO_SPAWN_BOSS;
  state->game_manager_initialized = true;
  state->is_game_paused = true;
}

void update_game_manager(void) {
  if (!state) {
    return;
  }

  if (!state->player_state_dynamic->is_dead) {
    state->mouse_pos_world = GetScreenToWorld2D(Vector2{
      GetMousePosition().x * get_app_settings()->scale_ratio.at(0),
      GetMousePosition().y * get_app_settings()->scale_ratio.at(1)
      }, 
      state->in_camera_metrics->handle
    );
    const player_update_results pur = update_player();
    if (pur.is_success) { // TODO: Player-Map collision detection system
      /*
      const Rectangle pl_col = state->p_player_public->collision;
      const Rectangle x0 = (Rectangle) {pl_col.x, pl_col.y + pur.move_request.y, pl_col.width, pl_col.height};
      const Rectangle y0 = (Rectangle) {pl_col.x + pur.move_request.x, pl_col.y, pl_col.width, pl_col.height};

      if(!CheckCollisionRecs(state->in_spawns[0].collision, x0)) {
        move_player(VECTOR2(0, pur.move_request.y));
      }
      if(!CheckCollisionRecs(state->in_spawns[0].collision, y0)) {
        move_player(VECTOR2(pur.move_request.x, 0));
      } 
      */
      move_player(pur.move_request);
    }
    update_abilities(&state->player_state_dynamic->ability_system);
    update_spawns(get_player_position(true));

    if (*state->p_spawn_system_spawn_count < 50) {
      populate_map_with_spawns();
    }

    for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
      if (state->player_state_dynamic->ability_system.abilities[i].is_active) {
        state->player_state_dynamic->ability_system.abilities[i].ability_play_time += GetFrameTime();
      }
    }
  }
}
void update_game_manager_debug(void) {
  if (!state) {
    return;
  }
  state->mouse_pos_world = GetScreenToWorld2D(Vector2{
    GetMousePosition().x * get_app_settings()->scale_ratio.at(0),
    GetMousePosition().y * get_app_settings()->scale_ratio.at(1)
    }, 
    state->in_camera_metrics->handle
  );

  const player_update_results pur = update_player();
  if (pur.is_success) { 
    move_player(pur.move_request);
  }
}
void render_game(void) {
  if (!state) {
    return;
  }
  if (!state->player_state_dynamic->is_dead) {
    render_abilities(&state->player_state_dynamic->ability_system);
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
  
  _add_ability(state->player_state_dynamic->starter_ability);
  refresh_player_stats(true, false);
  
  populate_map_with_spawns();

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->player_state_dynamic->exp_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)state->player_state_dynamic->health_perc));
}
void gm_reset_game(void) {
  clean_up_spawn_state();

  copy_memory(state->player_state_dynamic, &state->player_state_static, sizeof(player_state));
  state->is_game_end = false;
  state->is_game_paused = true;
  state->stage = worldmap_stage {};
}
void gm_save_game(void) {
  state->game_progression_data->p_player = state->player_state_static;
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void gm_load_game(void) {
  copy_memory(state->player_state_static.stats, state->game_progression_data->p_player.stats, sizeof(character_stat) * CHARACTER_STATS_MAX);
  refresh_player_stats(false, true);
}
void damage_any_spawn(Character2D *projectile) {
  if (!projectile) {
    TraceLog(LOG_ERROR, 
    "game_manager::damage_any_spawn()::Recieved actor was NULL");
    return;
  }
  
  for (u16 i = 0; i <= *state->p_spawn_system_spawn_count; ++i) {
    if (!state->in_spawns[i].is_dead){
      if (CheckCollisionRecs(state->in_spawns[i].collision, projectile->collision)) {
        damage_spawn(state->in_spawns[i].character_id, projectile->damage);
      }
    }
  }
}
void damage_any_collider_by_type(Character2D from_actor, actor_type to_type) {
  if (!from_actor.initialized) {
    TraceLog(LOG_ERROR, "game_manager::damage_any_collider_by_type()::Recieved actor has not initialized");
    return;
  }
  switch (to_type) {
    case ACTOR_TYPE_SPAWN: {
    for (u32 i = 0; i < *state->p_spawn_system_spawn_count; ++i) {
      if (!state->in_spawns[i].is_dead){
        if (CheckCollisionRecs(state->in_spawns[i].collision, from_actor.collision)) {
          damage_spawn(state->in_spawns[i].character_id, from_actor.damage);
        }
      }
    }
    return;
    }
    case ACTOR_TYPE_PLAYER: {
    if (CheckCollisionRecs(from_actor.collision, state->player_state_dynamic->collision)) {
      event_fire(EVENT_CODE_PLAYER_TAKE_DAMAGE, event_context((u8)from_actor.damage));
      return;
    }
    return;
    }
    case ACTOR_TYPE_PROJECTILE_SPAWN: return; 
    case ACTOR_TYPE_PROJECTILE_PLAYER: return;
    default:{
      TraceLog(LOG_WARNING, "game_manager::damage_any_collider_by_type()::Actor type is not supported");
      return;
    }
  }
  TraceLog(LOG_WARNING, "game_manager::damage_any_collider_by_type()::Function terminated unexpectedly");
}
void populate_map_with_spawns(void) {
  if (!state->in_spawns) {
    TraceLog(LOG_ERROR, "game_manager::populate_map_with_spawns()::Spawn accessor is not valid");
    return;
  }

  u32 spawn_trying_limit = SPAWN_TRYING_LIMIT;
  for (u16 i = 0; i < MAX_SPAWN_COUNT && (spawn_trying_limit <= SPAWN_TRYING_LIMIT && spawn_trying_limit != 0); ) 
  {
    Vector2 position = {
      (f32) get_random((i32)state->stage.spawning_areas[0].x, (i32)state->stage.spawning_areas[0].x + state->stage.spawning_areas[0].width),
      (f32) get_random((i32)state->stage.spawning_areas[0].y, (i32)state->stage.spawning_areas[0].y + state->stage.spawning_areas[0].height)
    };
    spawn_character(Character2D(
      (u16) get_random(SPAWN_TYPE_UNDEFINED+1, SPAWN_TYPE_MAX-2), 
      (u16) state->player_state_dynamic->level, 
      (u16) get_random(0, 100), position, 1.f)
    ) ? ++i : --spawn_trying_limit;
  }

  if ((state->remaining_waves_to_spawn_boss <= 0 || state->remaining_waves_to_spawn_boss > WAVE_COUNT_TO_SPAWN_BOSS) && *state->p_spawn_system_spawn_count < MAX_SPAWN_COUNT) {
    for (u16 i = 0; i < SPAWN_TRYING_LIMIT; i++) {
      Vector2 position = {
        (f32) get_random((i32)state->stage.spawning_areas[0].x, (i32)state->stage.spawning_areas[0].x + state->stage.spawning_areas[0].width),
        (f32) get_random((i32)state->stage.spawning_areas[0].y, (i32)state->stage.spawning_areas[0].y + state->stage.spawning_areas[0].height)
      };
      if(spawn_character(Character2D(SPAWN_TYPE_RED, (u16) state->player_state_dynamic->level + 5, 150, position, 1.f))) {
        state->remaining_waves_to_spawn_boss = WAVE_COUNT_TO_SPAWN_BOSS;
        TraceLog(LOG_INFO, "Boss spawned!");
        break;
      }
    }
  }
  else if(state->remaining_waves_to_spawn_boss > 0) {
    state->remaining_waves_to_spawn_boss--;
    TraceLog(LOG_INFO, "To boss spawn:%d", state->remaining_waves_to_spawn_boss);
  }
}
void upgrade_dynamic_player_stat(character_stats stat_id, u16 level) {
  if (!state || !state->player_state_dynamic->is_initialized) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::State is not valid");
    return;
  }
  if ( stat_id >= CHARACTER_STATS_MAX || stat_id <= CHARACTER_STATS_UNDEFINED) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::stat id out of bound");
    return;
  }
  character_stat* stat = &state->player_state_dynamic->stats[stat_id];
  if (level > 0 && level <= MAX_PASSIVE_UPGRADE_TIER) stat->level = level;
  else {
    if (stat->level > MAX_PASSIVE_UPGRADE_TIER || stat->level < 0) {
      TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::stat level out of bound");
      return;
    }
    stat->level++;
  }
  game_manager_set_stats(stat, stat->level);

  player_state* player = state->player_state_dynamic;

  switch (stat->id) {
    case CHARACTER_STATS_HEALTH: { 
      player->health_max = stat->buffer.u32[0];
      player->health_current = player->health_max;
      return; 
    }
    case CHARACTER_STATS_HP_REGEN: {
      player->health_regen = stat->buffer.u32[0];
      return; 
    }
    case CHARACTER_STATS_MOVE_SPEED: { 
      player->move_speed_multiply = stat->buffer.f32[0];
      return; 
    }
    case CHARACTER_STATS_AOE: {
      player->damage_area_multiply = stat->buffer.f32[0];
      return; 
    }
    case CHARACTER_STATS_DAMAGE: {
      player->damage = stat->buffer.u32[0];
      return; 
    }
    case CHARACTER_STATS_ABILITY_CD: {
      player->cooldown_multiply = stat->buffer.f32[0];
      return; 
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH: {
      player->projectile_amouth = stat->buffer.u16[0];
      return; 
    }
    case CHARACTER_STATS_EXP_GAIN: {
      player->exp_gain_multiply = stat->buffer.f32[0];
      return;
    }
    default: {
      TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::Recieved ");
      return;
    }
  }
  TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::Function terminated unexpectedly");
}
void upgrade_static_player_stat(character_stats stat_id, u16 level) {
  if (!state || !state->player_state_static.is_initialized) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_static_player_stat()::State is not valid");
    return;
  }
  if ( stat_id >= CHARACTER_STATS_MAX || stat_id <= CHARACTER_STATS_UNDEFINED) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_static_player_stat()::stat id out of bound");
    return;
  }
  character_stat* stat = &state->player_state_static.stats[stat_id];

  if (level > 0 && level <= MAX_PASSIVE_UPGRADE_TIER) stat->level = level;
  else {
    if (stat->level > MAX_PASSIVE_UPGRADE_TIER || stat->level < 0) {
      TraceLog(LOG_ERROR, "game_manager::upgrade_static_player_stat()::stat level out of bound");
      return;
    }
    stat->level++;
  }
  game_manager_set_stats(stat, stat->level);

  player_state* player = &state->player_state_static;

  switch (stat->id) {
    case CHARACTER_STATS_HEALTH: { 
      player->health_max = stat->buffer.u32[0];
      player->health_current = player->health_max;
      return; 
    }
    case CHARACTER_STATS_HP_REGEN: {
      player->health_regen = stat->buffer.u32[0];
      return; 
    }
    case CHARACTER_STATS_MOVE_SPEED: { 
      player->move_speed_multiply = stat->buffer.f32[0];
      return; 
    }
    case CHARACTER_STATS_AOE: {
      player->damage_area_multiply = stat->buffer.f32[0];
      return; 
    }
    case CHARACTER_STATS_DAMAGE: {
      player->damage = stat->buffer.u32[0];
      return; 
    }
    case CHARACTER_STATS_ABILITY_CD: {
      player->cooldown_multiply = stat->buffer.f32[0];
      return; 
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH: {
      player->projectile_amouth = stat->buffer.u16[0];
      return; 
    }
    case CHARACTER_STATS_EXP_GAIN: {
      player->exp_gain_multiply = stat->buffer.f32[0];
      return;
    }
    default: {
      TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::Recieved ");
      return;
    }
  }
  TraceLog(LOG_ERROR, "game_manager::upgrade_static_player_stat()::Function terminated unexpectedly");
}
/**
 * @brief default player is dynamic
 */
void refresh_player_stats(bool refresh_dynamic_state, bool refresh_static_state) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::refresh_player_stats()::State returned null");
    return;
  }
  if (refresh_dynamic_state) {
    if (state->player_state_dynamic != nullptr && !state->player_state_dynamic->is_initialized) {
      TraceLog(LOG_ERROR, "game_manager::refresh_player_stats()::stat returned null");
      return;
    }
    for (size_t stat = CHARACTER_STATS_UNDEFINED+1; stat < CHARACTER_STATS_MAX; stat++) {
      upgrade_dynamic_player_stat(static_cast<character_stats>(stat), state->player_state_dynamic->stats[stat].level);
    }
    return;
  }
  if(refresh_static_state) {
    if (!state->player_state_static.is_initialized) {
      TraceLog(LOG_ERROR, "game_manager::refresh_player_stats()::stat returned null");
      return;
    }
    for (size_t stat = CHARACTER_STATS_UNDEFINED+1; stat < CHARACTER_STATS_MAX; stat++) {
      upgrade_static_player_stat(static_cast<character_stats>(stat), state->player_state_static.stats[stat].level);
    }
    return;
  }
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
  if (level != 0 && level > MAX_PASSIVE_UPGRADE_TIER) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::stat level is not valid");
    return;
  }

  u32 next_curve_value = level_curve[level];
  stat->upgrade_cost = next_curve_value;
  switch (stat->id) {
    case CHARACTER_STATS_HEALTH:{
      const u32 value = next_curve_value * 2;

      stat->buffer.u32[0] = value;
      break;
    }
    case CHARACTER_STATS_HP_REGEN:{ 
      const u32 value = next_curve_value / 1000.f;

      stat->buffer.u32[0] = value;
      break;
    }
    case CHARACTER_STATS_MOVE_SPEED:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[0] = value;
      break;
    }
    case CHARACTER_STATS_AOE:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[0] = value;
      break;
    }
    case CHARACTER_STATS_DAMAGE:{
      const u32 value = next_curve_value * .1f;

      stat->buffer.u32[0] = value;
      break;
    }
    case CHARACTER_STATS_ABILITY_CD:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[0] = value;
      break;
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH:{
      const u16 value = stat->level;

      stat->buffer.u16[0] = value;
      break;
    }
    case CHARACTER_STATS_EXP_GAIN:{
      const u32 value = next_curve_value / 1000.f;

      stat->buffer.u32[0] = value;
      break;
    }
  default:{
    TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::Unsuppported stat id");
    break;
  }
  }
}
void upgrade_stat_pseudo(character_stat* stat) {
  if ( stat->id >= CHARACTER_STATS_MAX || stat->id <= CHARACTER_STATS_UNDEFINED) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_stat_pseudo()::stat id out of bound");
    return;
  }
  if (stat->level < 0 && stat->level >= MAX_PASSIVE_UPGRADE_TIER){
    TraceLog(LOG_ERROR, "game_manager::upgrade_stat_pseudo()::stat level out of bound");
    return;
  }
  stat->level++;

  game_manager_set_stats(stat, stat->level);
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
  if (!state->in_spawns[spawn_id].initialized) {
    return 0;
  }

  return &state->in_spawns[spawn_id];
}
u32 get_currency_souls(void) {
  return state->game_progression_data->currency_souls_player_have;
}
bool get_b_player_have_upgrade_points(void) {
  return state->player_state_dynamic->is_player_have_ability_upgrade_points;
}
void set_dynamic_player_have_ability_upgrade_points(bool _b) {
  state->player_state_dynamic->is_player_have_ability_upgrade_points = _b;
}
ability* get_dynamic_player_state_ability(ability_type type) {
  return &state->player_state_dynamic->ability_system.abilities[type];
}
character_stat* get_dynamic_player_state_stat(character_stats stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return 0;
  }
  
  return &state->player_state_dynamic->stats[stat];
}
character_stat* get_static_player_state_stat(character_stats stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return 0;
  }
  
  return &state->player_state_static.stats[stat];
}
bool* get_is_game_paused(void) {
  return __builtin_addressof(state->is_game_paused);
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
Vector2* gm_get_mouse_pos_world(void) {
  return &state->mouse_pos_world;
}
// GET / SET

// Exposed functions
u16 _spawn_character(Character2D _character) {
  return spawn_character(_character);
}
ability _get_ability(ability_type type) {
  if (type <= ABILITY_TYPE_UNDEFINED || type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_get_ability()::Ability type is out of bound");
    return ability {};
  }
  return get_ability(type);
}
bool _add_ability(ability_type _type) {
  if (_type <= ABILITY_TYPE_UNDEFINED || _type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_add_ability()::Ability type is out of bound");
    return false;
  }
  ability abl = get_ability(_type);
  ability_play_system* system = &state->player_state_dynamic->ability_system;
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  abl.p_owner = state->player_state_dynamic;
  abl.is_initialized = true;

  abl.proj_count += state->player_state_dynamic->projectile_amouth;

  for (i32 i=0; i < abl.proj_count; ++i) {
    abl.projectiles.at(i).is_active = true;
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
  ability_play_system* system = &state->player_state_dynamic->ability_system;
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  u16 _proj_count = abl->proj_count;
  upgrade_ability(abl);
  for (int i=_proj_count; i<abl->proj_count; ++i) {
    abl->projectiles.at(i).is_active = true;
  }

  return true;
}
ability _get_next_level(ability abl) {
  if (!abl.is_initialized || abl.type <= ABILITY_TYPE_UNDEFINED || abl.type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "game_manager::_get_next_level()::Recieved ability has not initialized yet");
    return ability{};
  
  }
  return get_next_level(abl);
}
void _set_player_position(Vector2 position) {
  state->player_state_dynamic->position = position;
  state->player_state_dynamic->collision.x = position.x;
  state->player_state_dynamic->collision.y = position.y;
}
Vector2 _get_player_position(bool centered) {
  return get_player_position(centered);
}
player_state* _get_dynamic_player_state(void) {
  return state->player_state_dynamic;
}
// Exposed functions


bool game_manager_on_event(u16 code, event_context context) {
  switch (code) {
    case EVENT_CODE_END_GAME: {
      state->is_game_end = true;
      return true;
    }
    case EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE: {
      Character2D from_actor = Character2D(Rectangle {
          (f32) context.data.i16[0], (f32) context.data.i16[1], 
          (f32) context.data.i16[2], (f32) context.data.i16[3]
        },
        (i32)context.data.i16[4]
      );
      damage_any_collider_by_type(from_actor, ACTOR_TYPE_PLAYER);

      return true;
    }
    case EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE: {
      Character2D from_actor = Character2D(Rectangle {
          (f32) context.data.i16[0], (f32) context.data.i16[1], 
          (f32) context.data.i16[2], (f32) context.data.i16[3]
        },
        (i32)context.data.i16[4]
      );
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
