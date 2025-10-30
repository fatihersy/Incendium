#include "game_manager.h"
#include <reasings.h>
#include <settings.h>
#include <save_game.h>
#include <sound.h>
#include <loc_types.h>

#include "core/ftime.h"
#include "core/event.h"
#include "core/fmemory.h"
#include "core/fmath.h"
#include "core/logger.h"

#include "game/abilities/ability_manager.h"
#include "game/collectible_manager.h"
#include "game/player.h"
#include "game/spawn.h"

typedef struct game_manager_system_state {
  tilemap ** in_active_map;
  save_data * game_progression_data;
  const camera_metrics * in_camera_metrics;
  const app_settings * in_app_settings;

  worldmap_stage stage;
  ingame_play_phases ingame_phase;
  player_state player_state_static;
  ingame_info game_info;
  Vector2 mouse_pos_world;
  Vector2 mouse_pos_screen;
  std::vector<character_trait> traits;
  std::vector<character_trait> chosen_traits;
  std::array<game_rule, GAME_RULE_MAX> game_rules;
  i32 collected_coins;
  i32 total_spawn_count;
  i32 total_boss_count;
  i32 total_boss_spawned;
  f32 total_play_time;
  f32 play_time;
  f32 delta_time;
  bool is_win;
  i32 stage_boss_id;
  ability_id starter_ability;
  
  playlist_control_system_state playlist;
  
  std::array<game_rule, GAME_RULE_MAX> default_game_rules;
  std::array<item_data, ITEM_TYPE_MAX> default_items;
  i32 next_inventory_slot_id;

  bool game_manager_initialized;

  game_manager_system_state(void) {
    this->in_active_map = nullptr;
    this->game_progression_data = nullptr;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;

    this->stage = worldmap_stage();
    this->ingame_phase = INGAME_PLAY_PHASE_UNDEFINED;
    this->player_state_static = player_state();
    this->game_info = ingame_info();
    this->mouse_pos_world = ZEROVEC2;
    this->mouse_pos_screen = ZEROVEC2;
    this->traits.clear();
    this->chosen_traits = std::vector<character_trait>();
    this->game_rules.fill(game_rule());
    this->collected_coins = 0;
    this->total_spawn_count = 0;
    this->total_boss_count = 0;
    this->total_boss_spawned = 0;
    this->total_play_time = 0.f;
    this->play_time = 0.f;
    this->is_win = false;
    this->stage_boss_id = INVALID_IDI32;
    this->starter_ability = ABILITY_ID_UNDEFINED;

    this->playlist = playlist_control_system_state();
    
    this->default_game_rules = std::array<game_rule, GAME_RULE_MAX>();
    this->default_items = std::array<item_data, ITEM_TYPE_MAX>();
    this->next_inventory_slot_id = 0;

    this->game_manager_initialized = false;
  }
} game_manager_system_state;

static game_manager_system_state * state = nullptr;

// INFO: To avoid dublicate symbol errors. Implementation in defines.h
extern const i32 level_curve[MAX_PLAYER_LEVEL+1];

#define SPAWN_TRYING_LIMIT 15
#define GET_BOSS_LEVEL(STAGE_LEVEL, VAL) (STAGE_LEVEL + (STAGE_LEVEL * state->stage.boss_scale))
#define GET_BOSS_SCALE(VAL) (100 + (100 * VAL))
#define GET_BOSS_SPEED(VAL) (1.1f * VAL)
#define ZOOM_CAMERA_GAME_RESULTS 1.5f
#define MAX_NEAREST_SPAWN_DISTANCE 512.f
#define MAX_GAME_RULE_LEVEL 5

bool game_manager_on_event(i32 code, event_context context);
bool game_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings,tilemap ** const in_active_map_ptr);
void populate_map_with_spawns(i32 min_count);
i32 spawn_boss(void);
void generate_in_game_info(void);
void game_manager_set_stat_trait_value_by_level(character_stat* stat, data128 value);
void reset_ingame_info(void);
void gm_update_player(void);

bool game_manager_initialize(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (state and state != nullptr) {
    return game_manager_reinit(in_camera_metrics, in_app_settings, in_active_map_ptr);
  }
  if (not in_camera_metrics or in_camera_metrics == nullptr) {
    IERROR("game_manager::game_manager_initialize()::Camera pointer is invalid");
    return false;
  }
  if (not in_app_settings or in_app_settings == nullptr) {
    IERROR("game_manager::game_manager_initialize()::Settings pointer is invalid");
    return false;
  }
  if (not in_active_map_ptr or in_active_map_ptr == nullptr) {
    IERROR("game_manager::game_manager_initialize()::Map pointer is invalid");
    return false;
  }
  state = (game_manager_system_state *)allocate_memory_linear(sizeof(game_manager_system_state), true);
  if (not state or state == nullptr) {
    IERROR("game_manager::game_manager_initialize()::State allocation failed");
    return false;
  }
  *state = game_manager_system_state();

  if (not save_system_initialize()) {
    IERROR("game_manager::game_manager_initialize()::Save system init returned false");
    return false;
  }
  if (not ability_system_initialize(in_camera_metrics, get_app_settings(), __builtin_addressof(state->game_info))) {
    IERROR("game_manager::game_manager_initialize()::Ability system init returned false");
    return false;
  }
  if (not spawn_system_initialize(in_camera_metrics, __builtin_addressof(state->game_info))) {
    IERROR("game_manager::game_manager_initialize()::Spawn system init returned false");
    return false;
  }
  if (not player_system_initialize(in_camera_metrics, get_app_settings(), __builtin_addressof(state->game_info))) {
    IERROR("game_manager::game_manager_initialize()::Player system init failed");
    return false;
  }
  if (not collectible_manager_initialize(in_camera_metrics, get_app_settings(), const_cast<const tilemap ** const>(in_active_map_ptr), __builtin_addressof(state->game_info))) {
    IERROR("game_manager::game_manager_initialize()::Collectibles system init failed");
    return false;
  }
  if (not sound_system_initialize()) {
    IERROR("game_manager::game_manager_initialize()::Sound system init failed");
    return false;
  }

  state->player_state_static = (*get_default_player());
  state->game_info.player_state_dynamic = get_player_state();
  state->game_info.player_state_static  = __builtin_addressof(state->player_state_static);
  state->game_info.in_spawns            = get_spawns();
  state->game_info.mouse_pos_world      = __builtin_addressof(state->mouse_pos_world);
  state->game_info.mouse_pos_screen     = __builtin_addressof(state->mouse_pos_screen);
  state->game_info.ingame_phase         = __builtin_addressof(state->ingame_phase);
  state->game_info.chosen_traits        = __builtin_addressof(state->chosen_traits);
  state->game_info.loots_on_the_map     = get_loots_pointer();
  state->game_info.current_map_info     = __builtin_addressof(state->stage);
  state->game_info.game_rules           = __builtin_addressof(state->game_rules);
  state->game_info.collected_coins      = __builtin_addressof(state->collected_coins);
  state->game_info.total_spawn_count    = __builtin_addressof(state->total_spawn_count);
  state->game_info.total_boss_count     = __builtin_addressof(state->total_boss_count);
  state->game_info.total_boss_spawned   = __builtin_addressof(state->total_boss_spawned);
  state->game_info.total_play_time      = __builtin_addressof(state->total_play_time);
  state->game_info.play_time            = __builtin_addressof(state->play_time);
  state->game_info.delta_time           = __builtin_addressof(state->delta_time);
  state->game_info.is_win               = __builtin_addressof(state->is_win);
  state->game_info.stage_boss_id        = __builtin_addressof(state->stage_boss_id);
  state->game_info.starter_ability      = __builtin_addressof(state->starter_ability);

  event_register(EVENT_CODE_END_GAME, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, game_manager_on_event);
  event_register(EVENT_CODE_ADD_CURRENCY_COINS, game_manager_on_event);
  event_register(EVENT_CODE_KILL_ALL_SPAWNS, game_manager_on_event);
  event_register(EVENT_CODE_ADD_TO_INVENTORY, game_manager_on_event);

  // Traits
  {
    i32 next_trait_id = 0;
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_HEALTH, "Healty", "Increases your healt a bit", -1, data128(static_cast<i32>(300))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_HEALTH, "Tough", "Increases your healt a fair amouth", -2, data128(static_cast<i32>(800))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_DAMAGE, "Artillery", "Increases your damage a fair amouth", -2, data128(static_cast<i32>(8))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_MOVE_SPEED, "Bunny Hop", "Increases your speed a bit", -1, data128(static_cast<f32>(32))));
    state->traits.push_back(character_trait(next_trait_id++, CHARACTER_STATS_MOVE_SPEED, "Flash",  "Increases your speed a fair amouth", -2, data128(static_cast<f32>(64.f))));
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

  // Runes
  {
    state->default_items.at(ITEM_TYPE_RUNE_DAMAGE_COMMON)       = item_data(ITEM_TYPE_RUNE_DAMAGE_COMMON,       "Damage common",      data128(3.f), ATLAS_TEX_ID_RUNE_DAMAGE_COMMON);
    state->default_items.at(ITEM_TYPE_RUNE_DAMAGE_UNCOMMON)     = item_data(ITEM_TYPE_RUNE_DAMAGE_UNCOMMON,     "Damage uncommon",    data128(8.f), ATLAS_TEX_ID_RUNE_DAMAGE_UNCOMMON);
    state->default_items.at(ITEM_TYPE_RUNE_DAMAGE_RARE)         = item_data(ITEM_TYPE_RUNE_DAMAGE_RARE,         "Damage rare",        data128(5.f), ATLAS_TEX_ID_RUNE_DAMAGE_RARE);
    state->default_items.at(ITEM_TYPE_RUNE_DAMAGE_EPIC)         = item_data(ITEM_TYPE_RUNE_DAMAGE_EPIC,         "Damage epic",        data128(5.f), ATLAS_TEX_ID_RUNE_DAMAGE_EPIC);
    state->default_items.at(ITEM_TYPE_RUNE_RESISTANCE_COMMON)   = item_data(ITEM_TYPE_RUNE_RESISTANCE_COMMON,   "Resistance common",  data128(3.f), ATLAS_TEX_ID_RUNE_RESISTANCE_COMMON);
    state->default_items.at(ITEM_TYPE_RUNE_RESISTANCE_UNCOMMON) = item_data(ITEM_TYPE_RUNE_RESISTANCE_UNCOMMON, "Resistance uncommon",data128(8.f), ATLAS_TEX_ID_RUNE_RESISTANCE_UNCOMMON);
    state->default_items.at(ITEM_TYPE_RUNE_RESISTANCE_RARE)     = item_data(ITEM_TYPE_RUNE_RESISTANCE_RARE,     "Resistance rare",    data128(5.f), ATLAS_TEX_ID_RUNE_RESISTANCE_RARE);
    state->default_items.at(ITEM_TYPE_RUNE_RESISTANCE_EPIC)     = item_data(ITEM_TYPE_RUNE_RESISTANCE_EPIC,     "Resistance epic",    data128(5.f), ATLAS_TEX_ID_RUNE_RESISTANCE_EPIC);
  }
  // Runes

  // Rules
  {
    state->default_game_rules.at(GAME_RULE_SPAWN_MULTIPLIER) = game_rule(
      GAME_RULE_SPAWN_MULTIPLIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_SPAWN_MULTIPLIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_SPAWN_MULTIPLIER_DESCRIPTION),
      Rectangle {2144, 672, 32, 32}, 6, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_PLAY_TIME_MULTIPLIER) = game_rule(
      GAME_RULE_PLAY_TIME_MULTIPLIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_PLAY_TIME_MULTIPLIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_PLAY_TIME_MULTIPLIER_DESCRIPTION), 
      Rectangle {1792, 640, 32, 32}, 6, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_DELTA_TIME_MULTIPLIER) = game_rule(
      GAME_RULE_DELTA_TIME_MULTIPLIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_DELTA_TIME_MULTIPLIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_DELTA_TIME_MULTIPLIER_DESCRIPTION),
      Rectangle {1696, 672, 32, 32}, 6, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_BOSS_MODIFIER) = game_rule(
      GAME_RULE_BOSS_MODIFIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_BOSS_MODIFIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_BOSS_MODIFIER_DESCRIPTION), 
      Rectangle {1728, 800, 32, 32}, 1, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_AREA_UNLOCKER) = game_rule(
      GAME_RULE_AREA_UNLOCKER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_AREA_UNLOCKER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_AREA_UNLOCKER_DESCRIPTION), 
      Rectangle {2144, 672, 32, 32}, 1, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_TRAIT_POINT_MODIFIER) = game_rule(
      GAME_RULE_TRAIT_POINT_MODIFIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_TRAIT_POINT_MODIFIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_TRAIT_POINT_MODIFIER_DESCRIPTION), 
      Rectangle {1600, 640, 32, 32}, 1, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_BONUS_RESULT_MULTIPLIER) = game_rule(
      GAME_RULE_BONUS_RESULT_MULTIPLIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_BONUS_RESULT_MULTIPLIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_BONUS_RESULT_MULTIPLIER_DESCRIPTION), 
      Rectangle {1792, 736, 32, 32}, 6, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_ZOMBIE_LEVEL_MODIFIER) = game_rule(
      GAME_RULE_ZOMBIE_LEVEL_MODIFIER, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_ZOMBIE_LEVEL_MODIFIER), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_ZOMBIE_LEVEL_MODIFIER_DESCRIPTION), 
      Rectangle {1568, 672, 32, 32}, 1, level_curve[0], data128()
    );
    state->default_game_rules.at(GAME_RULE_RESERVED_FOR_FUTURE_USE) = game_rule(
      GAME_RULE_RESERVED_FOR_FUTURE_USE, static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_RESERVED_FOR_FUTURE_USE), static_cast<i32>(LOC_TEXT_INGAME_GAME_RULE_RESERVED_FOR_FUTURE_USE_DESCRIPTION), 
      Rectangle {1568, 672, 32, 32}, 1, level_curve[0], data128()
    );
    for (size_t itr_000 = 1u; itr_000 < state->default_game_rules.size(); itr_000++) {
      game_rule& rule = state->default_game_rules.at(itr_000);
      gm_refresh_game_rule_by_level(&rule, rule.level);
    }
  }
  // Rules

  state->playlist = create_playlist(PLAYLIST_PRESET_INGAME_PLAY_LIST);

  return game_manager_reinit(in_camera_metrics, in_app_settings, in_active_map_ptr);
}
bool game_manager_reinit(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (not in_app_settings or in_app_settings == nullptr or not in_active_map_ptr or in_active_map_ptr == nullptr) {
    IERROR("game_manager::game_manager_reinit()::Recieved pointer(s) is/are invalid");
    return false;
  }
  if (not player_system_initialize(in_camera_metrics, in_app_settings, __builtin_addressof(state->game_info))) {
    IERROR("game_manager::game_manager_reinit()::Player system initialization failed");
    return false;
  }

  if (not parse_or_create_save_data_from_file(SAVE_SLOT_CURRENT_SESSION, save_data(SAVE_SLOT_CURRENT_SESSION, (*get_default_player()), state->default_game_rules, 0))) {
    IERROR("game_manager::game_manager_reinit()::Save system failed to creating/parsing serialization data");
    return false;
  }
  gm_load_game();

  state->game_manager_initialized = true;

  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = in_app_settings;
  state->in_active_map = in_active_map_ptr;
  set_player_state(state->player_state_static);

  state->game_rules = state->game_progression_data->game_rules;
  for (size_t itr_000 = 1u; itr_000 < state->game_rules.size(); itr_000++) {
    game_rule& rule = state->game_rules.at(itr_000);
    gm_refresh_game_rule_by_level(&rule, rule.level);
  }

  return true;
}

void update_game_manager(void) {
  if (not state or state == nullptr) {
    return;
  }
  state->mouse_pos_screen = Vector2 { GetMousePosition().x * get_app_settings()->scale_ratio.at(0), GetMousePosition().y * get_app_settings()->scale_ratio.at(1)};
  state->mouse_pos_world = GetScreenToWorld2D(Vector2 {state->mouse_pos_screen.x,state->mouse_pos_screen.y}, state->in_camera_metrics->handle);
  state->delta_time = delta_time_ingame();
  update_sound_system();

  switch (state->ingame_phase) {
    case INGAME_PLAY_PHASE_CLEAR_ZOMBIES: {
      if (not state->game_info.player_state_dynamic->is_dead) {
        gm_update_player();
        update_collectible_manager();

        update_spawns(state->game_info.player_state_dynamic->position);
        generate_in_game_info();
        update_abilities(__builtin_addressof(get_player_state()->ability_system));

        const Character2D * const boss = get_spawn_by_id(state->stage_boss_id);
        if (boss) {
          f32 boss_health_perc = static_cast<f32>(boss->health_current) / static_cast<f32>(boss->health_max);
          event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context(static_cast<f32>(PRG_BAR_ID_BOSS_HEALTH), boss_health_perc));
        }
        if ( state->total_boss_spawned < state->total_boss_count) {
          spawn_boss();
        }
        if (state->play_time <= 0.f) {
          gm_end_game(true, true);
        }
      }
      else {
        gm_end_game(true, false);
        return;
      }
      break;
    }
    case INGAME_PLAY_PHASE_RESULTS: { 
      
      break;
    }
    default: {
      IWARN("game_manager::update_game_manager()::Unsupported in-game phase");
      gm_end_game(false, false);
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
    case INGAME_PLAY_PHASE_CLEAR_ZOMBIES: {
      if (not state->game_info.player_state_dynamic->is_dead) {
        render_player();
        render_collectible_manager();
        render_abilities(__builtin_addressof(get_player_state()->ability_system));
        render_spawns();
      
        //for (size_t itr_000 = 0; itr_000 < (*state->in_active_map)->collisions.size() ; ++itr_000) {
        //  DrawRectangleLinesEx((*state->in_active_map)->collisions.at(itr_000).dest, 2.f, BLUE);
        //}
      }
      else {
        render_player();
        render_spawns();
      }
      return;
    }
    case INGAME_PLAY_PHASE_RESULTS: {
      
      return;
    }
    default: {
      IWARN("game_manager::render_game()::Unsupported in-game phase");
      gm_end_game(false, false);
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
      return;
    }
  }

  IERROR("game_manager::render_game()::Function ended unexpectedly");
  gm_end_game(false, false);
  event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
}

// OPS
/**
 * @brief Start game before, then upgrade the stat
 */
bool gm_start_game(worldmap_stage stage) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_start_game()::State is invalid");
    return false;
  }
  player_state * const _player = get_player_state();
  if (not _player or _player == nullptr) {
    IERROR("game_manager::gm_start_game()::Player state is invalid");
    return false;
  }
  (*_player) = state->player_state_static;

  state->stage = stage;
  std::array<character_stat, CHARACTER_STATS_MAX> * const _stat_array_ptr = __builtin_addressof(_player->stats);

  for (size_t itr_000 = 0; itr_000 < state->chosen_traits.size(); ++itr_000) {
    const character_trait * const _trait_ptr = __builtin_addressof(state->chosen_traits.at(itr_000));
    if (_trait_ptr->type < 0 && _trait_ptr->type >= CHARACTER_STATS_MAX) {
      IWARN("game_manager::gm_start_game()::Trait type is out of bound");
      continue;
    }
    character_stat * const _stat_ptr = __builtin_addressof(_stat_array_ptr->at(_trait_ptr->type));
    
    game_manager_set_stat_trait_value_by_level(_stat_ptr, _trait_ptr->ingame_ops);
  }

  _add_ability(state->starter_ability);
  ability *const player_starter_ability = __builtin_addressof(_player->ability_system.abilities.at(state->starter_ability));
  upgrade_ability(player_starter_ability);
  refresh_ability(player_starter_ability);

  #warning "Uncomment later"
  //populate_map_with_spawns(stage.total_spawn_count);
  //if (state->game_info.in_spawns->size() <= 0) {
  //  return false;
  //}

  _player->health_current = _player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3];
  _player->health_perc = static_cast<f32>(_player->health_current) / static_cast<f32>(_player->stats.at(CHARACTER_STATS_HEALTH).buffer.i32[3]);
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_EXPERIANCE, (f32)state->game_info.player_state_dynamic->exp_perc));
  event_fire(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, event_context((f32)PRG_BAR_ID_PLAYER_HEALTH, (f32)state->game_info.player_state_dynamic->health_perc));

  state->ingame_phase = INGAME_PLAY_PHASE_CLEAR_ZOMBIES;
  
  reset_ingame_info();

  set_ingame_delta_time_multiplier(state->game_rules.at(GAME_RULE_DELTA_TIME_MULTIPLIER).mm_ex.f32[3]);

  state->total_spawn_count = state->stage.total_spawn_count * state->game_rules.at(GAME_RULE_SPAWN_MULTIPLIER).mm_ex.f32[3];
  state->total_play_time   = state->stage.stage_duration * state->game_rules.at(GAME_RULE_PLAY_TIME_MULTIPLIER).mm_ex.f32[3];
  state->total_boss_count  = state->stage.total_boss_count + state->game_rules.at(GAME_RULE_BOSS_MODIFIER).mm_ex.i32[3];

  state->play_time = (*state->game_info.total_play_time);
  state->playlist.media_play(__builtin_addressof(state->playlist));
  return true;
}
void gm_end_game(bool wait_for_results, bool is_win) {
  clean_up_spawn_state();
  state->playlist.media_stop(__builtin_addressof(state->playlist));
  collectible_manager_state_clear();

  set_player_state(player_state());
  state->is_win = is_win;
  if (wait_for_results) {

    state->ingame_phase = INGAME_PLAY_PHASE_RESULTS;
    //event_fire(EVENT_CODE_PLAY_SOUND, event_context()); // TODO: play win or lose sound
    event_fire(EVENT_CODE_CAMERA_SET_ZOOM_TARGET, event_context(static_cast<f32>(ZOOM_CAMERA_GAME_RESULTS)));
  }
  else {
    state->ingame_phase = INGAME_PLAY_PHASE_UNDEFINED;
  }
  
  gm_save_game();
}
void gm_save_game(void) {
  state->game_progression_data->currency_coins_player_have = state->collected_coins;
  state->game_progression_data->player_data = state->player_state_static;
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void gm_load_game(void) {
  state->player_state_static = *(get_default_player());
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);
  state->game_rules = state->default_game_rules;

  for (size_t itr_000 = 0u; itr_000 < state->player_state_static.stats.size(); ++itr_000) {
    i32 level = state->game_progression_data->player_data.stats.at(itr_000).current_level;
    const character_stat& stat = state->player_state_static.stats.at(itr_000);
    if (level - stat.base_level >= 0 and level < MAX_PLAYER_LEVEL) {
      set_static_player_state_stat(static_cast<character_stat_id>(itr_000), state->game_progression_data->player_data.stats.at(itr_000).current_level);
    }
    else {
      set_static_player_state_stat(static_cast<character_stat_id>(itr_000), state->player_state_static.stats.at(itr_000).base_level);
    }
  }

  for (size_t itr_000 = 1u; itr_000 < state->game_rules.size() - 1u; ++itr_000) { // First is GAME_RULE_UNDEFINED and last one GAME_RULE_MAX
    game_rule& base_rule = state->game_rules.at(itr_000);
    game_rule& saved_rule = state->game_progression_data->game_rules.at(itr_000);
    if (not gm_refresh_game_rule_by_level(__builtin_addressof(base_rule), saved_rule.level)) {
      gm_refresh_game_rule_by_level(__builtin_addressof(base_rule), state->default_game_rules.at(itr_000).base_level);
    }
  }
  state->player_state_static.inventory = state->game_progression_data->player_data.inventory;
  for (player_inventory_slot& slot : state->player_state_static.inventory) {
    const item_data& item = state->default_items.at(slot.item_type);
    slot.slot_id = state->next_inventory_slot_id++;
    slot.display_name = item.display_name;
  }
  set_player_state(state->player_state_static);
}
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check) {
  switch (coll_check) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      Rectangle rect = Rectangle { (f32)coll_data.i16[0],  (f32)coll_data.i16[1], (f32)coll_data.i16[2], (f32)coll_data.i16[3] };

      for (size_t itr_000 = 0; itr_000 < state->game_info.in_spawns->size(); ++itr_000) {
        if (!state->game_info.in_spawns->at(itr_000).is_dead){
          if (CheckCollisionRecs(state->game_info.in_spawns->at(itr_000).collision, rect)) {
            const Character2D *const spw = __builtin_addressof(state->game_info.in_spawns->at(itr_000));
            damage_deal_result result = damage_spawn(spw->character_id, damage);
            if (result.type == DAMAGE_DEAL_RESULT_SUCCESS) {
              event_fire(EVENT_CODE_SPAWN_COMBAT_FEEDBACK_FLOATING_TEXT, event_context(
                static_cast<f32>(spw->collision.x + spw->collision.width * .5f), static_cast<f32>(spw->collision.y - spw->collision.height * .15f),
                static_cast<f32>(result.inflicted_damage)
              ));
            }
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
      IWARN("game_manager::gm_damage_spawn_if_collide()::Unsupported collision check type");
      return;
    }
  }

  IERROR("game_manager::gm_damage_spawn_if_collide()::Function terminated unexpectedly");
}
void gm_damage_player_if_collide(data128 coll_data, i32 damage, collision_type coll_check) {
  switch (coll_check) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      Rectangle rect = Rectangle { (f32)coll_data.i16[0],  (f32)coll_data.i16[1], (f32)coll_data.i16[2], (f32)coll_data.i16[3] };

      if (CheckCollisionRecs(state->game_info.player_state_dynamic->collision, rect)) {
        player_take_damage(damage);
        event_fire(EVENT_CODE_BEGIN_CAMERA_SHAKE, event_context());
      }
      return; 
    }
    
    case COLLISION_TYPE_CIRCLE_RECTANGLE: { 
      Vector2 circle_center = Vector2 { (f32)coll_data.i16[0],  (f32)coll_data.i16[1] };
      f32 circle_radius = (f32) coll_data.i16[2];

      if (CheckCollisionCircleRec(circle_center, circle_radius, state->game_info.player_state_dynamic->collision)) {
        player_take_damage(damage);
        event_fire(EVENT_CODE_BEGIN_CAMERA_SHAKE, event_context());
      }
      return; 
    }

    default: IWARN("game_manager::gm_damage_player_if_collide()::Unsupported collision check type"); return;
  }

  IERROR("game_manager::gm_damage_player_if_collide()::Function terminated unexpectedly");
}
void populate_map_with_spawns(i32 min_count) {
  if (not state or state == nullptr) {
    IERROR("game_manager::populate_map_with_spawns()::State is not valid");
    return;
  }
  if (not state->game_info.in_spawns or state->game_info.in_spawns == nullptr) {
    IWARN("game_manager::populate_map_with_spawns()::Spawns pointer is not valid");
    return;
  }
  i32 spawn_trying_limit = SPAWN_TRYING_LIMIT;
  for (i32 itr_000 = 0; itr_000 < min_count && (spawn_trying_limit <= SPAWN_TRYING_LIMIT && spawn_trying_limit != 0); ) 
  {
    Vector2 position = Vector2 {
      static_cast<f32>(get_random((i32)state->stage.spawning_areas.at(0).x, (i32)state->stage.spawning_areas.at(0).x + state->stage.spawning_areas.at(0).width)),
      static_cast<f32>(get_random((i32)state->stage.spawning_areas.at(0).y, (i32)state->stage.spawning_areas.at(0).y + state->stage.spawning_areas.at(0).height))
    };
    if (spawn_character(Character2D(
        SPAWN_TYPE_BROWN, //static_cast<i32>(get_random(SPAWN_TYPE_UNDEFINED+1, SPAWN_TYPE_MAX-2) >= 0),
        static_cast<i32>(state->game_info.player_state_dynamic->level), 
        static_cast<i32>(get_random(0, 100)), 
        position
      ))) { 
        ++itr_000; 
    }
    else { 
      --spawn_trying_limit; 
    }
  }
}
i32 spawn_boss(void) {
  if (not state or state == nullptr) {
    IERROR("game_manager::spawn_boss()::State is invalid");
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
      state->stage_boss_id = boss_id;
      state->total_boss_spawned++;
      return boss_id;
    }
    else {
      continue;
    }
  }
  return -1;
}
void currency_coins_add(i32 value) {
  state->game_progression_data->currency_coins_player_have += value;
}
void set_starter_ability(ability_id _id) {
  if (not state or state == nullptr) {
    IERROR("game_manager::set_starter_ability()::State is invalid");
    return;
  }
  if (_id <= ABILITY_ID_UNDEFINED or _id >= ABILITY_ID_MAX) {
    IWARN("game_manager::set_starter_ability()::Ability id is out of bound");
    return;
  }

  state->starter_ability = _id;
}
void generate_in_game_info(void) {
  if (not state or state == nullptr) {
    IERROR("game_manager::generate_in_game_info()::State is not valid");
    return;
  }
  if (not state->game_info.in_spawns or state->game_info.in_spawns == nullptr) {
    IWARN("game_manager::generate_in_game_info()::Spawn list is invalid");
    return;
  }
  Vector2 player_position = state->game_info.player_state_dynamic->position;
  const Character2D * spw_nearest = nullptr;

  f32 dist_cache = F32_MAX;
  for (const Character2D& spw : (*state->game_info.in_spawns)) {
    
    f32 dist = vec2_distance(player_position, spw.position);
    if (dist < dist_cache and dist < MAX_NEAREST_SPAWN_DISTANCE) {
      spw_nearest = __builtin_addressof(spw);
      dist_cache = dist;
    }
  }

  state->game_info.nearest_spawn = spw_nearest;
  state->play_time -= state->delta_time;
}
void reset_ingame_info(void) {
  if (not state or state == nullptr) {
    IERROR("game_manager::reset_ingame_info()::State is invalid");
    return;
  }
  state->collected_coins = 0;
  state->play_time = 0.f;
  state->is_win = false;
  state->stage_boss_id = INVALID_IDI32;
  state->starter_ability = ABILITY_ID_UNDEFINED;
}
void gm_update_player(void) {
  if (state->game_info.player_state_dynamic and state->game_info.player_state_dynamic != nullptr and not state->game_info.player_state_dynamic->is_dead) {
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

    for (size_t itr_000 = 0; itr_000 < ABILITY_ID_MAX; ++itr_000) {
      if (state->game_info.player_state_dynamic->ability_system.abilities.at(itr_000).is_active) {
        get_player_state()->ability_system.abilities.at(itr_000).ability_play_time += state->delta_time;
      }
    }
  }
}
void gm_add_to_inventory(item_type _item_type) {
  if (_item_type > ITEM_TYPE_MAX or _item_type < ITEM_TYPE_UNDEFINED) {
    IWARN("game_manager::gm_add_to_inventory()::Item id is out of bounds");
    return;
  }
  std::vector<player_inventory_slot>& inventory = state->player_state_static.inventory;

  auto find_or_create = [&inventory, &_item_type](item_type type, size_t start_with = 0) {
    for (size_t itr_000 = start_with; itr_000 < inventory.size(); itr_000++) {
      player_inventory_slot& slot = inventory.at(itr_000);
      if (slot.item_type == type) {
        slot.amount++;
        return;
      }
    }
    player_inventory_slot& slot = inventory.emplace_back(player_inventory_slot());
    slot.slot_id = state->next_inventory_slot_id++;
    slot.item_type = _item_type;
    slot.display_name = state->default_items.at(_item_type).display_name.c_str();
    slot.amount++;
  };
  switch (_item_type) {
    case ITEM_TYPE_EXPERIENCE: return;
    case ITEM_TYPE_COIN: return;
    case ITEM_TYPE_HEALTH_FRAGMENT: return;
    case ITEM_TYPE_CHEST: return;
    case ITEM_TYPE_RUNE_DAMAGE_COMMON: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_DAMAGE_UNCOMMON: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_DAMAGE_RARE: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_DAMAGE_EPIC: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_RESISTANCE_COMMON: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_RESISTANCE_UNCOMMON: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_RESISTANCE_RARE: {
      find_or_create(_item_type);
      return;
    }
    case ITEM_TYPE_RUNE_RESISTANCE_EPIC: {
      find_or_create(_item_type);
      return;
    }
    default: {
      IWARN("game_manager::gm_add_to_inventory()::Unsupported type");
      return;
    }
  }
  IWARN("game_manager::gm_add_to_inventory()::Function ended unexpectedly");
}
void gm_chosen_traits_add(const character_trait *const _trait) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_add_chosen_trait()::State is invalid");
    return;
  }
  if (not _trait or _trait == nullptr) {
    IERROR("game_manager::gm_add_chosen_trait()::Trait is invalid");
    return;
  }
  state->chosen_traits.push_back( (*_trait) );
}
void gm_chosen_traits_erase(size_t index) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_erase_chosen_trait()::State is invalid");
    return;
  }
  if (index >= state->chosen_traits.size()) {
    IERROR("game_manager::gm_erase_chosen_trait()::Index out of bound");
    return;
  }
  state->chosen_traits.erase(state->chosen_traits.begin() + index);
}
void gm_chosen_traits_clear(void) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_chosen_traits_clear()::State is invalid");
    return;
  }
  state->chosen_traits.clear();
}
// OPS

// GET / SET
i32 get_currency_coins_total(void) {
  if (not state->game_progression_data or state->game_progression_data == nullptr) {
    return -1;
  }
  return state->game_progression_data->currency_coins_player_have;
}
bool get_b_player_have_upgrade_points(void) {
  return state->game_info.player_state_dynamic->is_player_have_ability_upgrade_points;
}
void set_dynamic_player_have_ability_upgrade_points(bool _b) {
  get_player_state()->is_player_have_ability_upgrade_points = _b;
}
const character_stat* get_static_player_state_stat(character_stat_id stat) {
  if (stat >= CHARACTER_STATS_MAX || stat <= CHARACTER_STATS_UNDEFINED) {
    return nullptr;
  }
  
  return __builtin_addressof(state->player_state_static.stats.at(stat));
}
void set_static_player_state_stat(character_stat_id stat_id, i32 level) {
  if (not state or state == nullptr) {
    IERROR("game_manager::set_static_player_state_stat()::State is not valid");
    return;
  }
  if (stat_id >= CHARACTER_STATS_MAX or stat_id <= CHARACTER_STATS_UNDEFINED) {
    return;
  }
  character_stat * _stat_ptr = __builtin_addressof(state->player_state_static.stats.at(stat_id));
  
  gm_refresh_stat_by_level(_stat_ptr, level);
}
void gm_set_game_rule_level_by_id(game_rule_id rule_id, i32 level) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_set_game_rule_level_by_id()::State is not valid");
    return;
  }
  if (rule_id < GAME_RULE_UNDEFINED or rule_id > GAME_RULE_MAX) {
    IWARN("game_manager::gm_set_game_rule_level_by_id()::Rule id is out of bound");
    return;
  }
  if (level < 0 or level > MAX_GAME_RULE_LEVEL) {
    IWARN("game_manager::gm_set_game_rule_level_by_id()::Rule level is out of bound");
    return;
  }

  gm_refresh_game_rule_by_level(__builtin_addressof(state->game_rules.at(rule_id)), level);
}
const ingame_info* gm_get_ingame_info(void){
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_get_ingame_info()::State is not valid");
    return nullptr;
  }
  return __builtin_addressof(state->game_info); 
}
const std::vector<character_trait>* gm_get_character_traits_all(void) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_get_character_traits()::State is not valid");
    return nullptr;
  }

  return __builtin_addressof(state->traits);
}
void gm_refresh_stat_by_level(character_stat* stat, i32 level) {
  if (not state or state == nullptr) {
    IERROR("game_manager::game_manager_set_stats()::State is not valid");
    return;
  }
  if (not stat or stat == nullptr) {
    IWARN("game_manager::game_manager_set_stats()::stat is not valid");
    return;
  }
  if (stat->id >= CHARACTER_STATS_MAX || stat->id <= CHARACTER_STATS_UNDEFINED) {
    IWARN("game_manager::gm_refresh_stat_by_level()::Stat id is out of bound");
    return;
  }

  i32 next_curve_value = level_curve[level];
  stat->upgrade_cost = level_curve[ (level+1) - stat->base_level ];
  stat->current_level = level;
  switch (stat->id) {
    case CHARACTER_STATS_HEALTH:{
      const i32 value = next_curve_value;

      stat->buffer.i32[1] = value;
      stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
      return;
    }
    case CHARACTER_STATS_HP_REGEN:{ 
      const i32 value = next_curve_value / 100.f;

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
    case CHARACTER_STATS_PROJECTILE_AMOUNT:{
      const i32 value = stat->current_level;

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
      IWARN("game_manager::game_manager_set_stats()::Unsuppported stat id");
      return;
    }
  }

  IERROR("game_manager::game_manager_set_stats()::Function ended unexpectedly");
}
bool gm_refresh_game_rule_by_level(game_rule* rule, i32 level) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_refresh_game_rule_by_level()::State is not valid");
    return false;
  }
  if (not rule or rule == nullptr) {
    IWARN("game_manager::gm_refresh_game_rule_by_level()::rule is not valid");
    return false;
  }
  if (rule->id <= GAME_RULE_UNDEFINED or rule->id >= GAME_RULE_MAX) {
    IWARN("game_manager::gm_refresh_game_rule_by_level()::Rule id is out of bound");
    return false;
  }
  if (level < 0 or level - rule->base_level < 0 or level - rule->base_level > MAX_GAME_RULE_LEVEL) {
    IWARN("game_manager::gm_refresh_game_rule_by_level()::Level is out of bound");
    return false;
  }

  i32 next_curve_value = level_curve[static_cast<size_t>(level)];
  rule->upgrade_cost = level_curve[static_cast<size_t>(level - rule->base_level + 1u)];
  rule->level = level;

  switch (rule->id) {
    case GAME_RULE_SPAWN_MULTIPLIER:{
      const f32 value = next_curve_value / 10000.f;

      rule->mm_ex.f32[1] = value;
      rule->mm_ex.f32[3] = rule->mm_ex.f32[0] + rule->mm_ex.f32[1] + rule->mm_ex.f32[2];
      return true;
    }
    case GAME_RULE_PLAY_TIME_MULTIPLIER:{ 
      const f32 value = next_curve_value / 10000.f;

      rule->mm_ex.f32[1] = value;
      rule->mm_ex.f32[3] = rule->mm_ex.f32[0] + rule->mm_ex.f32[1] + rule->mm_ex.f32[2];
      return true;
    }
    case GAME_RULE_DELTA_TIME_MULTIPLIER:{
      const f32 value = next_curve_value / 10000.f;

      rule->mm_ex.f32[1] = value;
      rule->mm_ex.f32[3] = rule->mm_ex.f32[0] + rule->mm_ex.f32[1] + rule->mm_ex.f32[2];
      return true;
    }
    case GAME_RULE_BOSS_MODIFIER:{
      const i32 value = level;

      rule->mm_ex.i32[1] = value;
      rule->mm_ex.i32[3] = rule->mm_ex.i32[0] + rule->mm_ex.i32[1] + rule->mm_ex.i32[2];
      return true;
    }
    case GAME_RULE_AREA_UNLOCKER:{
      const i32 value = level;

      rule->mm_ex.i32[1] = value;
      rule->mm_ex.i32[3] = rule->mm_ex.i32[0] + rule->mm_ex.i32[1] + rule->mm_ex.i32[2];
      return true;
    }
    case GAME_RULE_TRAIT_POINT_MODIFIER:{
      const i32 value = next_curve_value / 100.f;

      rule->mm_ex.i32[1] = value;
      rule->mm_ex.i32[3] = rule->mm_ex.i32[0] + rule->mm_ex.i32[1] + rule->mm_ex.i32[2];
      return true;
    }
    case GAME_RULE_BONUS_RESULT_MULTIPLIER:{
      const f32 value = next_curve_value / 10000.f;

      rule->mm_ex.f32[1] = value;
      rule->mm_ex.f32[3] = rule->mm_ex.f32[0] + rule->mm_ex.f32[1] + rule->mm_ex.f32[2];
      return true;
    }
    case GAME_RULE_ZOMBIE_LEVEL_MODIFIER:{
      const i32 value = level;

      rule->mm_ex.i32[1] = value;
      rule->mm_ex.i32[3] = rule->mm_ex.i32[0] + rule->mm_ex.i32[1] + rule->mm_ex.i32[2];
      return true;
    }
    case GAME_RULE_RESERVED_FOR_FUTURE_USE:{
      #warning "GAME_RULE_RESERVED_FOR_FUTURE_USE"

      return true;
    }
    default:{
      IWARN("game_manager::gm_refresh_game_rule_by_level()::Unsuppported stat id");
      return false;
    }
  }
  IERROR("game_manager::gm_refresh_game_rule_by_level()::Function ended unexpectedly");
  return false;
}
void game_manager_set_stat_trait_value_by_level(character_stat* stat, data128 value) {
  if (not state or state == nullptr) {
    IERROR("game_manager::game_manager_sum_stat_value()::State is not valid");
    return;
  }
  if (not stat or stat == nullptr) {
    IWARN("game_manager::game_manager_sum_stat_value()::stat is not valid");
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
    case CHARACTER_STATS_PROJECTILE_AMOUNT:{
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
      IWARN("game_manager::game_manager_sum_stat_value()::Unsuppported stat id");
      break;
    }
  }
}
// GET / SET

// Exposed functions
const ability * _get_ability(ability_id _id) {
  if (_id <= ABILITY_ID_UNDEFINED or _id >= ABILITY_ID_MAX) {
    IWARN("game_manager::_get_ability()::Ability id is out of bound");
    return nullptr;
  }
  return get_ability(_id);
}
const std::array<ability, ABILITY_ID_MAX> * _get_all_abilities(void) {
  return get_all_abilities();
}
const Character2D * _get_spawn_by_id(i32 _id) {
  return get_spawn_by_id(_id);
}
player_state * gm_get_player_state(void) {
  return get_player_state();
}
bool _add_ability(ability_id _id) {
  if (_id <= ABILITY_ID_UNDEFINED or _id >= ABILITY_ID_MAX) {
    IWARN("game_manager::_add_ability()::Ability id is out of bound");
    return false;
  }
  ability abl = (*get_ability(_id));
  ability_play_system& system = get_player_state()->ability_system;

  abl.p_owner = get_player_state();
  abl.is_initialized = true;
  abl.proj_count += state->game_info.player_state_dynamic->stats.at(CHARACTER_STATS_PROJECTILE_AMOUNT).buffer.i32[3];
  abl.is_active = true;
  refresh_ability(__builtin_addressof(abl));
  system.abilities.at(_id) = abl;
  return true;
}
bool _upgrade_ability(ability* abl) {
  if (not abl or abl == nullptr or not abl->is_initialized) {
    IWARN("game_manager::_upgrade_ability::Recieved ability has not initialized yet");
    return false;
  }
  upgrade_ability(abl);
  refresh_ability(abl);

  return true;
}
ability _get_next_level(ability abl) {
  if (not abl.is_initialized or abl.id <= ABILITY_ID_UNDEFINED or abl.id >= ABILITY_ID_MAX) {
    IWARN("game_manager::_get_next_level()::Recieved ability has not initialized yet");
    return ability();
  }
  return get_next_level(abl);
}
void _set_player_position(Vector2 position) {
  get_player_state()->position = position;
  get_player_state()->collision.x = position.x - state->game_info.player_state_dynamic->collision.width * .5f;
  get_player_state()->collision.y = position.y - state->game_info.player_state_dynamic->collision.height * .5f;
}
// Exposed functions


bool game_manager_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_END_GAME: {
      gm_end_game(static_cast<bool>(context.data.i32[0]), static_cast<bool>(context.data.i32[1]));
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
    case EVENT_CODE_KILL_ALL_SPAWNS: {
      std::vector<Character2D>::const_iterator spw_iter = state->game_info.in_spawns->begin();
      for (; spw_iter != state->game_info.in_spawns->end(); spw_iter++) {
        damage_spawn(spw_iter->character_id, spw_iter->health_max);
      }
      return true;
    }
    case EVENT_CODE_ADD_CURRENCY_COINS: {
      state->game_info.collected_coins += context.data.i32[0];
      return true;
    }
    case EVENT_CODE_ADD_TO_INVENTORY: {
      gm_add_to_inventory(static_cast<item_type>(context.data.i32[0]));
    }
    default: {
      IWARN("game_manager::game_manager_on_event()::Unsuppported code.");
      return false;
    }
  }
  IERROR("game_manager::game_manager_on_event()::Fire event ended unexpectedly");
  return false;
}

#undef SPAWN_TRYING_LIMIT
#undef GET_BOSS_LEVEL
#undef GET_BOSS_SCALE
#undef GET_BOSS_SPEED
#undef ZOOM_CAMERA_GAME_RESULTS
