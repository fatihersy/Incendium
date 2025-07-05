#include "ability_manager.h"

#include "core/fmemory.h"

#include "ability_bullet.h"
#include "ability_codex.h"
#include "ability_comet.h"
#include "ability_fireball.h"
#include "ability_firetrail.h"
#include "ability_radience.h"

typedef struct ability_system_state {
  std::array<ability, ABILITY_TYPE_MAX> abilities;

  camera_metrics* in_camera_metrics;
  app_settings* in_settings;
  ingame_info* in_ingame_info; 
} ability_system_state;
static ability_system_state * state;

void register_ability(ability abl) { state->abilities.at(abl.type) = abl; }

bool ability_system_initialize(camera_metrics* _camera_metrics, app_settings* _settings, ingame_info* _ingame_info) {
  if (state) {
    return false;
  }
  state = (ability_system_state*)allocate_memory_linear(sizeof(ability_system_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;

  if(ability_bullet_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_bullet());
  }
  else {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to initialize ability bullet");
  }

  if(ability_codex_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_codex());
  } 
  else {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to initialize ability codex");
  }

  if(ability_comet_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_comet());
  } 
  else {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to initialize ability comet");
  }

  if(ability_fireball_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_fireball());
  } 
  else {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to initialize ability fireball");
  }

  if(ability_firetrail_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_firetrail());
  } 
  else {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to initialize ability firetrail");
  }

  if(ability_radience_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_radience());
  } 
  else {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to initialize ability radience");
  }
  
  return true;
}

void upgrade_ability(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::upgrade_ability()::Ability is not valid");
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::upgrade_ability()::Ability is not initialized or activated");
    return;
  }
  if (abl->proj_count >= MAX_ABILITY_PROJECTILE_COUNT-1) {
    TraceLog(LOG_WARNING, "ability::upgrade_ability()::Ability projectile count exceed");
    return;
  }

  switch (abl->type) {
    case ABILITY_TYPE_FIREBALL:  upgrade_ability_fireball(abl); break;
    case ABILITY_TYPE_BULLET:    upgrade_ability_bullet(abl); break;
    case ABILITY_TYPE_COMET:     upgrade_ability_comet(abl); break;
    case ABILITY_TYPE_CODEX:     upgrade_ability_codex(abl); break;
    case ABILITY_TYPE_RADIANCE:  upgrade_ability_radience(abl); break;
    case ABILITY_TYPE_FIRETRAIL: upgrade_ability_firetrail(abl); break;
    default: {
      TraceLog(LOG_WARNING, "ability::upgrade_ability()::Unsuppported ability type");
      break;
    }
  }
}
void refresh_ability(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability()::Ability is not valid");
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::refresh_ability()::Ability is not initialized or activated");
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability()::Ability projectile count exceed");
    return;
  }

  switch (abl->type) {
    case ABILITY_TYPE_FIREBALL:  refresh_ability_fireball(abl); break;
    case ABILITY_TYPE_BULLET:    refresh_ability_bullet(abl); break;
    case ABILITY_TYPE_COMET:     refresh_ability_comet(abl); break;
    case ABILITY_TYPE_CODEX:     refresh_ability_codex(abl); break;
    case ABILITY_TYPE_RADIANCE:  refresh_ability_radience(abl); break;
    case ABILITY_TYPE_FIRETRAIL: refresh_ability_firetrail(abl); break;
    default: {
      TraceLog(LOG_WARNING, "ability::refresh_ability()::Unsuppported ability type");
      break;
    }
  }
}
void update_abilities(ability_play_system* system) {
  if (system == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_abilities()::Ability system is not valid");
    return;
  }
  for (size_t itr_000 = 1; itr_000 < ABILITY_TYPE_MAX; ++itr_000) {
    ability* abl = __builtin_addressof(system->abilities.at(itr_000));
    if (abl == nullptr) {
      TraceLog(LOG_WARNING, "ability::update_abilities()::Ability is not valid");
      return;
    }
    if (!abl->is_active || !abl->is_initialized) continue;

    switch (abl->type) {
      case ABILITY_TYPE_FIREBALL:  update_ability_fireball(abl); break;
      case ABILITY_TYPE_BULLET:    update_ability_bullet(abl); break;
      case ABILITY_TYPE_COMET:     update_ability_comet(abl); break;
      case ABILITY_TYPE_CODEX:     update_ability_codex(abl); break;
      case ABILITY_TYPE_RADIANCE:  update_ability_radience(abl); break;
      case ABILITY_TYPE_FIRETRAIL: update_ability_firetrail(abl); break;
      default: {
        TraceLog(LOG_WARNING, "ability::update_abilities()::Unsuppported ability type");
        break;
      }
    }
  }
}
void render_abilities(ability_play_system* system) {
  for (size_t iter = 1; iter < ABILITY_TYPE_MAX; ++iter) {
    ability* abl = __builtin_addressof(system->abilities.at(iter));
    if (abl == nullptr || system == nullptr || !abl->is_active || !abl->is_initialized) { continue; }

    switch (abl->type) {
      case ABILITY_TYPE_FIREBALL:  render_ability_fireball(abl); break;
      case ABILITY_TYPE_BULLET:    render_ability_bullet(abl);   break;
      case ABILITY_TYPE_COMET:     render_ability_comet(abl);    break;
      case ABILITY_TYPE_CODEX:     render_ability_codex(abl);    break;
      case ABILITY_TYPE_RADIANCE:  render_ability_radience(abl); break;
      case ABILITY_TYPE_FIRETRAIL: render_ability_firetrail(abl); break;
      default: {
        TraceLog(LOG_WARNING, "ability::update_abilities()::Unsuppported ability type");
        break;
      }
    }
  }
}

ability get_ability(ability_type _type) {
  if (!state) {
    return ability();
  }
  if (_type <= ABILITY_TYPE_UNDEFINED || _type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "ability_manager::get_ability()::Ability type is out of bound");
    return ability();
  }

  ability abl = state->abilities.at(_type);
  return abl;
}

ability get_next_level(ability abl) {
  if (!abl.is_active || !abl.is_initialized) {
    TraceLog(LOG_WARNING, "ability_manager::get_next_level()::Ability is not active or not initialized");
    return ability();
  }

  switch (abl.type) {
    case ABILITY_TYPE_FIREBALL:  get_ability_fireball_next_level(abl); break;
    case ABILITY_TYPE_BULLET:    get_ability_bullet_next_level(abl); break;
    case ABILITY_TYPE_COMET:     get_ability_comet_next_level(abl); break;
    case ABILITY_TYPE_CODEX:     get_ability_codex_next_level(abl); break;
    case ABILITY_TYPE_RADIANCE:  get_ability_radience_next_level(abl); break;
    case ABILITY_TYPE_FIRETRAIL: get_ability_firetrail_next_level(abl); break;
    default: {
      TraceLog(LOG_WARNING, "ability::get_next_level()::Unsuppported ability type");
      return ability();
    }
  }

  return abl;
}
