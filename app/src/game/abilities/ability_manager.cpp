#include "ability_manager.h"

#include "core/fmemory.h"
#include "core/logger.h"

#include "ability_bullet.h"
#include "ability_harvester.h"
#include "ability_codex.h"
#include "ability_comet.h"
#include "ability_fireball.h"
#include "ability_firetrail.h"
#include "ability_radience.h"
#include "ability_pendulum.h"

typedef struct ability_system_state {
  std::array<ability, ABILITY_ID_MAX> abilities;
  ability empty_ability;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;
  ability_system_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
} ability_system_state;
static ability_system_state * state = nullptr;

void register_ability(ability abl) { state->abilities.at(abl.id) = abl; }

bool ability_system_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info) {
  if (state and state != nullptr) {
    return true;
  }
  state = (ability_system_state*)allocate_memory_linear(sizeof(ability_system_state),true);
  if (not state or state == nullptr) {
    IERROR("ability_manager::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  *state = ability_system_state();

  if (not _camera_metrics or _camera_metrics == nullptr) {
    IERROR("ability_manager::ability_system_initialize()::Camera metric pointer is invalid");
    return false;
  }
  if (not _settings or _settings == nullptr) {
    IERROR("ability_manager::ability_system_initialize()::Settings pointer is invalid");
    return false;
  }
  if (not _ingame_info or _ingame_info == nullptr) {
    IERROR("ability_manager::ability_system_initialize()::Game info pointer is invalid");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;

  if(ability_bullet_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_bullet());
  }
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability bullet");
  }
  if(ability_harvester_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_harvester());
  }
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability harvester");
  }

  if(ability_codex_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_codex());
  } 
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability codex");
  }

  if(ability_comet_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_comet());
  } 
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability comet");
  }

  if(ability_fireball_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_fireball());
  } 
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability fireball");
  }

  if(ability_firetrail_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_firetrail());
  } 
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability firetrail");
  }

  if(ability_radience_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_radience());
  } 
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability radience");
  }
  if(ability_pendulum_initialize(_camera_metrics, _settings, _ingame_info)) {
    register_ability(get_ability_pendulum());
  } 
  else {
    IERROR("ability_manager::ability_system_initialize()::Failed to initialize ability pendulum");
  }
  return true;
}

void upgrade_ability(ability& abl) {
  switch (abl.id) {
    case ABILITY_ID_FIREBALL:  upgrade_ability_fireball(abl); break;
    case ABILITY_ID_HARVESTER: upgrade_ability_harvester(abl); break;
    case ABILITY_ID_BULLET:    upgrade_ability_bullet(abl); break;
    case ABILITY_ID_COMET:     upgrade_ability_comet(abl); break;
    case ABILITY_ID_CODEX:     upgrade_ability_codex(abl); break;
    case ABILITY_ID_RADIANCE:  upgrade_ability_radience(abl); break;
    case ABILITY_ID_FIRETRAIL: upgrade_ability_firetrail(abl); break;
    case ABILITY_ID_PENDULUM:  upgrade_ability_pendulum(abl); break;

    default: {
      IWARN("ability_manager::upgrade_ability()::Unsuppported ability type");
      break;
    }
  }
}
void refresh_ability(ability& abl) {
  switch (abl.id) {
    case ABILITY_ID_FIREBALL:  refresh_ability_fireball(abl); break;
    case ABILITY_ID_BULLET:    refresh_ability_bullet(abl); break;
    case ABILITY_ID_HARVESTER: refresh_ability_harvester(abl); break;
    case ABILITY_ID_COMET:     refresh_ability_comet(abl); break;
    case ABILITY_ID_CODEX:     refresh_ability_codex(abl); break;
    case ABILITY_ID_RADIANCE:  refresh_ability_radience(abl); break;
    case ABILITY_ID_FIRETRAIL: refresh_ability_firetrail(abl); break;
    case ABILITY_ID_PENDULUM:  refresh_ability_pendulum(abl); break;
    default: {
      IWARN("ability_manager::refresh_ability()::Unsuppported ability type");
      break;
    }
  }
}
void update_abilities(ability_play_system& system) {
  for (ability& abl : system.abilities) {
    switch (abl.id) {
      case ABILITY_ID_FIREBALL:  update_ability_fireball(abl); break;
      case ABILITY_ID_BULLET:    update_ability_bullet(abl); break;
      case ABILITY_ID_HARVESTER: update_ability_harvester(abl); break;
      case ABILITY_ID_COMET:     update_ability_comet(abl); break;
      case ABILITY_ID_CODEX:     update_ability_codex(abl); break;
      case ABILITY_ID_RADIANCE:  update_ability_radience(abl); break;
      case ABILITY_ID_FIRETRAIL: update_ability_firetrail(abl); break;
      case ABILITY_ID_PENDULUM:  update_ability_pendulum(abl); break;

      default: { break; }
    }
  }
}
void render_abilities(ability_play_system& system) {
  for (ability& abl : system.abilities) {
    switch (abl.id) {
      case ABILITY_ID_FIREBALL:  render_ability_fireball(abl); break;
      case ABILITY_ID_BULLET:    render_ability_bullet(abl);   break;
      case ABILITY_ID_HARVESTER: render_ability_harvester(abl);   break;
      case ABILITY_ID_COMET:     render_ability_comet(abl);    break;
      case ABILITY_ID_CODEX:     render_ability_codex(abl);    break;
      case ABILITY_ID_RADIANCE:  render_ability_radience(abl); break;
      case ABILITY_ID_FIRETRAIL: render_ability_firetrail(abl); break;
      case ABILITY_ID_PENDULUM:  render_ability_pendulum(abl); break;

      default: { break; }
    }
  }
}

const ability& get_ability(ability_id _id) {
  if (_id <= ABILITY_ID_UNDEFINED or _id >= ABILITY_ID_MAX) {
    IWARN("ability_manager::get_ability()::Ability is not active or not initialized");
    return state->empty_ability;
  }
  return state->abilities[_id];
}
const std::array<ability, ABILITY_ID_MAX>& get_all_abilities(void) {
  return state->abilities;
}

void get_next_level(ability& abl) {
  switch (abl.id) {
    case ABILITY_ID_FIREBALL:  abl = get_ability_fireball_next_level(abl); break;
    case ABILITY_ID_HARVESTER: abl = get_ability_harvester_next_level(abl); break;
    case ABILITY_ID_BULLET:    abl = get_ability_bullet_next_level(abl); break;
    case ABILITY_ID_COMET:     abl = get_ability_comet_next_level(abl); break;
    case ABILITY_ID_CODEX:     abl = get_ability_codex_next_level(abl); break;
    case ABILITY_ID_RADIANCE:  abl = get_ability_radience_next_level(abl); break;
    case ABILITY_ID_FIRETRAIL: abl = get_ability_firetrail_next_level(abl); break;
    case ABILITY_ID_PENDULUM:  abl = get_ability_pendulum_next_level(abl); break;
    default: {
      IWARN("ability_manager::get_next_level()::Unsuppported ability type");
    }
  }
}
