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
  Character2D* spawns;
  u16* p_spawn_system_spawn_count;
  ability* player_abilities;

  player_state* p_player;
  worldmap_stage stage;

  save_data* game_progression_data;

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

bool game_manager_initialize(camera_metrics* _camera_metrics) {
  if (state) {
    game_manager_reinit();
    return true;
  }
  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);

  if (!ability_system_initialize(_camera_metrics, get_app_settings())) {
    TraceLog(LOG_ERROR, "game_manager::ability_system_initialize()::Returned false");
    return false;
  }
  if (!spawn_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::spawn_system_initialize()::Returned false");
    return false;
  }

  event_register(EVENT_CODE_END_GAME, game_manager_on_event);
  event_register(EVENT_CODE_PAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_UNPAUSE_GAME, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_ADD_ITEM_PLAYER_CURRENCY_SOULS, game_manager_on_event);

  game_manager_reinit();
  return true;
}

void update_game_manager() {
  if (!state) {
    return;
  }

  if (!state->p_player->is_dead) {
    const player_update_results pur = update_player();
    if (pur.is_success) { // TODO: Player-Map collision detection system
      /*
      const Rectangle pl_col = state->p_player->collision;
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
    update_abilities(&state->p_player->ability_system);
    update_spawns(get_player_position(true));

    for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
      state->p_player->ability_system.abilities[i].ability_play_time += GetFrameTime();
    }
  }
}

void render_game(void) {
  if (!state) {
    return;
  }
  if (!state->p_player->is_dead) {
    render_abilities(&state->p_player->ability_system);
    render_player();
    render_spawns();
  }
  else {
    render_player();
    render_spawns();
  }
}

// OPS
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
    for (u16 i = 0; i <= *state->p_spawn_system_spawn_count; ++i) {
      if (!state->spawns[i].is_dead){
        if (CheckCollisionRecs(state->spawns[i].collision, from_actor.collision)) {
          damage_spawn(state->spawns[i].character_id, from_actor.damage);
        }
      }
    }
    break;
  }
  case ACTOR_TYPE_PLAYER: {
    if (CheckCollisionRecs(from_actor.collision, state->p_player->collision)) {
      event_fire(EVENT_CODE_PLAYER_TAKE_DAMAGE, (event_context) { .data.u8[0] = from_actor.damage});
      return;
    }
    break;
  }
  case ACTOR_TYPE_PROJECTILE_SPAWN: break; 
  case ACTOR_TYPE_PROJECTILE_PLAYER: break; 
  }
}
void gm_start_game(worldmap_stage stage) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State returned null");
    return;
  }

  state->stage = stage;
  u32 spawn_trying_limit = SPAWN_TRYING_LIMIT;
  for (u16 i = 0; i < MAX_SPAWN_COUNT && (spawn_trying_limit <= SPAWN_TRYING_LIMIT && spawn_trying_limit > 0); ) 
  {
    Vector2 position = (Vector2) {
      get_random((i32)state->stage.spawning_areas[0].x, (i32)state->stage.spawning_areas[0].x + state->stage.spawning_areas[0].width),
      get_random((i32)state->stage.spawning_areas[0].y, (i32)state->stage.spawning_areas[0].y + state->stage.spawning_areas[0].height)
    };
    i32 rnd = get_random(0, 100);
    f32 scale = 2.f + (.5f * rnd / 100.f); // 2.f is minimum and "min + (.5f)" is max
    spawn_character((Character2D) {
      .character_id = 0,
      .buffer.u16[0] = get_random(SPAWN_TYPE_UNDEFINED+1, SPAWN_TYPE_MAX-1),
      .scale = scale,
      .initialized = false,
      .position = position,
      .w_direction = WORLD_DIRECTION_LEFT,
      .type = ACTOR_TYPE_SPAWN,
      .rotation = 0,
      .health = 100,
      .damage = 10,
      .speed = 1,
    }) ? ++i : --spawn_trying_limit;
  }

  heal_player(state->p_player->health_max);
  state->p_player->is_player_have_ability_upgrade_points = false;
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_EXPERIANCE,
    .data.f32[1] = state->p_player->exp_perc,
  });
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, (event_context){
    .data.f32[0] = PRG_BAR_ID_PLAYER_HEALTH,
    .data.f32[1] = state->p_player->health_perc,
  });  
  event_fire(EVENT_CODE_SCENE_MANAGER_SET_CAM_POS, (event_context){
    .data.f32[0] = state->p_player->position.x,
    .data.f32[1] = state->p_player->position.y,
  });

  _add_ability(state->p_player->starter_ability);
}
void upgrade_player_stat(character_stat* _stat) {
  if (!state) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State returned null");
    return;
  }

  character_stat* stat = &state->p_player->stats[_stat->id];
  if (!stat) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::stat returned null");
    return;
  }
  stat->level++;
  if (stat->level >= MAX_PLAYER_LEVEL || stat->level <= 0) {
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::player level out of bound");
    return;
  }
  u32 next_curve_value = level_curve[stat->level];
  switch (stat->id) {
  case CHARACTER_STATS_HEALTH:{
    const u32 value = next_curve_value;

    stat->buffer.u32[0] = value;
    state->p_player->health_max = value;
    break; 
  }
  case CHARACTER_STATS_HP_REGEN:{ 
    const u32 value = next_curve_value;

    stat->buffer.u32[0] = value;
    state->p_player->health_regen = value;
    break;
  }
  case CHARACTER_STATS_MOVE_SPEED:{
    const f32 value = next_curve_value / 1000.f;

    stat->buffer.f32[0] = value;
    state->p_player->move_speed_multiply = value;
    break;
  }
  case CHARACTER_STATS_AOE:{
    const f32 value = next_curve_value / 1000.f;

    stat->buffer.f32[0] = value;
    state->p_player->damage_area_multiply = value;
    break;
  }
  case CHARACTER_STATS_DAMAGE:{
    const u32 value = next_curve_value;

    stat->buffer.u32[0] = value;
    state->p_player->damage = value;
    break;
  }
  case CHARACTER_STATS_ABILITY_CD:{
    const f32 value = next_curve_value / 1000.f;

    stat->buffer.f32[0] = value;
    state->p_player->cooldown_multiply = value;
    break;
  }
  case CHARACTER_STATS_PROJECTILE_AMOUTH:{
    const u16 value = stat->level;

    stat->buffer.u16[0] = value;
    state->p_player->projectile_amouth = value;
    break;
  }
  case CHARACTER_STATS_EXP_GAIN:{
    const u32 value = next_curve_value / 1000.f;

    stat->buffer.u32[0] = value;
    state->p_player->exp_gain_multiply = value;
    break;
  }
  default:{
    TraceLog(LOG_ERROR, "game_manager::upgrade_player_stat()::Unsuppported stat id");
    break;
  }
  }
}
void gm_reset_game() {
  clean_up_spawn_state();

  state->is_game_end = false;
  state->is_game_paused = true;
  state->is_game_end = false;
  state->stage = (worldmap_stage){0};
}
void gm_save_game() {
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void game_manager_reinit(void) {
  if (!player_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Returned false");
  }

  state->p_player = get_player_state();
  state->player_abilities = state->p_player->ability_system.abilities;
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);
  state->spawns = get_spawns();
  state->p_spawn_system_spawn_count = get_spawn_count();
  state->game_manager_initialized = true;
  state->is_game_paused = true;
}
// OPS

// GET / SET
u32 get_currency_souls(void) {
  return state->game_progression_data->currency_souls_player_have;
}
bool get_b_player_have_upgrade_points(void) {
  return state->p_player->is_player_have_ability_upgrade_points;
}
void set_player_have_ability_upgrade_points(bool _b) {
  state->p_player->is_player_have_ability_upgrade_points = _b;
}
ability* get_player_ability(ability_type type) {
  return &state->p_player->ability_system.abilities[type];
}
character_stat* get_player_stat(character_stats stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return 0;
  }
  
  return &state->p_player->stats[stat];
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

  abl.proj_count += state->p_player->projectile_amouth;

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
  ability_play_system* system = &state->p_player->ability_system;
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
  get_player_state()->position = position;
  get_player_state()->collision.x = position.x;
  get_player_state()->collision.y = position.y;
}
Vector2 _get_player_position(bool centered) {
  return get_player_position(centered);
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
        context.data.u16[0], context.data.u16[1], 
        context.data.u16[2], context.data.u16[3]
      },
      .damage = context.data.u16[4],
      .initialized = true
    };
    damage_any_collider_by_type(from_actor, ACTOR_TYPE_PLAYER);

    return true;
  }
  case EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE: {
    Character2D from_actor = (Character2D) {
      .collision = (Rectangle) {
        context.data.u16[0], context.data.u16[1], 
        context.data.u16[2], context.data.u16[3]
      },
      .damage = context.data.u16[4],
      .initialized = true
    };
    damage_any_collider_by_type(from_actor, ACTOR_TYPE_SPAWN);

    return true;
  }
  case EVENT_CODE_ADD_ITEM_PLAYER_CURRENCY_SOULS: {
    state->game_progression_data->currency_souls_player_have += context.data.u32[0];
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
