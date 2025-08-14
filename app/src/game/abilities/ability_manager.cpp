#include "ability_manager.h"

#include "core/fmemory.h"

#include "ability_bullet.h"
#include "ability_codex.h"
#include "ability_comet.h"
#include "ability_fireball.h"
#include "ability_firetrail.h"
#include "ability_radience.h"

typedef struct ability_system_state {
  std::array<ability, ABILITY_ID_MAX> abilities;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;
  ability_system_state(void) {
    this->abilities.fill(ability());
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
} ability_system_state;
static ability_system_state * state = nullptr;

void register_ability(ability abl) { state->abilities.at(abl.id) = abl; }

bool ability_system_initialize(const camera_metrics* _camera_metrics,const app_settings* _settings,const ingame_info* _ingame_info) {
  if (state and state != nullptr) {
    return false;
  }
  state = (ability_system_state*)allocate_memory_linear(sizeof(ability_system_state),true);
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  *state = ability_system_state();

  if (not _camera_metrics or _camera_metrics == nullptr) {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Camera metric pointer is invalid");
    return false;
  }
  if (not _settings or _settings == nullptr) {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Settings pointer is invalid");
    return false;
  }
  if (not _ingame_info or _ingame_info == nullptr) {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Game info pointer is invalid");
    return false;
  }

  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;

  if(ability_bullet_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_bullet());
  }
  else {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to initialize ability bullet");
  }

  if(ability_codex_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_codex());
  } 
  else {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to initialize ability codex");
  }

  if(ability_comet_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_comet());
  } 
  else {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to initialize ability comet");
  }

  if(ability_fireball_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_fireball());
  } 
  else {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to initialize ability fireball");
  }

  if(ability_firetrail_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_firetrail());
  } 
  else {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to initialize ability firetrail");
  }

  if(ability_radience_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_radience());
  } 
  else {
    TraceLog(LOG_ERROR, "ability_manager::ability_system_initialize()::Failed to initialize ability radience");
  }
  
  return true;
}

void upgrade_ability(ability* abl) {
  if (not abl or abl == nullptr) {
    TraceLog(LOG_WARNING, "ability_manager::upgrade_ability()::Ability is not valid");
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability_manager::upgrade_ability()::Ability is not initialized or activated");
    return;
  }
  if (abl->proj_count >= MAX_ABILITY_PROJECTILE_COUNT-1) {
    TraceLog(LOG_WARNING, "ability_manager::upgrade_ability()::Ability projectile count exceed");
    return;
  }

  switch (abl->id) {
    case ABILITY_ID_FIREBALL:  upgrade_ability_fireball(abl); break;
    case ABILITY_ID_BULLET:    upgrade_ability_bullet(abl); break;
    case ABILITY_ID_COMET:     upgrade_ability_comet(abl); break;
    case ABILITY_ID_CODEX:     upgrade_ability_codex(abl); break;
    case ABILITY_ID_RADIANCE:  upgrade_ability_radience(abl); break;
    case ABILITY_ID_FIRETRAIL: upgrade_ability_firetrail(abl); break;
    default: {
      TraceLog(LOG_WARNING, "ability_manager::upgrade_ability()::Unsuppported ability type");
      break;
    }
  }
}
void refresh_ability(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability_manager::refresh_ability()::Ability is not valid");
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability_manager::refresh_ability()::Ability is not initialized or activated");
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability_manager::refresh_ability()::Ability projectile count exceed");
    return;
  }

  switch (abl->id) {
    case ABILITY_ID_FIREBALL:  refresh_ability_fireball(abl); break;
    case ABILITY_ID_BULLET:    refresh_ability_bullet(abl); break;
    case ABILITY_ID_COMET:     refresh_ability_comet(abl); break;
    case ABILITY_ID_CODEX:     refresh_ability_codex(abl); break;
    case ABILITY_ID_RADIANCE:  refresh_ability_radience(abl); break;
    case ABILITY_ID_FIRETRAIL: refresh_ability_firetrail(abl); break;
    default: {
      TraceLog(LOG_WARNING, "ability_manager::refresh_ability()::Unsuppported ability type");
      break;
    }
  }
}
void update_abilities(ability_play_system* system) {
  if (system == nullptr) {
    TraceLog(LOG_WARNING, "ability_manager::update_abilities()::Ability system is not valid");
    return;
  }
  for (size_t itr_000 = 1; itr_000 < ABILITY_ID_MAX; ++itr_000) {
    ability* abl = __builtin_addressof(system->abilities.at(itr_000));
    if (abl == nullptr) {
      TraceLog(LOG_WARNING, "ability_manager::update_abilities()::Ability is not valid");
      return;
    }
    if (!abl->is_active || !abl->is_initialized) continue;

    switch (abl->id) {
      case ABILITY_ID_FIREBALL:  update_ability_fireball(abl); break;
      case ABILITY_ID_BULLET:    update_ability_bullet(abl); break;
      case ABILITY_ID_COMET:     update_ability_comet(abl); break;
      case ABILITY_ID_CODEX:     update_ability_codex(abl); break;
      case ABILITY_ID_RADIANCE:  update_ability_radience(abl); break;
      case ABILITY_ID_FIRETRAIL: update_ability_firetrail(abl); break;
      default: {
        TraceLog(LOG_WARNING, "ability_manager::update_abilities()::Unsuppported ability type");
        break;
      }
    }
  }
}
void render_abilities(ability_play_system* system) {
  for (size_t iter = 1; iter < ABILITY_ID_MAX; ++iter) {
    ability* abl = __builtin_addressof(system->abilities.at(iter));
    if (abl == nullptr || system == nullptr || !abl->is_active || !abl->is_initialized) { continue; }

    switch (abl->id) {
      case ABILITY_ID_FIREBALL:  render_ability_fireball(abl); break;
      case ABILITY_ID_BULLET:    render_ability_bullet(abl);   break;
      case ABILITY_ID_COMET:     render_ability_comet(abl);    break;
      case ABILITY_ID_CODEX:     render_ability_codex(abl);    break;
      case ABILITY_ID_RADIANCE:  render_ability_radience(abl); break;
      case ABILITY_ID_FIRETRAIL: render_ability_firetrail(abl); break;
      default: {
        TraceLog(LOG_WARNING, "ability_manager::render_abilities()::Unsuppported ability type");
        break;
      }
    }
  }
}

const ability* get_ability(ability_id _id) {
  if (not state or state == nullptr) {
    TraceLog(LOG_WARNING, "ability_manager::get_ability()::State is not valid");
    return nullptr;
  }
  if (_id <= ABILITY_ID_UNDEFINED or _id >= ABILITY_ID_MAX) {
    TraceLog(LOG_WARNING, "ability_manager::get_ability()::Ability type is out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->abilities.at(_id));
}
const std::array<ability, ABILITY_ID_MAX> * get_all_abilities(void) {
  if (not state or state == nullptr) {
    TraceLog(LOG_WARNING, "ability_manager::get_all_abilities()::State is not valid");
    return nullptr;
  }

  return __builtin_addressof(state->abilities);
}

ability get_next_level(ability abl) {
  if (!abl.is_active || !abl.is_initialized) {
    TraceLog(LOG_WARNING, "ability_manager::get_next_level()::Ability is not active or not initialized");
    return ability();
  }

  switch (abl.id) {
    case ABILITY_ID_FIREBALL:  get_ability_fireball_next_level(abl); break;
    case ABILITY_ID_BULLET:    get_ability_bullet_next_level(abl); break;
    case ABILITY_ID_COMET:     get_ability_comet_next_level(abl); break;
    case ABILITY_ID_CODEX:     get_ability_codex_next_level(abl); break;
    case ABILITY_ID_RADIANCE:  get_ability_radience_next_level(abl); break;
    case ABILITY_ID_FIRETRAIL: get_ability_firetrail_next_level(abl); break;
    default: {
      TraceLog(LOG_WARNING, "ability_manager::get_next_level()::Unsuppported ability type");
      return ability();
    }
  }

  return abl;
}
