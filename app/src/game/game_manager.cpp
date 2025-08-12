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
  std::vector<character_trait> traits;
  std::vector<character_trait> chosen_traits;

  bool game_manager_initialized;
  game_manager_system_state(void) {
    this->game_progression_data = nullptr;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->in_active_map = nullptr;
    this->stage = worldmap_stage();
    this->ingame_phase = INGAME_PHASE_UNDEFINED;
    this->player_state_static = player_state();
    this->game_info = ingame_info();
    this->mouse_pos_world = ZEROVEC2;
    this->mouse_pos_screen = ZEROVEC2;
    this->traits.clear();
    this->game_manager_initialized = false;
  }
} game_manager_system_state;

static game_manager_system_state * state = nullptr;

// To avoid dublicate symbol errors. Implementation in defines.h
extern const u32 level_curve[MAX_PLAYER_LEVEL+1];

#define SPAWN_TRYING_LIMIT 15
#define GET_BOSS_LEVEL(STAGE_LEVEL, VAL) (STAGE_LEVEL + (STAGE_LEVEL * state->stage.boss_scale))
#define GET_BOSS_SCALE(VAL) (100 + (100 * VAL))
#define GET_BOSS_SPEED(VAL) (1.1f * VAL)

bool game_manager_on_event(i32 code, event_context context);
bool game_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings,tilemap ** const in_active_map_ptr);
void populate_map_with_spawns(i32 min_count);
i32 spawn_boss(void);
void generate_in_game_info(void);
void game_manager_set_stat_trait_value_by_level(character_stat* stat, data128 value);
void reset_ingame_info(void);
void gm_update_player(void);

bool game_manager_initialize(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (state) {
    return game_manager_reinit(in_camera_metrics, in_app_settings, in_active_map_ptr);
  }
  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Gama manager state allocation failed");
    return false;
  }
  *state = game_manager_system_state();

  if (not save_system_initialize()) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Save system init returned false");
    return false;
  }
  if (not ability_system_initialize(in_camera_metrics, get_app_settings(), __builtin_addressof(state->game_info))) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Ability system init returned false");
    return false;
  }
  if (not spawn_system_initialize(in_camera_metrics, __builtin_addressof(state->game_info))) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_initialize()::Spawn system init returned false");
    return false;
  }
  if (not player_system_initialize(in_camera_metrics, get_app_settings(), __builtin_addressof(state->game_info))) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Returned false");
    return false;
  }
  state->player_state_static = *get_player_state();
  
  state->game_info.player_state_dynamic = get_player_state();
  state->game_info.in_spawns            = get_spawns();
  state->game_info.mouse_pos_world      = __builtin_addressof(state->mouse_pos_world);
  state->game_info.mouse_pos_screen     = __builtin_addressof(state->mouse_pos_screen);
  state->game_info.ingame_phase         = __builtin_addressof(state->ingame_phase);
  state->game_info.chosen_traits        = __builtin_addressof(state->chosen_traits);

  event_register(EVENT_CODE_END_GAME, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_ADD_CURRENCY_SOULS, game_manager_on_event);

  // Traits
  {
    i32 next_trait_id = 0;
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_HEALTH,     
      "Healty", "Increases your healt a bit", -1, data128(static_cast<i32>(300)))
    );
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_HEALTH,     
      "Tough", "Increases your healt a fair amouth", -2, data128(static_cast<i32>(800)))
    );
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE,     
      "Artillery", "Increases your damage a fair amouth", -2, data128(static_cast<i32>(8)))
    );
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_MOVE_SPEED, 
      "Bunny Hop", "Increases your speed a bit", -1, data128(static_cast<f32>(1000)))
    );
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_MOVE_SPEED, 
      "Flash",  "Increases your speed a fair amouth", -2, data128(static_cast<f32>(5000)))
    );
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 6",  "Trait 6 description", -2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 7",  "Trait 7 description", -2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 8",  "Trait 8 description",  2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 9",  "Trait 9 description",  2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 10", "Trait 10 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 11", "Trait 11 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 12", "Trait 12 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 13", "Trait 13 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 14", "Trait 14 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 15", "Trait 15 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 16", "Trait 16 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 17", "Trait 17 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 18", "Trait 18 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 19", "Trait 19 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 20", "Trait 20 description", 2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Trait 21", "Trait 21 description", 2, data128(static_cast<i32>(8))));
  }
  // Traits

  return game_manager_reinit(in_camera_metrics, in_app_settings, in_active_map_ptr);
}
bool game_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (not player_system_initialize(in_camera_metrics, in_app_settings, __builtin_addressof(state->game_info))) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Player system initialization failed");
    return false;
  }
  if (not parse_or_create_save_data_from_file(SAVE_SLOT_CURRENT_SESSION)) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_reinit()::Save system failed to creating/parsing serialization data");
    return false;
  }
  gm_load_game();
  state->game_manager_initialized = true;

  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = in_app_settings;
  state->in_active_map = in_active_map_ptr;

  return true;
}

void update_game_manager(void) {
  if (not state or state == nullptr) {
    return;
  }
  state->mouse_pos_screen = Vector2 { GetMousePosition().x * get_app_settings()->scale_ratio.at(0), GetMousePosition().y * get_app_settings()->scale_ratio.at(1)};
  state->mouse_pos_world = GetScreenToWorld2D(Vector2 {state->mouse_pos_screen.x,state->mouse_pos_screen.y}, state->in_camera_metrics->handle);

  switch (state->ingame_phase) {
    case INGAME_PHASE_CLEAR_ZOMBIES: {
      if (not state->game_info.player_state_dynamic->is_dead) {
        gm_update_player();
        
        if (state->game_info.in_spawns->size() == 0) {
          if ( (spawn_boss() >= 0) && state->ingame_phase != INGAME_PHASE_DEFEAT_BOSS) {
            state->ingame_phase = INGAME_PHASE_DEFEAT_BOSS;
          }
          else {
            gm_end_game(true);
          }
        }
        update_spawns(state->game_info.player_state_dynamic->position);
        update_abilities(__builtin_addressof(state->game_info.player_state_dynamic->ability_system));
        generate_in_game_info();
      }
      else {
        gm_end_game(false);
        return;
      }
      break;
    }
    case INGAME_PHASE_DEFEAT_BOSS: {
      if (not state->game_info.player_state_dynamic->is_dead) {
        const Character2D * const boss = get_spawn_by_id(state->game_info.stage_boss_id);
        if (boss) {
          f32 boss_health_perc = static_cast<f32>(boss->health_current) / static_cast<f32>(boss->health_max);
          event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context(static_cast<f32>(PRG_BAR_ID_BOSS_HEALTH), boss_health_perc));
          gm_update_player();
          update_spawns(state->game_info.player_state_dynamic->position);
          update_abilities(__builtin_addressof(state->game_info.player_state_dynamic->ability_system));
          generate_in_game_info();
        } else {
          gm_end_game(true);
        }
      }
      else {
        gm_end_game(false);
      }
      break;
    }
    case INGAME_PHASE_RESULTS: { 
      
      break;
    }
    default: {
      TraceLog(LOG_WARNING, "game_manager::update_game_manager()::Unknown in-game phase");
      gm_end_game(false);
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
    }
    break;
  }
}
void update_game_manager_debug(void) {
  if (not state or state == nullptr) {
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
  if (not state or state == nullptr) {
    return;
  }
  switch (state->ingame_phase) {
    case INGAME_PHASE_CLEAR_ZOMBIES: {
      if (not state->game_info.player_state_dynamic->is_dead) {
        render_abilities(__builtin_addressof(state->game_info.player_state_dynamic->ability_system));
        render_player();
        render_spawns();
      
        //for (size_t itr_000 = 0; itr_000 < (*state->in_active_map)->collisions.size() ; ++itr_000) {
        //  DrawRectangleLinesEx((*state->in_active_map)->collisions.at(itr_000).dest, 2.f, BLUE);
        //}
      }
      else {
        render_player();
        render_spawns();
      }
      break;
    }
    case INGAME_PHASE_DEFEAT_BOSS: { 
      if (not state->game_info.player_state_dynamic->is_dead) {
        render_abilities(__builtin_addressof(state->game_info.player_state_dynamic->ability_system));
        render_player();
        render_spawns();
      
        //for (size_t itr_000 = 0; itr_000 < (*state->in_active_map)->collisions.size() ; ++itr_000) {
        //  DrawRectangleLinesEx((*state->in_active_map)->collisions.at(itr_000).dest, 2.f, BLUE);
        //}
      }
      else {
        render_player();
        render_spawns();
      }
      break;
    }
    case INGAME_PHASE_RESULTS: { 
      
      break;
    }
    default: {
      TraceLog(LOG_WARNING, "game_manager::render_game()::Unknown in-game phase");
      gm_end_game(false);
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
    }
    break;
  }
}

// OPS
/**
 * @brief Start game before, then upgrade the stat
 */
bool gm_start_game(worldmap_stage stage) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::gm_start_game()::State is invalid");
    return false;
  }
  state->stage = stage;
  _add_ability(state->game_info.player_state_dynamic->starter_ability);

  std::array<character_stat, CHARACTER_STATS_MAX> * const _stat_array_ptr = __builtin_addressof(state->game_info.player_state_dynamic->stats);

  for (size_t itr_000 = 0; itr_000 < state->chosen_traits.size(); ++itr_000) {
    const character_trait * const _trait_ptr = __builtin_addressof(state->chosen_traits.at(itr_000));
    if (_trait_ptr->type < 0 && _trait_ptr->type >= CHARACTER_STATS_MAX) {
      TraceLog(LOG_WARNING, "game_manager::gm_start_game()::Trait type is out of bound");
      continue;
    }
    character_stat * const _stat_ptr = __builtin_addressof(_stat_array_ptr->at(_trait_ptr->type));
    
    game_manager_set_stat_trait_value_by_level(_stat_ptr, _trait_ptr->ingame_ops);
  }
  
  //populate_map_with_spawns(stage.total_spawn_count);
  //if (state->game_info.in_spawns->size() <= 0) {
  //  return false;
  //}
  player_state * p_player_dynamic = state->game_info.player_state_dynamic;
  p_player_dynamic->health_current = p_player_dynamic->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  p_player_dynamic->health_perc = static_cast<f32>(p_player_dynamic->health_current) / static_cast<f32>(p_player_dynamic->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->game_info.player_state_dynamic->exp_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)state->game_info.player_state_dynamic->health_perc));

  state->ingame_phase = INGAME_PHASE_CLEAR_ZOMBIES;
  reset_ingame_info();
  return true;
}
void gm_end_game(bool is_win) {
  clean_up_spawn_state();

  *state->game_info.player_state_dynamic = player_state();
  state->game_info.is_win = is_win;
  state->ingame_phase = INGAME_PHASE_RESULTS;
}
void gm_save_game(void) {
  state->game_progression_data->currency_souls_player_have += state->game_info.collected_souls;
  state->game_progression_data->p_player = state->player_state_static;
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void gm_load_game(void) {
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);

  for (size_t itr_000 = 0; itr_000 < state->player_state_static.stats.size(); ++itr_000) {
    i32 level = state->game_progression_data->p_player.stats.at(itr_000).level;
    if (level > 0 && level < MAX_PLAYER_LEVEL) {
      set_static_player_state_stat(static_cast<character_stats>(itr_000), state->game_progression_data->p_player.stats.at(itr_000).level);
    }
    else {
      set_static_player_state_stat(static_cast<character_stats>(itr_000), 1);
    }
  }
  
  state->player_state_static.stats = state->player_state_static.stats;
  *state->game_info.player_state_dynamic = state->player_state_static;
}
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check) {
  switch (coll_check) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      Rectangle rect = Rectangle { (f32)coll_data.i16[0],  (f32)coll_data.i16[1], (f32)coll_data.i16[2], (f32)coll_data.i16[3] };

      for (size_t itr_000 = 0; itr_000 < state->game_info.in_spawns->size(); ++itr_000) {
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

      for (size_t itr_000 = 0; itr_000 < state->game_info.in_spawns->size(); ++itr_000) {
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
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::populate_map_with_spawns()::State is not valid");
    return;
  }
  if (not state->game_info.in_spawns or state->game_info.in_spawns == nullptr) {
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
    if (spawn_character(Character2D(static_cast<i32>(get_random(SPAWN_TYPE_UNDEFINED+1, SPAWN_TYPE_MAX-2) >= 0),
      static_cast<i32>(state->game_info.player_state_dynamic->level),static_cast<i32>(get_random(0, 100)), position))
    ) 
    { ++itr_000; }
    else { --spawn_trying_limit; }
  }
}

i32 spawn_boss(void) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::spawn_boss()::State is invalid");
    return -1;
  }
  for (i32 boss_spawning_attemts = 0; boss_spawning_attemts < SPAWN_TRYING_LIMIT; ++boss_spawning_attemts) 
  {
    Vector2 position = Vector2 {
      static_cast<f32>(get_random((i32)state->stage.spawning_areas.at(0).x, (i32)state->stage.spawning_areas.at(0).x + state->stage.spawning_areas.at(0).width)),
      static_cast<f32>(get_random((i32)state->stage.spawning_areas.at(0).y, (i32)state->stage.spawning_areas.at(0).y + state->stage.spawning_areas.at(0).height))
    };
    Character2D _boss = Character2D(SPAWN_TYPE_BOSS, GET_BOSS_LEVEL(state->stage.stage_level, state->stage.boss_scale), GET_BOSS_SCALE(state->stage.boss_scale), position);

    i32 boss_id = spawn_character(_boss);
    if (boss_id >= 0) {
      state->game_info.stage_boss_id = boss_id;
      return boss_id;
    }
    else {
      continue;
    }
  }
  return -1;
}

void currency_souls_add(i32 value) {
  state->game_info.collected_souls += value;
}
void generate_in_game_info(void) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::generate_in_game_info()::State is not valid");
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
  state->game_info.play_time += GetFrameTime();
}
void reset_ingame_info(void) {
  if (!state || state == nullptr) {
    TraceLog(LOG_INFO, "game_manager::reset_ingame_info()::State is invalid");
    return;
  }
  state->game_info.collected_souls = 0;
  state->game_info.play_time = 0.f;
  state->game_info.is_win = false;
}
void gm_update_player(void) {
  if (state->game_info.player_state_dynamic && state->game_info.player_state_dynamic != nullptr && !state->game_info.player_state_dynamic->is_dead) {
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
      }
      if ( !is_collided_y && pur.move_request.y) {
        player_move_player(VECTOR2(0.f, pur.move_request.y));
      }
      if (pur.move_request.x == 0 && pur.move_request.y == 0) {
        player_move_player(VECTOR2(0.f, 0.f));
      }
    }

    for (size_t itr_000 = 0; itr_000 < ABILITY_TYPE_MAX; ++itr_000) {
      if (state->game_info.player_state_dynamic->ability_system.abilities.at(itr_000).is_active) {
        state->game_info.player_state_dynamic->ability_system.abilities.at(itr_000).ability_play_time += GetFrameTime();
      }
    }
  }
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
  
  return __builtin_addressof(state->player_state_static.stats.at(stat));
}
void set_static_player_state_stat(character_stats stat_id, i32 level) {
  if (stat_id >= CHARACTER_STATS_MAX || stat_id <= CHARACTER_STATS_UNDEFINED) {
    return;
  }
  character_stat * _stat_ptr = __builtin_addressof(state->player_state_static.stats.at(stat_id));
  
  game_manager_set_stat_value_by_level(_stat_ptr, level);
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
const std::vector<character_trait>* gm_get_character_traits(void) {
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::gm_get_character_traits()::State is not valid");
    return nullptr;
  }

  return __builtin_addressof(state->traits);
}
void game_manager_set_stat_value_by_level(character_stat* stat, i32 level) {
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

  i32 next_curve_value = level_curve[level];
  stat->upgrade_cost = next_curve_value;
  stat->level = level;
  switch (stat->id) {
    case CHARACTER_STATS_HEALTH:{
      const i32 value = next_curve_value;

      stat->buffer.i32[1] = value;
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      return;
    }
    case CHARACTER_STATS_HP_REGEN:{ 
      const i32 value = next_curve_value / 1000.f;

      stat->buffer.i32[1] = value;
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      return;
    }
    case CHARACTER_STATS_MOVE_SPEED:{
      const f32 value = next_curve_value * .05f;

      stat->buffer.f32[1] = value;
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      return;
    }
    case CHARACTER_STATS_AOE:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[1] = value;
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      return;
    }
    case CHARACTER_STATS_DAMAGE:{
      const i32 value = next_curve_value * .1f;

      stat->buffer.i32[1] = value;
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      return;
    }
    case CHARACTER_STATS_ABILITY_CD:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[1] = value;
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      return;
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH:{
      const i32 value = stat->level;

      stat->buffer.i32[1] = value;
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      return;
    }
    case CHARACTER_STATS_EXP_GAIN:{
      const f32 value = next_curve_value / 1000.f;

      stat->buffer.f32[1] = value;
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      return;
    }
    case CHARACTER_STATS_TOTAL_TRAIT_POINTS:{
      const i32 value = next_curve_value / 100.f;

      stat->buffer.i32[1] = value;
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      return;
    }
    default:{
      TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::Unsuppported stat id");
      return;
    }
  }

  TraceLog(LOG_ERROR, "game_manager::game_manager_set_stats()::Function ended unexpectedly");
}
void game_manager_set_stat_trait_value_by_level(character_stat* stat, data128 value) {
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_sum_stat_value()::State is not valid");
    return;
  }
  if (not stat or stat == nullptr) {
    TraceLog(LOG_ERROR, "game_manager::game_manager_sum_stat_value()::stat is not valid");
    return;
  }
  switch (stat->id) {
    case CHARACTER_STATS_HEALTH:{
      stat->buffer.i32[2] = value.i32[0];
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      break;
    }
    case CHARACTER_STATS_HP_REGEN:{ 
      stat->buffer.i32[2] = value.i32[0];
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      break;
    }
    case CHARACTER_STATS_MOVE_SPEED:{
      stat->buffer.f32[2] = value.f32[0];
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      break;
    }
    case CHARACTER_STATS_AOE:{
      stat->buffer.f32[2] = value.f32[0];
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      break;
    }
    case CHARACTER_STATS_DAMAGE:{
      stat->buffer.i32[2] = value.i32[0];
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      break;
    }
    case CHARACTER_STATS_ABILITY_CD:{
      stat->buffer.f32[2] = value.f32[0];
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      break;
    }
    case CHARACTER_STATS_PROJECTILE_AMOUTH:{
      stat->buffer.i32[2] = value.i32[0];
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      break;
    }
    case CHARACTER_STATS_EXP_GAIN:{
      stat->buffer.f32[2] = value.f32[0];
      stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
      break;
    }
    case CHARACTER_STATS_TOTAL_TRAIT_POINTS:{
      stat->buffer.i32[2] = value.i32[0];
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      break;
    }
    default:{
      TraceLog(LOG_ERROR, "game_manager::game_manager_sum_stat_value()::Unsuppported stat id");
      break;
    }
  }
}
// GET / SET

// Exposed functions
ability _get_ability(ability_type type) {
  if (type <= ABILITY_TYPE_UNDEFINED or type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_get_ability()::Ability type is out of bound");
    return ability();
  }
  return get_ability(type);
}
const Character2D * _get_spawn_by_id(i32 _id) {
  return get_spawn_by_id(_id);
}
bool _add_ability(ability_type _type) {
  if (_type <= ABILITY_TYPE_UNDEFINED or _type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_INFO, "game_manager::_add_ability()::Ability type is out of bound");
    return false;
  }
  ability abl = get_ability(_type);
  ability_play_system* system = __builtin_addressof(state->game_info.player_state_dynamic->ability_system);
  if (not system or system == nullptr) {
    TraceLog(LOG_WARNING, "game_manager::_add_ability()::Recieved system was NULL");
    return false;
  }
  abl.p_owner = state->game_info.player_state_dynamic;
  abl.is_initialized = true;
  abl.proj_count += state->game_info.player_state_dynamic->stats.at(CHARACTER_STATS_PROJECTILE_AMOUTH).buffer.i32[3];
  abl.is_active = true;

  refresh_ability(__builtin_addressof(abl));
  system->abilities.at(_type) = abl;
  return true;
}
bool _upgrade_ability(ability* abl) {
  if (not abl or abl == nullptr or not abl->is_initialized) {
    TraceLog(LOG_WARNING, "game_manager::_upgrade_ability::Recieved ability has not initialized yet");
    return false;
  }
  upgrade_ability(abl);
  refresh_ability(abl);

  return true;
}
ability _get_next_level(ability abl) {
  if (not abl.is_initialized or abl.type <= ABILITY_TYPE_UNDEFINED or abl.type >= ABILITY_TYPE_MAX) {
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
      gm_end_game(static_cast<bool>(context.data.i32[0]));
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
    default: {
      TraceLog(LOG_WARNING, "game_engine::game_manager_on_event()::Unsuppported code.");
      return false;
    }
  }

  TraceLog(LOG_WARNING, "game_engine::game_manager_on_event()::Fire event ended unexpectedly");
  return false;
}

#undef SPAWN_TRYING_LIMIT
