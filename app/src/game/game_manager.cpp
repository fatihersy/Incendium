#include "game_manager.h"
#include <cmath>
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
#include "game/user_interface.h"
#include "game/resource.h"

struct game_manager_system_state {
  tilemap ** in_active_map;
  save_data * game_progression_data;
  const camera_metrics * in_camera_metrics;
  const app_settings * in_app_settings;

  worldmap_stage stage;
  ingame_play_phases ingame_phase;
  player_state player_state_static;
  std::vector<player_inventory_slot> player_inventory;
  ingame_info game_info;
  Vector2 mouse_pos_world {};
  Vector2 mouse_pos_screen {};
  std::vector<character_trait> traits;
  std::vector<character_trait> chosen_traits;
  std::array<game_rule, GAME_RULE_MAX> game_rules;
  i32 collected_coins {};
  i32 total_spawn_count {};
  i32 total_boss_count {};
  i32 total_boss_spawned {};
  f32 total_play_time {};
  f32 play_time {};
  f32 delta_time {};
  bool is_win {};
  i32 stage_boss_id;
  ability_id starter_ability;
  
  playlist_control_system_state playlist;
  
  std::array<game_rule, GAME_RULE_MAX> default_game_rules;
  std::array<item_data, ITEM_TYPE_MAX> default_items;
  std::array<sigil_slot, SIGIL_SLOT_MAX> sigil_slots;

  i32 next_inventory_slot_id {};
  i32 next_item_id {};

  bool game_manager_initialized {};

  game_manager_system_state(void) {
    this->in_active_map = nullptr;
    this->game_progression_data = nullptr;
    this->in_camera_metrics = nullptr;
    this->in_app_settings = nullptr;
    this->ingame_phase = INGAME_PLAY_PHASE_UNDEFINED;
    this->stage_boss_id = INVALID_IDI32;
    this->starter_ability = ABILITY_ID_UNDEFINED;
  }
};

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

#define FIRST_UPGRADABLE_GAME_RULE GAME_RULE_SPAWN_MULTIPLIER
#define LAST_UPGRADABLE_GAME_RULE GAME_RULE_RESERVED_FOR_FUTURE_USE

bool game_manager_on_event(i32 code, event_context context);
bool game_manager_reinit(void);
void populate_map_with_spawns(i32 min_count);
i32 spawn_boss(void);
void generate_in_game_info(void);
void gm_set_character_stat_trait_value(character_stat& stat, data128 value);
void gm_set_game_rule_trait_value(game_rule& stat, data128 value);
void reset_ingame_info(void);
void gm_update_player(void);
void set_static_player_state_stat(character_stat_id stat_id, i32 level);
void gm_refresh_stat_by_level(character_stat* stat, i32 level);
[[__nodiscard__]] bool player_state_rebuild(void);
i32 gm_get_sigil_upgrade_soul_requirement(i32 level);

bool game_manager_initialize(const camera_metrics * in_camera_metrics, const app_settings * in_app_settings, tilemap ** const in_active_map_ptr) {
  if (state and state != nullptr) {
    return game_manager_reinit();
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
    auto add_trait = [&](character_trait_type _trait_type, i32 _context_id, i32 _loc_display_text, i32 loc_desc_text, i32 point, data128 data) {
      state->traits.push_back(character_trait(next_trait_id++, _trait_type, _context_id, _loc_display_text, loc_desc_text, point, data));
    };
    // Positive Traits
    {
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_MOVE_SPEED, LOC_TEXT_PLAYER_TRAIT_ALACRITY, LOC_TEXT_PLAYER_TRAIT_ALACRITY_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_HEALTH, LOC_TEXT_PLAYER_TRAIT_FORTITUDE, LOC_TEXT_PLAYER_TRAIT_FORTITUDE_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_OVERALL_DAMAGE, LOC_TEXT_PLAYER_TRAIT_POWER, LOC_TEXT_PLAYER_TRAIT_POWER_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_OVERALL_LUCK, LOC_TEXT_PLAYER_TRAIT_FORTUNE, LOC_TEXT_PLAYER_TRAIT_FORTUNE_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_CRITICAL_CHANCE, LOC_TEXT_PLAYER_TRAIT_KEEN, LOC_TEXT_PLAYER_TRAIT_KEEN_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_DROP_RATE, LOC_TEXT_PLAYER_TRAIT_SERENDIPITY, LOC_TEXT_PLAYER_TRAIT_SERENDIPITY_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_HP_REGEN, LOC_TEXT_PLAYER_TRAIT_VIGOR, LOC_TEXT_PLAYER_TRAIT_VIGOR_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_CONDITION_DURATION, LOC_TEXT_PLAYER_TRAIT_PERSISTENCE, LOC_TEXT_PLAYER_TRAIT_PERSISTENCE_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_DAMAGE_OVER_TIME, LOC_TEXT_PLAYER_TRAIT_AFFLICTION, LOC_TEXT_PLAYER_TRAIT_AFFLICTION_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_BASIC_ATTACK_SPEED, LOC_TEXT_PLAYER_TRAIT_FRENZY, LOC_TEXT_PLAYER_TRAIT_FRENZY_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_BASIC_ATTACK_DAMAGE, LOC_TEXT_PLAYER_TRAIT_BRUTALITY, LOC_TEXT_PLAYER_TRAIT_BRUTALITY_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_CRITICAL_DAMAGE, LOC_TEXT_PLAYER_TRAIT_DEVASTATOR, LOC_TEXT_PLAYER_TRAIT_DEVASTATOR_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_DAMAGE_REDUCTION, LOC_TEXT_PLAYER_TRAIT_RESILIENCE, LOC_TEXT_PLAYER_TRAIT_RESILIENCE_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_AOE, LOC_TEXT_PLAYER_TRAIT_AMPLITUDE, LOC_TEXT_PLAYER_TRAIT_AMPLITUDE_DESC, 
        -1, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_MOVE_SPEED, LOC_TEXT_PLAYER_TRAIT_EXPEDITION, LOC_TEXT_PLAYER_TRAIT_EXPEDITION_DESC, 
        -2, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_REWARD_MODIFIER, LOC_TEXT_PLAYER_TRAIT_TRIBUTE, LOC_TEXT_PLAYER_TRAIT_TRIBUTE_DESC, 
        -2, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_DAMAGE_DEFERRAL, LOC_TEXT_PLAYER_TRAIT_STAGGER, LOC_TEXT_PLAYER_TRAIT_STAGGER_DESC, 
        -2, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_SIGIL_EFFECTIVENESS, LOC_TEXT_PLAYER_TRAIT_SIGILIC, LOC_TEXT_PLAYER_TRAIT_SIGILIC_DESC, 
        -2, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_VITAL_SIGIL_EFFECTIVENESS, LOC_TEXT_PLAYER_TRAIT_VITALITY_SYNERGY, LOC_TEXT_PLAYER_TRAIT_VITALITY_SYNERGY_DESC, 
        -2, data128(static_cast<f32>(00.00f))
      );
      add_trait(CHARACTER_TRAIT_CHARACTER_STAT, CHARACTER_STATS_LETAL_SIGIL_EFFECTIVENESS, LOC_TEXT_PLAYER_TRAIT_LETHAL_SYNERGY, LOC_TEXT_PLAYER_TRAIT_LETHAL_SYNERGY_DESC, 
        -2, data128(static_cast<f32>(00.00f))
      );
    }
    // Positive Traits

    // Negavite Traits
    {
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_BASIC_ATTACKS_REQUIRE_STAMINA, LOC_TEXT_PLAYER_TRAIT_EXERTION, LOC_TEXT_PLAYER_TRAIT_EXERTION_DESC, 1, data128());
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_JUMP_REQUIRE_STAMINA, LOC_TEXT_PLAYER_TRAIT_HOBBLED, LOC_TEXT_PLAYER_TRAIT_HOBBLED_DESC,            1, data128());
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_LOOTING_DISTANCE, LOC_TEXT_PLAYER_TRAIT_MYOPIA, LOC_TEXT_PLAYER_TRAIT_MYOPIA_DESC,                  1, data128(0.6f));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_ZOMBIE_BITE_CONDITIONS, LOC_TEXT_PLAYER_TRAIT_HEMOPHILIA, LOC_TEXT_PLAYER_TRAIT_HEMOPHILIA_DESC,    1, 
        data128(GAME_RULE_ZOMBIE_BITE_CONDITION_BLEED)
      );
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_NEAR_ZOMBIE_CONDITIONS, LOC_TEXT_PLAYER_TRAIT_DREAD, LOC_TEXT_PLAYER_TRAIT_DREAD_DESC,
        1, data128(GAME_RULE_NEAR_ZOMBIE_CONDITION_LOSE_SPEED)
      );
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_ZOMBIE_BITE_CONDITIONS, LOC_TEXT_PLAYER_TRAIT_INFECTION, LOC_TEXT_PLAYER_TRAIT_INFECTION_DESC,
        1, data128(GAME_RULE_ZOMBIE_BITE_CONDITION_INFECT)
      );
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_ITEM_SLOT_COUNT,                 LOC_TEXT_PLAYER_TRAIT_CLUMSY,          LOC_TEXT_PLAYER_TRAIT_CLUMSY_DESC,          1, data128(4));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_YOUR_SKILLS_DAMAGES_YOU,         LOC_TEXT_PLAYER_TRAIT_BACKLASH,        LOC_TEXT_PLAYER_TRAIT_BACKLASH_DESC,        1, data128());
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_HEALTH_BAR_VISIBILITY,           LOC_TEXT_PLAYER_TRAIT_OBSCURED_VITALS, LOC_TEXT_PLAYER_TRAIT_OBSCURED_VITALS_DESC, 1, data128());
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_INGAME_ZOOM,                     LOC_TEXT_PLAYER_TRAIT_TUNNEL_VISION,   LOC_TEXT_PLAYER_TRAIT_TUNNEL_VISION_DESC,   1, data128(1.5f));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_CHEST_SCROLL_SPEED,              LOC_TEXT_PLAYER_TRAIT_JITTERY,         LOC_TEXT_PLAYER_TRAIT_JITTERY_DESC,         1, data128(1.5f));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_SKILL_SLOT_COUNT,                LOC_TEXT_PLAYER_TRAIT_FORGETFUL,       LOC_TEXT_PLAYER_TRAIT_FORGETFUL_DESC,       1, data128(4));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_ZOMBIE_FOLLOW_DISTANCE,          LOC_TEXT_PLAYER_TRAIT_BLOOD_SCENT,     LOC_TEXT_PLAYER_TRAIT_BLOOD_SCENT_DESC,     1, data128(2.f));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_TIMER_VISIBILITY,                LOC_TEXT_PLAYER_TRAIT_APATHY,          LOC_TEXT_PLAYER_TRAIT_APATHY_DESC,          1, data128());
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_EXP_BAR_VISIBILITY,              LOC_TEXT_PLAYER_TRAIT_IGNORANCE,       LOC_TEXT_PLAYER_TRAIT_IGNORANCE_DESC,       1, data128());
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_PROJECTILE_PASS_THROUGH_MODIFIER,LOC_TEXT_PLAYER_TRAIT_BLUNTED,         LOC_TEXT_PLAYER_TRAIT_BLUNTED_DESC,         1, data128(1));
      add_trait(CHARACTER_TRAIT_GAME_RULE, GAME_RULE_ROLL_DISTANCE_MULTIPLIER,        LOC_TEXT_PLAYER_TRAIT_ENCUMBERED,      LOC_TEXT_PLAYER_TRAIT_ENCUMBERED_DESC,      1, data128(0.75f));
    }
    // Negavite Traits

  }
  // Traits

  // Sigils
  {
    auto set_sigil = [](item_type _item_type, loc_text_id loc_id, data128 data, i32 tex_id) {
      state->default_items.at(_item_type) = item_data(_item_type, static_cast<i32>(loc_id), data, tex_id, false, false);
    };
    set_sigil(ITEM_TYPE_SIGIL_HEAD,                             LOC_TEXT_SIGIL_NAME_HEAD,         data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_MICHAEL,                     LOC_TEXT_SIGIL_NAME_ARCH_MICHAEL, data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_GABRIEL,                     LOC_TEXT_SIGIL_NAME_ARCH_GABRIEL, data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_RAPHAEL,                     LOC_TEXT_SIGIL_NAME_ARCH_RAPHAEL, data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_URIEL,                       LOC_TEXT_SIGIL_NAME_ARCH_URIEL,   data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_SAMAEL,                      LOC_TEXT_SIGIL_NAME_ARCH_SAMAEL,  data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_ZADKIEL,                     LOC_TEXT_SIGIL_NAME_ARCH_ZADKIEL, data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_THAVAEL,                     LOC_TEXT_SIGIL_NAME_ARCH_THAVAEL, data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_AZRAEL,                      LOC_TEXT_SIGIL_NAME_ARCH_AZRAEL,  data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_CAMAEL,                      LOC_TEXT_SIGIL_NAME_ARCH_CAMAEL,  data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_ARCH_JOPHIEL,                     LOC_TEXT_SIGIL_NAME_ARCH_JOPHIEL, data128(), TEX_ID_UNSPECIFIED);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_HEALTH,                    LOC_TEXT_SIGIL_INTENT_COMMON_HEALTH,                    data128(), TEX_ID_SIGIL_HEALTH);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_STAMINA,                   LOC_TEXT_SIGIL_INTENT_COMMON_STAMINA,                   data128(), TEX_ID_SIGIL_STAMINA);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_MANA,                      LOC_TEXT_SIGIL_INTENT_COMMON_MANA,                      data128(), TEX_ID_SIGIL_MANA);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_HP_REGEN,                  LOC_TEXT_SIGIL_INTENT_COMMON_HP_REGEN,                  data128(), TEX_ID_SIGIL_HP_REGEN);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_STAMINA_REGEN,             LOC_TEXT_SIGIL_INTENT_COMMON_STAMINA_REGEN,             data128(), TEX_ID_SIGIL_STAMINA_REGEN);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_MANA_REGEN,                LOC_TEXT_SIGIL_INTENT_COMMON_MANA_REGEN,                data128(), TEX_ID_SIGIL_MANA_REGEN);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_MOVE_SPEED,                LOC_TEXT_SIGIL_INTENT_COMMON_MOVE_SPEED,                data128(), TEX_ID_SIGIL_MOVE_SPEED);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_AOE,                       LOC_TEXT_SIGIL_INTENT_COMMON_AOE,                       data128(), TEX_ID_SIGIL_AOE);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_OVERALL_DAMAGE,            LOC_TEXT_SIGIL_INTENT_COMMON_OVERALL_DAMAGE,            data128(), TEX_ID_SIGIL_OVERALL_DAMAGE);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_ABILITY_CD,                LOC_TEXT_SIGIL_INTENT_COMMON_ABILITY_CD,                data128(), TEX_ID_SIGIL_ABILITY_CD);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_PROJECTILE_AMOUNT,         LOC_TEXT_SIGIL_INTENT_COMMON_PROJECTILE_AMOUTH,         data128(), TEX_ID_SIGIL_PROJECTILE_AMOUTH);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_EXP_GAIN,                  LOC_TEXT_SIGIL_INTENT_COMMON_EXP_GAIN,                  data128(), TEX_ID_SIGIL_EXP_GAIN);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_TOTAL_TRAIT_POINTS,        LOC_TEXT_SIGIL_INTENT_COMMON_TOTAL_TRAIT_POINTS,        data128(), TEX_ID_SIGIL_TOTAL_TRAIT_POINT);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_BASIC_ATTACK_DAMAGE,       LOC_TEXT_SIGIL_INTENT_COMMON_BASIC_ATTACK_DAMAGE,       data128(), TEX_ID_SIGIL_BASIC_ATTACK_DAMAGE);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_BASIC_ATTACK_SPEED,        LOC_TEXT_SIGIL_INTENT_COMMON_BASIC_ATTACK_SPEED,        data128(), TEX_ID_SIGIL_BASIC_ATTACK_SPEED);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_CRITICAL_CHANCE,           LOC_TEXT_SIGIL_INTENT_COMMON_CRITICAL_CHANCE,           data128(), TEX_ID_SIGIL_CRITICAL_CHANCE);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_CRITICAL_DAMAGE,           LOC_TEXT_SIGIL_INTENT_COMMON_CRITICAL_DAMAGE,           data128(), TEX_ID_SIGIL_CRITICAL_DAMAGE);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_OVERALL_LUCK,              LOC_TEXT_SIGIL_INTENT_COMMON_OVERALL_LUCK,              data128(), TEX_ID_SIGIL_OVERALL_LUCK);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_DAMAGE_REDUCTION,          LOC_TEXT_SIGIL_INTENT_COMMON_DAMAGE_REDUCTION,          data128(), TEX_ID_SIGIL_DAMAGE_REDUCTION);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_CONDITION_DURATION,        LOC_TEXT_SIGIL_INTENT_COMMON_CONDITION_DURATION,        data128(), TEX_ID_SIGIL_CONDITION_DURATION);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_DAMAGE_OVER_TIME,          LOC_TEXT_SIGIL_INTENT_COMMON_DAMAGE_OVER_TIME,          data128(), TEX_ID_SIGIL_DAMAGE_OVER_TIME);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_DAMAGE_DEFERRAL,           LOC_TEXT_SIGIL_INTENT_COMMON_DAMAGE_DEFERRAL,           data128(), TEX_ID_SIGIL_DAMAGE_DEFERRAL);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_SIGIL_EFFECTIVENESS,       LOC_TEXT_SIGIL_INTENT_COMMON_SIGIL_EFFECTIVENESS,       data128(), TEX_ID_SIGIL_SIGIL_EFFECTIVENESS);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_VITAL_SIGIL_EFFECTIVENESS, LOC_TEXT_SIGIL_INTENT_COMMON_VITAL_SIGIL_EFFECTIVENESS, data128(), TEX_ID_SIGIL_VITAL_SIGIL_EFFECTIVENESS);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_LETAL_SIGIL_EFFECTIVENESS, LOC_TEXT_SIGIL_INTENT_COMMON_LETHAL_SIGIL_EFFECTIVENESS,data128(), TEX_ID_SIGIL_LETHAL_SIGIL_EFFECTIVENESS);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_DROP_RATE,                 LOC_TEXT_SIGIL_INTENT_COMMON_DROP_RATE,                 data128(), TEX_ID_SIGIL_DROP_RATE);
    set_sigil(ITEM_TYPE_SIGIL_COMMON_REWARD_MODIFIER,           LOC_TEXT_SIGIL_INTENT_COMMON_REWARD_MODIFIER,           data128(), TEX_ID_SIGIL_REWARD_MODIFIER);
  }
  // Sigils

  // Rules
  {
    auto add_game_rule_upgradable = [](game_rule_id _rule_id, i32 _loc_display_text, i32 loc_desc_text, Rectangle _icon_src, i32 base_level, data_type _data_type,  data128 data) {
      state->default_game_rules.at(_rule_id) = game_rule(_rule_id, _loc_display_text, loc_desc_text,_icon_src, base_level, _data_type, data);
    };
    add_game_rule_upgradable(GAME_RULE_SPAWN_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_SPAWN_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_SPAWN_MULTIPLIER_DESCRIPTION,
      Rectangle {2144, 672, 32, 32}, 6, DATA_TYPE_F32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_PLAY_TIME_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_PLAY_TIME_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_PLAY_TIME_MULTIPLIER_DESCRIPTION, 
      Rectangle {1792, 640, 32, 32}, 6, DATA_TYPE_F32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_DELTA_TIME_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_DELTA_TIME_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_DELTA_TIME_MULTIPLIER_DESCRIPTION,
      Rectangle {1696, 672, 32, 32}, 6, DATA_TYPE_F32, data128(static_cast<f32>(.2f))
    );
    add_game_rule_upgradable(GAME_RULE_BOSS_MODIFIER, LOC_TEXT_INGAME_GAME_RULE_BOSS_MODIFIER, LOC_TEXT_INGAME_GAME_RULE_BOSS_MODIFIER_DESCRIPTION, 
      Rectangle {1728, 800, 32, 32}, 1, DATA_TYPE_I32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_AREA_UNLOCKER, LOC_TEXT_INGAME_GAME_RULE_AREA_UNLOCKER, LOC_TEXT_INGAME_GAME_RULE_AREA_UNLOCKER_DESCRIPTION, 
      Rectangle {2144, 672, 32, 32}, 1, DATA_TYPE_I32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_TRAIT_POINT_MODIFIER, LOC_TEXT_INGAME_GAME_RULE_TRAIT_POINT_MODIFIER, LOC_TEXT_INGAME_GAME_RULE_TRAIT_POINT_MODIFIER_DESCRIPTION, 
      Rectangle {1600, 640, 32, 32}, 1, DATA_TYPE_I32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_BONUS_RESULT_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_BONUS_RESULT_MULTIPLIER, LOC_TEXT_INGAME_GAME_RULE_BONUS_RESULT_MULTIPLIER_DESCRIPTION, 
      Rectangle {1792, 736, 32, 32}, 6, DATA_TYPE_F32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_ZOMBIE_LEVEL_MODIFIER, LOC_TEXT_INGAME_GAME_RULE_ZOMBIE_LEVEL_MODIFIER, LOC_TEXT_INGAME_GAME_RULE_ZOMBIE_LEVEL_MODIFIER_DESCRIPTION, 
      Rectangle {1568, 672, 32, 32}, 1, DATA_TYPE_I32, data128()
    );
    add_game_rule_upgradable(GAME_RULE_RESERVED_FOR_FUTURE_USE, LOC_TEXT_INGAME_GAME_RULE_RESERVED_FOR_FUTURE_USE, LOC_TEXT_INGAME_GAME_RULE_RESERVED_FOR_FUTURE_USE_DESCRIPTION, 
      Rectangle {1568, 672, 32, 32}, 1, DATA_TYPE_UNDEFINED, data128()
    );
    auto add_game_rule = [](game_rule_id _rule_id, data_type _data_type,  data128 data) {
      state->default_game_rules.at(_rule_id) = game_rule(_rule_id, 0, 0, ZERORECT, 0, _data_type, data);
    };
    add_game_rule(GAME_RULE_BASIC_ATTACKS_REQUIRE_STAMINA,    DATA_TYPE_I32, data128(0));     // INFO: i32, bool
    add_game_rule(GAME_RULE_JUMP_REQUIRE_STAMINA,             DATA_TYPE_I32, data128(0));     // INFO: i32, bool
    add_game_rule(GAME_RULE_LOOTING_DISTANCE,                 DATA_TYPE_F32, data128(50.f));  // INFO: f32
    add_game_rule(GAME_RULE_ZOMBIE_BITE_CONDITIONS,           DATA_TYPE_I32, data128());      // INFO: i32, enum
    add_game_rule(GAME_RULE_NEAR_ZOMBIE_CONDITIONS,           DATA_TYPE_I32, data128());      // INFO: i32, enum
    add_game_rule(GAME_RULE_ITEM_SLOT_COUNT,                  DATA_TYPE_I32, data128(6));     // INFO: i32
    add_game_rule(GAME_RULE_YOUR_SKILLS_DAMAGES_YOU,          DATA_TYPE_I32, data128(0));     // INFO: i32, bool
    add_game_rule(GAME_RULE_HEALTH_BAR_VISIBILITY,            DATA_TYPE_I32, data128(1));     // INFO: i32, bool
    add_game_rule(GAME_RULE_INGAME_ZOOM,                      DATA_TYPE_F32, data128(.85f));  // INFO: f32
    add_game_rule(GAME_RULE_CHEST_SCROLL_SPEED,               DATA_TYPE_F32, data128(1500.f));// INFO: f32
    add_game_rule(GAME_RULE_SKILL_SLOT_COUNT,                 DATA_TYPE_I32, data128(5));     // INFO: i32
    add_game_rule(GAME_RULE_ZOMBIE_FOLLOW_DISTANCE,           DATA_TYPE_F32, data128(728.f)); // INFO: f32
    add_game_rule(GAME_RULE_TIMER_VISIBILITY,                 DATA_TYPE_I32, data128(1));     // INFO: i32, bool
    add_game_rule(GAME_RULE_EXP_BAR_VISIBILITY,               DATA_TYPE_I32, data128(1));     // INFO: i32, bool
    add_game_rule(GAME_RULE_PROJECTILE_PASS_THROUGH_MODIFIER, DATA_TYPE_I32, data128(0));     // INFO: i32
    add_game_rule(GAME_RULE_ROLL_DISTANCE_MULTIPLIER,         DATA_TYPE_F32, data128(1.5f));  // INFO: f32

    for (size_t itr_000 = 1u; itr_000 < state->default_game_rules.size() - 1u; itr_000++) {
      game_rule& rule = state->default_game_rules.at(itr_000);
      gm_refresh_game_rule_by_level(&rule, rule.level);
    }
  }
  // Rules

  // Items
  {
    auto set_default_item = [](item_type _item_type, loc_text_id loc_id, data128 data, i32 tex_id, bool from_atlas) {
      state->default_items.at(_item_type) = item_data(_item_type, static_cast<i32>(loc_id), data, tex_id, 1, from_atlas);
    };
    set_default_item(ITEM_TYPE_EMPTY, LOC_TEXT_EMPTY, data128(), ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, true);
    set_default_item(ITEM_TYPE_SOUL,  LOC_TEXT_ITEM_DISPLAY_NAME_SOULS, data128(), ATLAS_TEX_ID_CURRENCY_SOUL_ICON, true);
  }
  // Items

  state->playlist = create_playlist(PLAYLIST_PRESET_INGAME_PLAY_LIST);
  
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

  return game_manager_reinit();
}
bool game_manager_reinit(void) {
  
  return true;
}

void game_manager_cleanup_state(void) {
  clean_up_spawn_state();
  collectible_manager_state_clear();
  set_player_state(player_state());

  if (state and state != nullptr) {
    state->ingame_phase = INGAME_PLAY_PHASE_UNDEFINED;
    state->playlist.media_stop(__builtin_addressof(state->playlist));
    state->is_win = false;
    state->stage = worldmap_stage();
    state->ingame_phase = INGAME_PLAY_PHASE_UNDEFINED;
    state->chosen_traits.clear();
    state->collected_coins = 0;
    state->total_spawn_count = 0;
    state->total_boss_count = 0;
    state->total_boss_spawned = 0;
    state->total_play_time = 0.f;
    state->play_time = 0.f;
    state->delta_time = 0.f;
    state->stage_boss_id = I32_MAX;
    state->starter_ability = ABILITY_ID_UNDEFINED;
    state->game_rules = state->default_game_rules;
  }
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
    case INGAME_PLAY_PHASE_IDLE: {
      update_spawns_animation_only();
      break; 
    }
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
          gm_end_game(true);
        }
      }
      else {
        gm_end_game(false);
        return;
      }
      break;
    }
    case INGAME_PLAY_PHASE_RESULTS: { 
      
      break;
    }
    default: {
      IWARN("game_manager::update_game_manager()::Unsupported in-game phase");
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
    case INGAME_PLAY_PHASE_IDLE: {
      render_spawns();
      return;
    }
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
      gm_end_game(false);
      event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
      return;
    }
  }

  IERROR("game_manager::render_game()::Function ended unexpectedly");
  gm_end_game(false);
  event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
}
void gm_draw_sigil(item_type type, Vector2 position, bool should_center) {
  if (type <= M_ITEM_TYPE_SIGIL_START or type >= M_ITEM_TYPE_SIGIL_END) {
    return;
  }
  const item_data& _sigil = state->default_items[type];
  const f32 sigil_rad = state->in_app_settings->render_height * GM_SIGIL_COMMON_RAD_SCALE_BY_VIEWPORT_SIZE;
  const Rectangle dest = {
    position.x,
    position.y,
    sigil_rad * 2.f,
    sigil_rad * 2.f
  };
  Vector2 origin = ZEROVEC2;
  if (should_center) {
    origin.x = dest.width * .5f;
    origin.y = dest.height * .5f;
  }
  const Vector2 backing_position = {
    should_center ? dest.x : dest.x + dest.width * .5f,
    should_center ? dest.y : dest.y + dest.height * .5f 
  };
  DrawCircle(backing_position.x, backing_position.y, sigil_rad, BLACK);

  if (_sigil.from_atlas) {
    gui_draw_atlas_texture_id(static_cast<atlas_texture_id>(_sigil.tex_id), dest, origin, 0.f, RED);
  }
  else {
    const Texture2D * const tex = get_texture_by_enum(static_cast<texture_id>(_sigil.tex_id));
    if (not tex or tex == nullptr) {
      return;
    }
    const Rectangle source = Rectangle {0.f, 0.f, static_cast<f32>(tex->width), static_cast<f32>(tex->height)};
    gui_draw_texture_id_pro(static_cast<texture_id>(_sigil.tex_id), source, dest, RED, origin);
  }
}

// OPS
/**
 * @brief Start game before, then upgrade the stat
 */
void gm_start_game() {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_start_game()::State is invalid");
    return;
  }
  _set_player_position(ZEROVEC2);
  state->playlist.media_play(__builtin_addressof(state->playlist));
  state->ingame_phase = INGAME_PLAY_PHASE_CLEAR_ZOMBIES;
}
[[nodiscard]] bool gm_init_game(worldmap_stage stage, std::vector<character_trait>& _chosen_traits, ability_id starter_ability) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_start_game()::State is invalid");
    return false;
  }
  reset_ingame_info();
  state->chosen_traits = _chosen_traits;
  state->starter_ability = starter_ability;

  if (not player_state_rebuild()) {
    IERROR("game_manager::gm_start_game()::Player rebuild is failed");
    game_manager_cleanup_state();
    return false;
  }
  set_player_state(state->player_state_static);
  player_state * const _player = get_player_state();
  if (not _player or _player == nullptr) {
    IERROR("game_manager::gm_start_game()::Player state is invalid");
    return false;
  }
  for (character_trait& trait : state->chosen_traits) {
    if (trait.affected_context == character_trait_type::CHARACTER_TRAIT_CHARACTER_STAT) {
      if (trait.context_id <= CHARACTER_STATS_UNDEFINED or trait.context_id >= CHARACTER_STATS_MAX) {
        IWARN("game_manager::gm_start_game()::Trait type is out of bound");
        continue;
      }
      gm_set_character_stat_trait_value(_player->stats[trait.context_id], trait.ingame_ops);
    }
    else if (trait.affected_context == character_trait_type::CHARACTER_TRAIT_GAME_RULE) {
      if (trait.context_id <= GAME_RULE_UNDEFINED or trait.context_id >= GAME_RULE_MAX) {
        IWARN("game_manager::gm_start_game()::Rule type is out of bound");
        continue;
      }
      gm_set_game_rule_trait_value(state->game_rules[trait.context_id], trait.ingame_ops);
    }
  }
  state->stage = stage;
  state->ingame_phase = INGAME_PLAY_PHASE_IDLE;
  state->total_spawn_count = state->stage.total_spawn_count * state->game_rules.at(GAME_RULE_SPAWN_MULTIPLIER).mm_ex.f32[3];
  state->total_play_time   = state->stage.stage_duration * state->game_rules.at(GAME_RULE_PLAY_TIME_MULTIPLIER).mm_ex.f32[3];
  state->total_boss_count  = state->stage.total_boss_count + state->game_rules.at(GAME_RULE_BOSS_MODIFIER).mm_ex.i32[3];
  state->play_time = (*state->game_info.total_play_time);

  populate_map_with_spawns(stage.total_spawn_count);
  if (state->game_info.in_spawns->size() <= 0) {
    return false;
  }
  _add_ability(state->starter_ability);
  ability *const player_starter_ability = __builtin_addressof(_player->ability_system.abilities.at(state->starter_ability));
  upgrade_ability(player_starter_ability);
  refresh_ability(player_starter_ability);

  event_fire(EVENT_CODE_SET_SPAWN_FOLLOW_DISTANCE, event_context(state->game_rules[GAME_RULE_ZOMBIE_FOLLOW_DISTANCE].mm_ex.f32[3]));
  set_ingame_delta_time_multiplier(state->game_rules.at(GAME_RULE_DELTA_TIME_MULTIPLIER).mm_ex.f32[3]);
  event_fire(EVENT_CODE_CAMERA_SET_ZOOM_TARGET, 
    event_context(state->game_rules[GAME_RULE_INGAME_ZOOM].mm_ex.f32[3])
  );

  return true;
}

void gm_end_game(bool is_win) {

  state->is_win = is_win;
  state->ingame_phase = INGAME_PLAY_PHASE_RESULTS;
  //event_fire(EVENT_CODE_PLAY_SOUND, event_context()); // TODO: play win or lose sound
  event_fire(EVENT_CODE_CAMERA_SET_ZOOM_TARGET, event_context(static_cast<f32>(ZOOM_CAMERA_GAME_RESULTS)));

  gm_save_game();
}
void gm_save_game(void) {
  state->game_progression_data->player_data = state->player_state_static;
  state->game_progression_data->game_rules = state->game_rules;
  save_save_data(SAVE_SLOT_CURRENT_SESSION);
}
void gm_load_game(void) {
  state->player_state_static = *(get_default_player());
  state->game_progression_data = get_save_data(SAVE_SLOT_CURRENT_SESSION);
  state->game_rules = state->default_game_rules;

  for (size_t itr_000 = FIRST_UPGRADABLE_GAME_RULE; itr_000 < LAST_UPGRADABLE_GAME_RULE; ++itr_000) {
    i32 level = state->game_progression_data->player_data.stats.at(itr_000).current_level;
    const character_stat& stat = state->player_state_static.stats.at(itr_000);
    if (level - stat.base_level >= 0 and level < MAX_PLAYER_LEVEL) {
      set_static_player_state_stat(static_cast<character_stat_id>(itr_000), state->game_progression_data->player_data.stats.at(itr_000).current_level);
    }
    else {
      set_static_player_state_stat(static_cast<character_stat_id>(itr_000), state->player_state_static.stats.at(itr_000).base_level);
    }
  }
  for (size_t itr_000 = FIRST_UPGRADABLE_GAME_RULE; itr_000 < LAST_UPGRADABLE_GAME_RULE; ++itr_000) { // First is GAME_RULE_UNDEFINED and last one GAME_RULE_MAX
    game_rule& base_rule = state->game_rules.at(itr_000);
    game_rule& saved_rule = state->game_progression_data->game_rules.at(itr_000);
    if (not gm_refresh_game_rule_by_level(__builtin_addressof(base_rule), saved_rule.level)) {
      gm_refresh_game_rule_by_level(__builtin_addressof(base_rule), state->default_game_rules.at(itr_000).base_level);
    }
  }
  state->player_state_static.inventory = state->game_progression_data->player_data.inventory;
  for (player_inventory_slot& slot : state->player_state_static.inventory) {
    const item_data& default_item = state->default_items.at(slot.item.type);
    item_data saved_item = slot.item;
    slot.item = default_item;
    slot.item.buffer.f32[1] = saved_item.buffer.f32[1];
    slot.item.level = saved_item.level;
    slot.slot_id = state->next_inventory_slot_id++;
    slot.item.id = state->next_item_id++;
  }
  state->player_inventory = state->game_progression_data->player_data.inventory;
  for (player_inventory_slot& slot : state->player_inventory) {
    const item_data& default_item = state->default_items.at(slot.item.type);
    item_data saved_item = slot.item;
    slot.item = default_item;
    slot.item.buffer.f32[1] = saved_item.buffer.f32[1];
    slot.item.level = saved_item.level;
    slot.slot_id = state->next_inventory_slot_id++;
    slot.item.id = state->next_item_id++;
  }

  for (sigil_slot& slot : state->game_progression_data->sigil_slots) {
    if (slot.sigil.type <= ITEM_TYPE_UNDEFINED or slot.sigil.type >= ITEM_TYPE_MAX) {
      continue;
    }
    gm_set_sigil_slot(sigil_slot(slot.id, slot.sigil.type, slot.sigil.buffer));
  }
  set_player_state(state->player_state_static);
}
void gm_refresh_save_slot(void) {
  gm_save_game();
  gm_load_game();
}
void gm_damage_spawn_if_collide(data128 coll_data, i32 damage, collision_type coll_check) {
  switch (coll_check) {
    case COLLISION_TYPE_RECTANGLE_RECTANGLE: {
      Rectangle rect = Rectangle { (f32)coll_data.i16[0],  (f32)coll_data.i16[1], (f32)coll_data.i16[2], (f32)coll_data.i16[3] };

      damage_spawn_by_collision(rect, damage, coll_check);
      return; 
    }
    case COLLISION_TYPE_CIRCLE_RECTANGLE: { 
      Vector2 circle_center = Vector2 { (f32)coll_data.i16[0],  (f32)coll_data.i16[1] };
      f32 circle_radius = (f32) coll_data.i16[2];

      damage_spawn_by_collision(Rectangle {circle_center.x, circle_center.y, circle_radius, 0.f}, damage, coll_check);
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
  clean_up_spawn_state();
  collectible_manager_state_clear();
  state->ingame_phase = INGAME_PLAY_PHASE_UNDEFINED;
  state->chosen_traits.clear();
  state->stage = worldmap_stage();
  state->game_rules.fill(game_rule());
  state->collected_coins = 0;
  state->total_spawn_count = 0;
  state->total_boss_count = 0;
  state->total_boss_spawned = 0;
  state->total_play_time = 0.f;
  state->play_time = 0.f;
  state->delta_time = 0.f;
  state->is_win = false;
  state->stage_boss_id = INVALID_IDI32;
  state->starter_ability = ABILITY_ID_UNDEFINED;

  state->game_rules = state->default_game_rules;
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
void gm_add_to_inventory(item_type _item_type, data128 ig_buffer, data128 ui_buffer, i32 amouth, i32 level) {
  if (_item_type >= ITEM_TYPE_MAX or _item_type <= ITEM_TYPE_UNDEFINED) {
    IWARN("game_manager::gm_add_to_inventory()::Item id is out of bounds");
    return;
  }
  if (_item_type == ITEM_TYPE_EMPTY) {
    return;
  }
  std::vector<player_inventory_slot>& inventory = state->player_inventory;
  auto create_slot = [&](void) -> player_inventory_slot {
    player_inventory_slot slot = player_inventory_slot();
    slot.slot_id = state->next_inventory_slot_id++;
    
    const item_data& _default = state->default_items[_item_type];
    slot.item = _default;
    slot.item.buffer = ig_buffer;
    slot.item.buffer.f32[0] = _default.buffer.f32[0];
    slot.item.id = state->next_item_id++;
    slot.item.level = level;
    slot.amouth = amouth;
    slot.ui_buffer = ui_buffer;
    gm_refresh_item(slot.item);
    return slot;
  };
  item_data& _default = state->default_items[_item_type];
  if (_default.stackable) {
    for (player_inventory_slot& slot : inventory) {
      if (slot.item.type == _item_type){
        slot.amouth += amouth;
        return;
      }
    }
    state->player_inventory.push_back(create_slot());
    return;
  }
  state->player_inventory.push_back(create_slot());
}
void gm_set_sigil_slot(sigil_slot slot) {
  if (slot.sigil.type <= M_ITEM_TYPE_SIGIL_START or slot.sigil.type >= M_ITEM_TYPE_SIGIL_END) {
    return;
  }
  if (slot.id <= SIGIL_SLOT_UNDEFINED or slot.id >= SIGIL_SLOT_MAX) {
    return;
  }
  const data128 _ig_buffer = slot.sigil.buffer;
  const data128 _ui_buffer = state->sigil_slots[slot.id].ui_buffer;
  const item_data& sigil = state->default_items[slot.sigil.type];

  slot.sigil = sigil;
  slot.sigil.buffer = _ig_buffer;

  state->sigil_slots[slot.id] = slot;
  state->sigil_slots[slot.id].ui_buffer = _ui_buffer;
  state->sigil_slots[slot.id].filled = true;
  state->sigil_slots[slot.id].draw_item = true;

  gm_refresh_sigil(&state->sigil_slots[slot.id].sigil);
}
void gm_clear_sigil_slot(sigil_slot_id id) {
  if (id <= SIGIL_SLOT_UNDEFINED or id >= SIGIL_SLOT_MAX) {
    return;
  }
  sigil_slot& slot = state->sigil_slots[id];
  slot.sigil = item_data();
  slot.filled = false;
  slot.draw_item = false;
}
void gm_set_sigil_ui_context(sigil_slot_id id, data128 ui_buffer, bool draw_item) {
  if (id <= SIGIL_SLOT_UNDEFINED or id >= SIGIL_SLOT_MAX) {
    return;
  }
  state->sigil_slots[id].ui_buffer = ui_buffer;
  state->sigil_slots[id].draw_item = draw_item;
}
void gm_remove_from_inventory_by_id(i32 id) {
  std::vector<player_inventory_slot>& inventory = state->player_inventory;

  std::erase_if(inventory, [id] (player_inventory_slot& slot) {
    return slot.slot_id == id;
  });

}
inventory_remove_amouth_result gm_remove_from_inventory_by_amouth(i32 id, i32 amouth) {
  std::vector<player_inventory_slot>& inventory = state->player_inventory;
  inventory_remove_amouth_result result {};

  std::erase_if(inventory, [&result, id, amouth] (player_inventory_slot& slot) {
    if (slot.slot_id != id) return false;
    slot.amouth -= amouth;
    result.found = true;
    result.remaining_amouth = slot.amouth;

    return slot.amouth <= 0;
  });

  return result;
}
void gm_update_inventory_item_by_id(item_data data, i32 id) {
  std::vector<player_inventory_slot>& inventory = state->player_inventory;

  for (player_inventory_slot& slot : inventory) {
    if (slot.slot_id == id) {
      slot.item.buffer = data.buffer;
    }
  }
}
void gm_set_chosen_traits(std::vector<character_trait> traits) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_set_chosen_traits()::State is not valid");
    return;
  }
  state->chosen_traits = traits;
}
bool player_state_rebuild(void) {
  if (not state or state == nullptr) {
    IERROR("player::player_state_rebuild()::State is invalid");
    return false;
  }
  player_state _player = state->player_state_static; 

  for (size_t itr_000 = 1u; itr_000 < _player.stats.size() - 1u; itr_000++) {
    character_stat& stat = _player.stats[itr_000];
    gm_refresh_stat_by_level(&stat, stat.current_level);
  }

  _player.exp_to_next_level = level_curve[_player.level];

  _player.health_current = _player.stats[CHARACTER_STATS_HEALTH].buffer.i32[3];
  _player.health_perc = 1.f;
  _player.stamina_current = _player.stats[CHARACTER_STATS_STAMINA].buffer.i32[3];
  _player.stamina_perc = 1.f;
  _player.mana_current = _player.stats[CHARACTER_STATS_MANA].buffer.i32[3];
  _player.mana_perc = 1.f;
  
  _player.attack_1_sprite.fps = _player.attack_1_sprite.frame_total / _player.stats[CHARACTER_STATS_BASIC_ATTACK_SPEED].buffer.f32[3];
  _player.attack_2_sprite.fps = _player.attack_2_sprite.frame_total / _player.stats[CHARACTER_STATS_BASIC_ATTACK_SPEED].buffer.f32[3];
  _player.attack_3_sprite.fps = _player.attack_3_sprite.frame_total / _player.stats[CHARACTER_STATS_BASIC_ATTACK_SPEED].buffer.f32[3];

  if (not std::isfinite(_player.attack_1_sprite.fps) or not std::isfinite(_player.attack_2_sprite.fps) or not std::isfinite(_player.attack_3_sprite.fps)) {
    return false;
  }
  state->player_state_static = _player;
  return true;
}
bool upgrade_ability_by_id(ability_id abl_id) {
  if (abl_id <= ABILITY_ID_UNDEFINED or abl_id >= ABILITY_ID_MAX) {
    IWARN("game_manager::upgrade_ability_by_id::Ability id is out of bound");
    return false;
  }
  upgrade_ability(&get_player_state()->ability_system.abilities[abl_id]);
  refresh_ability(&get_player_state()->ability_system.abilities[abl_id]);
  return true;
}
bool set_inventory_ui_ex(i32 slot_id, data128 data) {
  for (player_inventory_slot& slot : state->player_inventory) {
    if (slot_id == slot.slot_id) {
      slot.ui_buffer = data;
      return true;
    }
  }
  return false;
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
  gm_refresh_game_rule_by_level(__builtin_addressof(state->game_rules.at(rule_id)), level);
}
const ingame_info* gm_get_ingame_info(void){
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_get_ingame_info()::State is not valid");
    return nullptr;
  }
  return __builtin_addressof(state->game_info); 
}
const std::vector<character_trait>& gm_get_character_traits_all(void) {
  return state->traits;
}
const std::array<item_data, ITEM_TYPE_MAX>& gm_get_default_items(void) {
  return state->default_items;
}
const std::array<sigil_slot, SIGIL_SLOT_MAX>& gm_get_sigil_slots(void) {
  return state->sigil_slots;
}
f32 gm_get_ingame_chance(ingame_chance chance_type, [[__maybe_unused__]] data128 context) {
  if (chance_type <= INGAME_CHANCE_UNDEFINED or chance_type >= INGAME_CHANCE_MAX) {
    IWARN("game_manager::gm_get_ingame_chance()::chance type out of bound");
    return -1.f;
  }
  if( chance_type == INGAME_CHANCE_ITEM_DROP) {
    return -1.f;
  }
  else if( chance_type == INGAME_CHANCE_CRITICLE) {
    return -1.f;
  }
  else if( chance_type == INGAME_CHANCE_UPGRADE) {
  }
  IERROR("game_manager::gm_get_ingame_chance()::Function ended unexpectedly");
  return -1.f;
}


i32 gm_get_sigil_upgrade_soul_requirement(i32 level) {
  if (level < 0 or level > MAX_PLAYER_LEVEL) {
    return I32_MAX;
  }
  return level_curve[level] / 10.f;
}
/**
 * @brief Left hand should be sigil and right hand should be same of soul type
 */
sigil_upgrade_result gm_get_sigil_upgrade_requirements(item_data& lhs, item_data& rhs) {
  using result = sigil_upgrade_result;
  if (lhs.type == ITEM_TYPE_EMPTY or rhs.type == ITEM_TYPE_EMPTY) {
    return result::IDLE;
  }
  if (lhs.type < M_ITEM_TYPE_SIGIL_START or lhs.type > M_ITEM_TYPE_SIGIL_END) {
    return result::ERROR_LHS_TYPE_INCOMPATIBLE;
  }
  if (lhs.type == rhs.type) {
    return result::SUCCESS;
  }
  else if (rhs.type == ITEM_TYPE_SOUL) {
    if (lhs.level < 0 or lhs.level > MAX_PLAYER_LEVEL) {
      return result::ERROR_LHS_LEVEL_OUT_OF_BOUND;
    }
    const i32 soul_req = gm_get_sigil_upgrade_soul_requirement(lhs.level);

    if (rhs.buffer.i32[0] < soul_req) {
      return result(result::ERROR_INSUFFICIENT, soul_req);
    }

    return result(result::SUCCESS, soul_req);
  }
  else {
    return result::ERROR_RHS_TYPE_INCOMPATIBLE;
  }
}
sigil_upgrade_result gm_upgrade_sigil(item_data& lhs, item_data& rhs) {
  using result = sigil_upgrade_result;
  result _result = gm_get_sigil_upgrade_requirements(lhs, rhs);
  if (not _result.success) {
    return _result;
  }
  if (lhs.type == rhs.type) {
    lhs.buffer.f32[3] += rhs.buffer.f32[3];
    return result::SUCCESS;
  }
  else if (rhs.type == ITEM_TYPE_SOUL) {
    rhs.buffer.i32[0] -= _result.soul_requirement;

    lhs.level++;
    gm_refresh_sigil(&lhs);
    return sigil_upgrade_result(result::SUCCESS, _result.soul_requirement);
  }

  return _result;
}
void gm_refresh_stat_by_level(character_stat* stat, i32 level) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_refresh_stat_by_level()::State is not valid");
    return;
  }
  if (not stat or stat == nullptr) {
    IWARN("game_manager::gm_refresh_stat_by_level()::stat is not valid");
    return;
  }
  if (stat->id >= CHARACTER_STATS_MAX or stat->id <= CHARACTER_STATS_UNDEFINED) {
    IWARN("game_manager::gm_refresh_stat_by_level()::Stat id is out of bound");
    return;
  }
  if (level < 0 or (level + 1) - stat->base_level < 0) {
    IWARN("game_manager::gm_refresh_stat_by_level()::Stat level is out of bound");
    return;
  }
  i32 next_curve_value = level_curve[level];
  stat->upgrade_cost = level_curve[ (level+1) - stat->base_level ];
  stat->current_level = level;

  auto set_stat_value_int = [&](i32 value) {
    stat->buffer.i32[1] = value;
    stat->buffer.i32[3] = stat->buffer.i32[0] + stat->buffer.i32[1] + stat->buffer.i32[2];
  };
  auto set_stat_value_f32 = [&](f32 value) {
    stat->buffer.f32[1] = value;
    stat->buffer.f32[3] = stat->buffer.f32[0] + stat->buffer.f32[1] + stat->buffer.f32[2];
  };
  switch (stat->id) {
    case CHARACTER_STATS_HEALTH:                    { set_stat_value_int(next_curve_value         ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_STAMINA:                   { set_stat_value_int(next_curve_value         ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_MANA:                      { set_stat_value_int(next_curve_value         ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_HP_REGEN:                  { set_stat_value_int(next_curve_value / 10.f  ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_STAMINA_REGEN:             { set_stat_value_int(next_curve_value / 10.f  ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_MANA_REGEN:                { set_stat_value_int(next_curve_value / 10.f  ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_MOVE_SPEED:                { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_AOE:                       { set_stat_value_f32(next_curve_value / 1000.f); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_OVERALL_DAMAGE:            { set_stat_value_f32(next_curve_value / 1000.f); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_ABILITY_CD:                { set_stat_value_f32(next_curve_value / 1000.f); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_PROJECTILE_AMOUNT:         { set_stat_value_int(level                    ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_EXP_GAIN:                  { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_TOTAL_TRAIT_POINTS:        { set_stat_value_int(next_curve_value / 100.f ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_BASIC_ATTACK_DAMAGE:       { set_stat_value_int(next_curve_value / 100.f ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_BASIC_ATTACK_SPEED:        { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_CRITICAL_CHANCE:           { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_CRITICAL_DAMAGE:           { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_OVERALL_LUCK:              { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_DAMAGE_REDUCTION:          { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_CONDITION_DURATION:        { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_DAMAGE_OVER_TIME:          { set_stat_value_int(next_curve_value / 100.f ); return; } // DATA_TYPE_I32
    case CHARACTER_STATS_DAMAGE_DEFERRAL:           { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_SIGIL_EFFECTIVENESS:       { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_VITAL_SIGIL_EFFECTIVENESS: { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_LETAL_SIGIL_EFFECTIVENESS: { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_DROP_RATE:                 { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    case CHARACTER_STATS_REWARD_MODIFIER:           { set_stat_value_f32(next_curve_value / 100.f ); return; } // DATA_TYPE_F32
    default:{
      IWARN("game_manager::gm_refresh_stat_by_level()::Unsuppported stat id");
      return;
    }
  }
  IERROR("game_manager::gm_refresh_stat_by_level()::Function ended unexpectedly");
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
  
  auto set_game_rule_value_int = [&](i32 value) {
    rule->mm_ex.i32[1] = value;
    rule->mm_ex.i32[3] = rule->mm_ex.i32[0] + rule->mm_ex.i32[1] + rule->mm_ex.i32[2];
  };
  auto set_game_rule_value_f32 = [&](f32 value) {
    rule->mm_ex.f32[1] = value;
    rule->mm_ex.f32[3] = rule->mm_ex.f32[0] + rule->mm_ex.f32[1] + rule->mm_ex.f32[2];
  };
  switch (rule->id) {
    case GAME_RULE_SPAWN_MULTIPLIER:{        set_game_rule_value_f32(next_curve_value / 10000.f); return true; }
    case GAME_RULE_PLAY_TIME_MULTIPLIER:{    set_game_rule_value_f32(next_curve_value / 10000.f); return true; }
    case GAME_RULE_DELTA_TIME_MULTIPLIER:{   set_game_rule_value_f32(next_curve_value / 10000.f); return true; }
    case GAME_RULE_BOSS_MODIFIER:{           set_game_rule_value_int(level);                      return true; }
    case GAME_RULE_AREA_UNLOCKER:{           set_game_rule_value_int(level);                      return true; }
    case GAME_RULE_TRAIT_POINT_MODIFIER:{    set_game_rule_value_int(next_curve_value / 100.f);   return true; }
    case GAME_RULE_BONUS_RESULT_MULTIPLIER:{ set_game_rule_value_f32(next_curve_value / 10000.f); return true; }
    case GAME_RULE_ZOMBIE_LEVEL_MODIFIER:{   set_game_rule_value_int(level);                      return true; }
    case GAME_RULE_RESERVED_FOR_FUTURE_USE:{
      #warning "GAME_RULE_RESERVED_FOR_FUTURE_USE"

      return true;
    }
    case GAME_RULE_BASIC_ATTACKS_REQUIRE_STAMINA: {    return true; }                                //INFO: i32, bool Should not set by level
    case GAME_RULE_JUMP_REQUIRE_STAMINA: {             return true; }                                //INFO: i32, bool Should not set by level
    case GAME_RULE_LOOTING_DISTANCE: {                 set_game_rule_value_f32(level); return true; }//INFO: f32
    case GAME_RULE_ZOMBIE_BITE_CONDITIONS: {           return true; }                                //INFO: i32, enum Should not set by level
    case GAME_RULE_NEAR_ZOMBIE_CONDITIONS: {           return true; }                                //INFO: i32, enum Should not set by level
    case GAME_RULE_ITEM_SLOT_COUNT: {                  set_game_rule_value_int(level); return true; }//INFO: i32
    case GAME_RULE_YOUR_SKILLS_DAMAGES_YOU: {          return true; }                                //INFO: i32, bool Should not set by level
    case GAME_RULE_HEALTH_BAR_VISIBILITY: {            return true; }                                //INFO: i32, bool Should not set by level
    case GAME_RULE_INGAME_ZOOM: {                      set_game_rule_value_f32(level); return true; }//INFO: f32
    case GAME_RULE_CHEST_SCROLL_SPEED: {               set_game_rule_value_f32(level); return true; }//INFO: f32
    case GAME_RULE_SKILL_SLOT_COUNT: {                 set_game_rule_value_int(level); return true; }//INFO: i32
    case GAME_RULE_ZOMBIE_FOLLOW_DISTANCE: {           set_game_rule_value_f32(level); return true; }//INFO: f32
    case GAME_RULE_TIMER_VISIBILITY: {                 return true; }                                //INFO: i32, bool Should not set by level
    case GAME_RULE_EXP_BAR_VISIBILITY: {               return true; }                                //INFO: i32, bool Should not set by level
    case GAME_RULE_PROJECTILE_PASS_THROUGH_MODIFIER: { set_game_rule_value_int(level); return true; }//INFO: i32
    case GAME_RULE_ROLL_DISTANCE_MULTIPLIER: {         set_game_rule_value_f32(level); return true; }//INFO: f32
    default:{
      IWARN("game_manager::gm_refresh_game_rule_by_level()::Unsuppported stat id");
      return false;
    }
  }
  IERROR("game_manager::gm_refresh_game_rule_by_level()::Function ended unexpectedly");
  return false;
}
bool gm_refresh_sigil(item_data * sigil) {
  if (not sigil or sigil == nullptr) {
    return false;
  }
  if (sigil->type <= M_ITEM_TYPE_SIGIL_START or sigil->type >= M_ITEM_TYPE_SIGIL_END) {
    return false;
  }
  sigil->buffer.f32[3] = (sigil->buffer.f32[0] * (sigil->buffer.f32[1] / 1000.f)) + sigil->buffer.f32[0];
  return true;
}
void gm_refresh_item(item_data& item) {
  if (item.type <= ITEM_TYPE_UNDEFINED or item.type >= ITEM_TYPE_MAX) {
    item = item_data();
    return;
  }
  if (item.level > MAX_PLAYER_LEVEL or item.level < 0) {
    item.level = 1;
  }
  if (item.type >= M_ITEM_TYPE_SIGIL_START and item.type <= M_ITEM_TYPE_SIGIL_END) {
    gm_refresh_sigil(&item);
    return;
  }
  data128 _buffer = item.buffer;
  i32 _id = item.id;
  i32 _level = item.level;

  const item_data& _default = gm_get_default_items()[item.type];
  item = _default;

  item.buffer = _buffer;
  item.buffer.f32[0] = _default.buffer.f32[0];
  item.id = _id;
  item.level = _level;

  _buffer.f32[3] = _buffer.f32[0] + _buffer.f32[1] + _buffer.f32[2];
}

void gm_set_character_stat_trait_value(character_stat& stat, data128 value) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_set_character_stat_trait_value()::State is not valid");
    return;
  }
  auto set_stat_value_int = [&stat, &value](void) {
    stat.buffer.i32[2] = value.i32[0];
    stat.buffer.i32[3] = stat.buffer.i32[0] + stat.buffer.i32[1] + stat.buffer.i32[2];
  };
  auto set_stat_value_f32 = [&stat, &value](void) {
    stat.buffer.f32[2] = value.f32[0];
    stat.buffer.f32[3] = stat.buffer.f32[0] + stat.buffer.f32[1] + stat.buffer.f32[2];
  };
  switch (stat.data_type) {
    case DATA_TYPE_I32:{
      set_stat_value_int();
      return;
    }
    case DATA_TYPE_F32:{
      set_stat_value_f32();
      return;
    }
    default:{
      IWARN("game_manager::gm_set_character_stat_trait_value()::Unsuppported data type");
      return;
    }
  }
  IERROR("game_manager::gm_set_character_stat_trait_value()::Function ended unexpectedly");
}
void gm_set_game_rule_trait_value(game_rule& rule, data128 value) {
  if (not state or state == nullptr) {
    IERROR("game_manager::gm_set_game_rule_trait_value()::State is not valid");
    return;
  }
  auto set_rule_value_int = [&rule, &value](void) {
    rule.mm_ex.i32[2] = value.i32[0];
    rule.mm_ex.i32[3] = rule.mm_ex.i32[0] + rule.mm_ex.i32[1] + rule.mm_ex.i32[2];
  };
  auto set_rule_value_f32 = [&rule, &value](void) {
    rule.mm_ex.f32[2] = value.f32[0];
    rule.mm_ex.f32[3] = rule.mm_ex.f32[0] + rule.mm_ex.f32[1] + rule.mm_ex.f32[2];
  };
  switch (rule.data_type) {
    case DATA_TYPE_I32:{
      set_rule_value_int();
      return;
    }
    case DATA_TYPE_F32:{
      set_rule_value_f32();
      return;
    }
    default:{
      IWARN("game_manager::gm_set_game_rule_trait_value()::Unsuppported data type");
      return;
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
const player_state * gm_get_player_state(void) {
  return get_player_state();
}
f32 gm_get_player_sprite_scale(void) {
  return PLAYER_PLAYER_SCALE;
}
const std::vector<player_inventory_slot>& gm_get_inventory(void) {
  return state->player_inventory;
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
      gm_end_game(static_cast<bool>(context.data.i32[1]));
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
      item_type _item_type = static_cast<item_type>(context.data.i32[0]);
      gm_add_to_inventory(_item_type, state->default_items[_item_type].buffer);
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
#undef MAX_NEAREST_SPAWN_DISTANCE
#undef MAX_GAME_RULE_LEVEL
#undef FIRST_UPGRADABLE_GAME_RULE
#undef LAST_UPGRADABLE_GAME_RULE
