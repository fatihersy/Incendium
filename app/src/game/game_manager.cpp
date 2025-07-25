#include "game_manager.h"
#include <settings.h>
#include "save_game.h"

#include "core/ftime.h"
#include "core/event.h"
#include "core/fmemory.h"
#include "core/fmath.h"

#include "game/abilities/ability_manager.h"
#include "game/player.h"
#include "game/spawn.h"

typedef struct game_manager_system_state {
  save_data* game_progression_data;
  const camera_metrics* in_camera_metrics;
  const app_settings* in_app_settings;
  tilemap ** in_active_map;

  worldmap_stage stage;
  ingame_phases ingame_phase;
  player_state player_state_static;
  ingame_info game_info;
  Vector2 mouse_pos_world;
  Vector2 mouse_pos_screen;

  bool show_pause_menu;
  bool is_game_end;
  bool is_game_paused;
  bool game_manager_initialized;
} game_manager_system_state;

static game_manager_system_state * state;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

#define SPAWN_TRYING_LIMIT 15
#define GET_BOSS_LEVEL(STAGE_LEVEL, VAL) (STAGE_LEVEL + (STAGE_LEVEL * state->stage.boss_scale))
#define GET_BOSS_SCALE(VAL) (100 + (100 * VAL))
#define GET_BOSS_SPEED(VAL) (1.1f * VAL)

bool game_manager_on_event(i32 code, event_context context);
bool game_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings,tilemap ** const in_active_map_ptr);
void game_manager_set_stats(character_stat* stat, u32 level);
void populate_map_with_spawns(i32 min_count);
bool spawn_boss(void);
void generate_in_game_info(void);

bool game_manager_initialize(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (state) {
    return game_manager_reinit(in_camera_metrics, in_app_settings, in_active_map_ptr);
  }
  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Gama manager state allocation failed");
    return false;
  }
  state->game_info = ingame_info();

  if (!save_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Save system init returned false");
    return false;
  }
  if (!ability_system_initialize(in_camera_metrics, get_app_settings(), __builtin_addressof(state->game_info))) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Ability system init returned false");
    return false;
  }
  if (!spawn_system_initialize(in_camera_metrics)) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Spawn system init returned false");
    return false;
  }
  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Returned false");
    return false;
  }
  state->player_state_static = *get_player_state();
  
  state->game_info.player_state_dynamic          = get_player_state();
  state->game_info.in_spawns                     = get_spawns();
  state->game_info.mouse_pos_world               = __builtin_addressof(state->mouse_pos_world);
  state->game_info.mouse_pos_screen              = __builtin_addressof(state->mouse_pos_screen);
  state->game_info.is_game_end                   = __builtin_addressof(state->is_game_end);
  state->game_info.is_game_paused                = __builtin_addressof(state->is_game_paused);
  state->game_info.ingame_phase                  = __builtin_addressof(state->ingame_phase);
  state->game_info.show_pause_menu               = __builtin_addressof(state->show_pause_menu);

  event_register(EVENT_CODE_END_GAME, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_ADD_CURRENCY_SOULS, game_manager_on_event);
  event_register(EVENT_CODE_RESUME_GAME, game_manager_on_event);
  event_register(EVENT_CODE_PAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_TOGGLE_GAME_PAUSE, game_manager_on_event);

  return game_manager_reinit(in_camera_metrics, in_app_settings, in_active_map_ptr);
}
bool game_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Player system initialization failed");
    return false;
  }
  if (!parse_or_create_save_data_from_file(SAVE_SLOT_CURRENT_SESSION)) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Save system failed to creating/parsing serialization data");
    return false;
  }
  gm_load_game();
  state->game_manager_initialized = true;
  state->is_game_paused = true;

  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = in_app_settings;
  state->in_active_map = in_active_map_ptr;

  return true;
}

void update_game_manager(void) {
  if (!state) {
    return;
  }
  state->mouse_pos_screen = Vector2 {
    GetMousePosition().x * get_app_settings()->scale_ratio.at(0),
    GetMousePosition().y * get_app_settings()->scale_ratio.at(1)
  };
  state->mouse_pos_world = GetScreenToWorld2D(Vector2 {
    state->mouse_pos_screen.x,
    state->mouse_pos_screen.y
  },
    state->in_camera_metrics->handle
  );

  if(state->is_game_paused || state->is_game_end) {
    return;
  }

  if (state->game_info.player_state_dynamic && state->game_info.player_state_dynamic != nullptr && !state->game_info.player_state_dynamic->is_dead) {
    state->game_info.player_state_dynamic->position_centered = Vector2 {
      state->game_info.player_state_dynamic->position.x + state->game_info.player_state_dynamic->dimentions_div2.x,
      state->game_info.player_state_dynamic->position.y + state->game_info.player_state_dynamic->dimentions_div2.y
    };
    const player_update_results pur = update_player();
    if (pur.is_success) { // TODO: Player-Map collision detection system
      const Rectangle& pl_map_coll = state->game_info.player_state_dynamic->map_level_collision;
      const Rectangle new_x = Rectangle { pl_map_coll.x + pur.move_request.x, pl_map_coll.y,                      pl_map_coll.width, pl_map_coll.height };
      const Rectangle new_y = Rectangle { pl_map_coll.x,                      pl_map_coll.y + pur.move_request.y, pl_map_coll.width, pl_map_coll.height };

      bool is_collided_x = false;
      bool is_collided_y = false;
      for (size_t itr_000 = 0; itr_000 < (*state->in_active_map)->collisions.size() ; ++itr_000) {
        const Rectangle& map_coll = (*state->in_active_map)->collisions.at(itr_000).dest;

        if(CheckCollisionRecs(map_coll, new_x)) {
          is_collided_x = true;
        }
        if(CheckCollisionRecs(map_coll, new_y)) {
          is_collided_y = true;
        }
      }
      if ( !is_collided_x && pur.move_request.x) {
        player_move_player(VECTOR2(pur.move_request.x, 0.f));
        //player_move_player(VECTOR2(0.f, 2.f));
      }
      if ( !is_collided_y && pur.move_request.y) {
        player_move_player(VECTOR2(0.f, pur.move_request.y));
      }
      if (pur.move_request.x == 0 && pur.move_request.y == 0) {
        player_move_player(VECTOR2(0.f, 0.f));
      }
    }
    if (state->game_info.in_spawns->size() == 0) {
      if (spawn_boss() && state->ingame_phase != INGAME_PHASE_DEFEAT_BOSS) {
        state->ingame_phase = INGAME_PHASE_DEFEAT_BOSS;
      }
      else {
        gm_end_game(true);
      }
    }

    update_spawns(state->game_info.player_state_dynamic->position_centered);

    generate_in_game_info();

    update_abilities(__builtin_addressof(state->game_info.player_state_dynamic->ability_system));

    for (size_t itr_000 = 0; itr_000 < ABILITY_TYPE_MAX; ++itr_000) {
      if (state->game_info.player_state_dynamic->ability_system.abilities.at(itr_000).is_active) {
        state->game_info.player_state_dynamic->ability_system.abilities.at(itr_000).ability_play_time += GetFrameTime();
      }
    }
    state->game_info.play_time += GetFrameTime();
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
    player_move_player(pur.move_request);
  }
}
void render_game(void) {
  if (!state) {
    return;
  }
  if (!state->game_info.player_state_dynamic->is_dead) {
    render_abilities(__builtin_addressof(state->game_info.player_state_dynamic->ability_system));
    render_player();
    render_spawns();

    for (size_t itr_000 = 0; itr_000 < (*state->in_active_map)->collisions.size() ; ++itr_000) {
      DrawRectangleLinesEx((*state->in_active_map)->collisions.at(itr_000).dest, 2.f, BLUE);
    }
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
bool gm_start_game(worldmap_stage stage) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State returned null");
    return false;
  }
  state->stage = stage;
  if (_add_ability(state->game_info.player_state_dynamic->starter_ability)) {
    refresh_player_stats(true, false);
  }
  else return false;
  
  populate_map_with_spawns(stage.total_spawn_count);
  if (state->game_info.in_spawns->size() <= 0) {
    return false;
  }
  TraceLog(LOG_INFO, "Spawn Count: %llu", state->game_info.in_spawns->size());

  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->game_info.player_state_dynamic->exp_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)state->game_info.player_state_dynamic->health_perc));

  state->ingame_phase = INGAME_PHASE_CLEAR_ZOMBIES;
  state->is_game_end = false;
  state->is_game_paused = false;

  return true;
}
void gm_end_game(bool is_win) {
  clean_up_spawn_state();

  copy_memory(state->game_info.player_state_dynamic, __builtin_addressof(state->player_state_static), sizeof(player_state));
  state->game_info.is_win = is_win;
  state->is_game_end = true;
  state->is_game_paused = false;
  state->stage = worldmap_stage();
}
void gm_save_game(void) {
  state->game_progression_data->currency_souls_player_have += state->game_info.collected_souls;
  state->game_progression_data->p_player = state->player_state_static;
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void gm_load_game(void) {
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);

  for (size_t iter=0; iter < CHARACTER_STATS_MAX; ++iter) {
    state->player_state_static.stats[iter].level = state->game_progression_data->p_player.stats[iter].level;
  }
  copy_memory(state->game_info.player_state_dynamic, __builtin_addressof(state->player_state_static), sizeof(player_state));

  refresh_player_stats(false, true);
}
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check) {
  switch (coll_check) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      Rectangle rect = Rectangle { (f32)coll_data.i16[0],  (f32)coll_data.i16[1], (f32)coll_data.i16[2], (f32)coll_data.i16[3] };

      for (u32 itr_000 = 0; itr_000 < state->game_info.in_spawns->size(); ++itr_000) {
        if (!state->game_info.in_spawns->at(itr_000).is_dead){
          if (CheckCollisionRecs(state->game_info.in_spawns->at(itr_000).collision, rect)) {
            damage_spawn(state->game_info.in_spawns->at(itr_000).character_id, damage);
          }
        }
      }
      return; 
    }
    case COLLISION_TYPE_CIRCLE_RECTANGLE: { 
      Vector2 circle_center = Vector2 { (f32)coll_data.i16[0],  (f32)coll_data.i16[1] };
      f32 circle_radius = (f32) coll_data.i16[2];

      for (u32 itr_000 = 0; itr_000 < state->game_info.in_spawns->size(); ++itr_000) {
        if (!state->game_info.in_spawns->at(itr_000).is_dead){
          if (CheckCollisionCircleRec(circle_center, circle_radius, state->game_info.in_spawns->at(itr_000).collision)) {
            damage_spawn(state->game_info.in_spawns->at(itr_000).character_id, damage);
          }
        }
      }
      return; 
    }
    default: {
      TraceLog(LOG_WARNING, "game_manager::gm_damage_spawn_if_collide()::Unsupported collision check type");
      return;
    }
  }

  TraceLog(LOG_WARNING, "game_manager::gm_damage_spawn_if_collide()::Function terminated unexpectedly");
}
void gm_damage_player_if_collide(data128 coll_data, i32 damage, collision_type coll_check) {
  switch (coll_check) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      Rectangle rect = Rectangle { (f32)coll_data.i16[0],  (f32)coll_data.i16[1], (f32)coll_data.i16[2], (f32)coll_data.i16[3] };

      if (CheckCollisionRecs(state->game_info.player_state_dynamic->collision, rect)) {
        player_take_damage(damage);
      }
      return; 
    }
    
    case COLLISION_TYPE_CIRCLE_RECTANGLE: { 
      Vector2 circle_center = Vector2 { (f32)coll_data.i16[0],  (f32)coll_data.i16[1] };
      f32 circle_radius = (f32) coll_data.i16[2];

      if (CheckCollisionCircleRec(circle_center, circle_radius, state->game_info.player_state_dynamic->collision)) {
        player_take_damage(damage);
      }
      return; 
    }

    default: TraceLog(LOG_WARNING, "game_manager::gm_damage_player_if_collide()::Unsupported collision check type"); return;
  }

  TraceLog(LOG_WARNING, "game_manager::gm_damage_player_if_collide()::Function terminated unexpectedly");
}

void populate_map_with_spawns(i32 min_count) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::populate_map_with_spawns()::State is not valid");
    return;
  }
  if (!state->game_info.in_spawns || state->game_info.in_spawns == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::populate_map_with_spawns()::Spawns pointer is not valid");
    return;
  }
  i32 spawn_trying_limit = SPAWN_TRYING_LIMIT;
  for (i32 itr_000 = 0; itr_000 < min_count && (spawn_trying_limit <= SPAWN_TRYING_LIMIT && spawn_trying_limit != 0); ) 
  {
    Vector2 position = Vector2 {
      static_cast<f32>(get_random((i32)state->stage.spawning_areas.at(0).x, (i32)state->stage.spawning_areas.at(0).x + state->stage.spawning_areas.at(0).width)),
      static_cast<f32>(get_random((i32)state->stage.spawning_areas.at(0).y, (i32)state->stage.spawning_areas.at(0).y + state->stage.spawning_areas.at(0).height))
    };
    spawn_character(Character2D(
      static_cast<i32>(get_random(SPAWN_TYPE_UNDEFINED+1, SPAWN_TYPE_MAX-2)),
      static_cast<i32>(state->game_info.player_state_dynamic->level),
      static_cast<i32>(get_random(0, 100)),
      position,
      1.f)
    )
      ? ++itr_000 : --spawn_trying_limit;
  }
}

bool spawn_boss(void) {
  const worldmap_stage& stage = state->stage;

  Character2D _boss = Character2D(
    SPAWN_TYPE_BOSS,
    GET_BOSS_LEVEL(stage.stage_level, state->stage.boss_scale),
    GET_BOSS_SCALE(state->stage.boss_scale), 
    state->stage.boss_spawn_location, 
    GET_BOSS_SPEED(state->stage.boss_scale)
  );

  return spawn_character(_boss);
}
void upgrade_dynamic_player_stat(character_stats stat_id, u16 level) {
  if (!state || !state->game_info.player_state_dynamic->is_initialized) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::State is not valid");
    return;
  }
  if ( stat_id >= CHARACTER_STATS_MAX || stat_id <= CHARACTER_STATS_UNDEFINED) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::stat id out of bound");
    return;
  }
  character_stat* stat = __builtin_addressof(state->game_info.player_state_dynamic->stats[stat_id]);
  if (level > 0 && level <= MAX_PASSIVE_UPGRADE_TIER) stat->level = level;
  else {
    if (stat->level > MAX_PASSIVE_UPGRADE_TIER || stat->level < 0) {
      TraceLog(LOG_ERROR, "game_manager::upgrade_dynamic_player_stat()::stat level out of bound");
      return;
    }
    stat->level++;
  }
  game_manager_set_stats(stat, stat->level);

  player_state* player = state->game_info.player_state_dynamic;

  switch (stat->id) {
    case CHARACTER_STATS_HEALTH: { 
      player->health_max = stat->buffer.u32[0];
      player->health_current = player->health_max;
      player->health_perc = (f32) player->health_current / (f32) player->health_max;
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
  character_stat* stat = __builtin_addressof(state->player_state_static.stats[stat_id]);

  if (level > 0 && level <= MAX_PASSIVE_UPGRADE_TIER) stat->level = level;
  else {
    if (stat->level > MAX_PASSIVE_UPGRADE_TIER || stat->level < 0) {
      TraceLog(LOG_ERROR, "game_manager::upgrade_static_player_stat()::stat level out of bound");
      return;
    }
    stat->level++;
  }
  game_manager_set_stats(stat, stat->level);

  player_state* player = __builtin_addressof(state->player_state_static);

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
    if (state->game_info.player_state_dynamic != nullptr && !state->game_info.player_state_dynamic->is_initialized) {
      TraceLog(LOG_ERROR, "game_manager::refresh_player_stats()::stat returned null");
      return;
    }
    for (size_t stat = CHARACTER_STATS_UNDEFINED+1; stat < CHARACTER_STATS_MAX; stat++) {
      upgrade_dynamic_player_stat(static_cast<character_stats>(stat), state->game_info.player_state_dynamic->stats[stat].level);
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
  state->game_info.collected_souls += value;
}
void generate_in_game_info(void) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::generate_in_game_info()::State is not valid");
    return;
  }
  else if(state->is_game_paused || state->is_game_end) {
    return;
  }
  Vector2 player_position = state->game_info.player_state_dynamic->position;
  const Character2D * spw_nearest = nullptr;

  f32 dist_cache = F32_MAX;
  for (size_t itr_000 = 0; itr_000 < state->game_info.in_spawns->size(); ++itr_000) {
    const Character2D* spw = __builtin_addressof(state->game_info.in_spawns->at(itr_000));
    if (!spw) continue;
    
    f32 dist = vec2_distance(player_position, spw->position);
    if (dist < dist_cache) {
      spw_nearest = spw;
      dist_cache = dist;
    }
  }
  state->game_info.nearest_spawn = spw_nearest;
}
// OPS

// GET / SET
i32 get_currency_souls(void) {
  return state->game_progression_data->currency_souls_player_have;
}
bool get_b_player_have_upgrade_points(void) {
  return state->game_info.player_state_dynamic->is_player_have_ability_upgrade_points;
}
void set_dynamic_player_have_ability_upgrade_points(bool _b) {
  state->game_info.player_state_dynamic->is_player_have_ability_upgrade_points = _b;
}
const character_stat* get_static_player_state_stat(character_stats stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return nullptr;
  }
  
  return __builtin_addressof(state->player_state_static.stats[stat]);
}
void toggle_is_game_paused(void) {
  state->is_game_paused = !state->is_game_paused;
}
void set_currency_souls(i32 value) {
  state->game_progression_data->currency_souls_player_have = value;
}
const ingame_info* gm_get_ingame_info(void){
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_get_ingame_info()::State is not valid");
    return nullptr;
  }
  return __builtin_addressof(state->game_info); 
}
// GET / SET

// Exposed functions
u16 _spawn_character(Character2D _character) {
  return spawn_character(_character);
}
ability _get_ability(ability_type type) {
  if (type <= ABILITY_TYPE_UNDEFINED || type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_get_ability()::Ability type is out of bound");
    return ability();
  }
  return get_ability(type);
}
bool _add_ability(ability_type _type) {
  if (_type <= ABILITY_TYPE_UNDEFINED || _type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_add_ability()::Ability type is out of bound");
    return false;
  }
  ability abl = get_ability(_type);
  ability_play_system* system = __builtin_addressof(state->game_info.player_state_dynamic->ability_system);
  if (!system) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  abl.p_owner = state->game_info.player_state_dynamic;
  abl.is_initialized = true;
  abl.proj_count += state->game_info.player_state_dynamic->projectile_amouth;
  abl.is_active = true;

  refresh_ability(__builtin_addressof(abl));
  system->abilities.at(_type) = abl;
  return true;
}
bool _upgrade_ability(ability* abl) {
  if (!abl->is_initialized) {
    TraceLog(LOG_WARNING, "game_manager::_upgrade_ability::Recieved ability has not initialized yet");
    return false;
  }
  upgrade_ability(abl);
  refresh_ability(abl);

  return true;
}
ability _get_next_level(ability abl) {
  if (!abl.is_initialized || abl.type <= ABILITY_TYPE_UNDEFINED || abl.type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "game_manager::_get_next_level()::Recieved ability has not initialized yet");
    return ability();
  }
  return get_next_level(abl);
}
void _set_player_position(Vector2 position) {
  state->game_info.player_state_dynamic->position = position;
  state->game_info.player_state_dynamic->collision.x = position.x;
  state->game_info.player_state_dynamic->collision.y = position.y;
}
// Exposed functions


bool game_manager_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_END_GAME: {
      state->is_game_end = true;
      return true;
    }
    case EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE: {
      data128 coll_data = data128( 
        static_cast<i16>(context.data.i16[0]),
        static_cast<i16>(context.data.i16[1]),
        static_cast<i16>(context.data.i16[2]),
        static_cast<i16>(context.data.i16[3])
      );
      gm_damage_player_if_collide(coll_data, static_cast<i32>(context.data.i16[4]), static_cast<collision_type>(context.data.i16[5]));
      return true;
    }
    case EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE: {
      data128 coll_data = data128( 
        static_cast<i16>(context.data.i16[0]),
        static_cast<i16>(context.data.i16[1]),
        static_cast<i16>(context.data.i16[2]),
        static_cast<i16>(context.data.i16[3])
      );
      gm_damage_spawn_if_collide(coll_data, static_cast<i32>(context.data.i16[4]), static_cast<collision_type>(context.data.i16[5]));
      return true;
    }
    case EVENT_CODE_ADD_CURRENCY_SOULS: {
      state->game_info.collected_souls += context.data.i32[0];
      return true;
    }
    case EVENT_CODE_RESUME_GAME: {
      state->is_game_paused = false;
      state->show_pause_menu = false;
      return true;
    }
    case EVENT_CODE_PAUSE_GAME: {
      state->is_game_paused = true;
      state->show_pause_menu = true;
      return true;
    }
    case EVENT_CODE_TOGGLE_GAME_PAUSE: {
      state->is_game_paused = !state->is_game_paused;
      state->show_pause_menu = !state->show_pause_menu;
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
