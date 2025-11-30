#include "ability_pendulum.h"
#include <cmath>
#include "loc_types.h"

#include "core/fmemory.h"
#include "core/logger.h"

#include "game/spritesheet.h"

struct ability_pendulum_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;
  ability_pendulum_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
};

static ability_pendulum_state* state = nullptr;

#define PENDULUM_BLADE_DEST_X (state->in_camera_metrics->frustum.x + state->in_camera_metrics->frustum.width * .5f)
#define PENDULUM_BLADE_DEST_Y (state->in_camera_metrics->frustum.y * .18f)

#define PENDULUM_BLADE_WIDTH  (state->in_settings->render_height * .01f)
#define PENDULUM_BLADE_HEIGHT (state->in_camera_metrics->frustum.height * .85f)

static inline f32 shm_angle(f32 t, f32 theta0, f32 omega, f32 damping) {
  return theta0 * cosf(omega * t) * std::exp(-damping * t);
}

bool ability_pendulum_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info) {
  if (state && state != nullptr) {
    IWARN("ability::ability_pendulum_initialize()::Initialize called multiple times");
    return false;
  }
  state = (ability_pendulum_state*)allocate_memory_linear(sizeof(ability_pendulum_state), true);
  if (not state || state == nullptr) {
    IERROR("ability::ability_pendulum_initialize()::Failed to allocate state");
    return false;
  }
  state->in_settings = settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_pendulum(ability& abl) {
  if (abl.id <= ABILITY_ID_UNDEFINED || abl.id >= ABILITY_ID_MAX) {
    IWARN("ability::upgrade_ability_pendulum()::Ability is not initialized");
    return;
  }
  ++abl.level;
  for (ability_upgradables& upg : abl.upgradables) {
    switch (upg) {
      case ABILITY_UPG_DAMAGE: {
        abl.base_damage += abl.level * 2;
        break;
      }
      case ABILITY_UPG_HITBOX: { break; }
      case ABILITY_UPG_SPEED:  { break; }
      default: return;
    }
  }
}
ability get_ability_pendulum(void) {
  if (not state || state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> upgr = {
    ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED
  };
  return ability(static_cast<i32>(LOC_TEXT_EMPTY), ABILITY_ID_PENDULUM,
    upgr,
    0.60f, 1.f, Vector2{1.f, 1.f}, 1, 900.f, 1.25f, 20,
    Vector2{ PENDULUM_BLADE_WIDTH, PENDULUM_BLADE_HEIGHT }, Rectangle{2400, 928, 32, 32}
  );
}
ability get_ability_pendulum_next_level(ability abl) {
  upgrade_ability_pendulum(abl);
  return abl;
}

void update_ability_pendulum(ability& abl) {
  if (abl.id != ABILITY_ID_PENDULUM) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or not abl.p_owner or abl.projectiles.empty()) {
    return;
  }
  projectile& prj = abl.projectiles[0];
  const player_state* player = reinterpret_cast<player_state*>(abl.p_owner);

  abl.ability_cooldown_accumulator += (*state->in_ingame_info->delta_time);

  prj.accumulator += (*state->in_ingame_info->delta_time);
  const f32 theta0 = 60.f;
  const f32 omega = 4.5f;
  const f32 damping = 0.9f;
  const f32 angle = shm_angle(prj.accumulator, theta0, omega, damping);

  spritesheet& blade = prj.animations[0];
  blade.coord.x = PENDULUM_BLADE_DEST_X;
  blade.coord.y = PENDULUM_BLADE_DEST_Y;
  blade.rotation = angle;

  if (abl.ability_cooldown_accumulator > abl.ability_cooldown_duration) {
    abl.ability_cooldown_accumulator = 0.f;
    const i32 final_damage = abl.base_damage + static_cast<i32>(player->stats.at(CHARACTER_STATS_OVERALL_DAMAGE).buffer.f32[3]);
    const Rectangle blade_rect = blade.coord;
    
  }
}

void render_ability_pendulum(ability& abl) {
  if (abl.id != ABILITY_ID_PENDULUM) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or abl.projectiles.empty()) {
    return;
  }
  projectile& prj = abl.projectiles[0];
  spritesheet& blade = prj.animations[0];
  play_sprite_on_site_pro(blade, blade.coord, blade.origin, blade.rotation, blade.tint);
  spritesheet& fx = prj.animations[1];
  if (not fx.is_played) {
    update_sprite(fx, (*state->in_ingame_info->delta_time));
    play_sprite_on_site_pro(fx, fx.coord, fx.origin, fx.rotation, fx.tint);
  }
}

void refresh_ability_pendulum(ability& abl) {
  if (abl.id != ABILITY_ID_PENDULUM) {
    IWARN("ability::refresh_ability_pendulum()::Ability type is incorrect");
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    IWARN("ability::refresh_ability_pendulum()::Ability is not initialized or activated");
    return;
  }
  abl.projectiles.clear();
  abl.animation_ids.clear();
  abl.projectiles.reserve(abl.proj_count);

  projectile& prj = abl.projectiles.emplace_back(projectile());
  prj.damage = abl.base_damage;
  prj.is_active = true;
  prj.duration = abl.proj_duration;
  prj.accumulator = 0.f;

  spritesheet& blade = prj.animations.emplace_back(spritesheet());
  blade.sheet_id = SHEET_ID_HARVESTER_TRAIL;
  set_sprite(blade, true, true);
  blade.coord = Rectangle{ 0.f, 0.f, PENDULUM_BLADE_WIDTH, PENDULUM_BLADE_HEIGHT };
  blade.origin = Vector2{ blade.coord.width * .5f, blade.coord.height * .9f };
}


#undef PENDULUM_BLADE_WIDTH
#undef PENDULUM_BLADE_HEIGHT
#undef PENDULUM_EFFECT_WIDTH
#undef PENDULUM_EFFECT_HEIGHT
