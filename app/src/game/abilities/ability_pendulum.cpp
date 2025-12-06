#include "ability_pendulum.h"
#include <cmath>
#include "loc_types.h"

#include "core/fmemory.h"
#include "core/logger.h"
#include "core/fmath.h"
#include "core/event.h"

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

#define PENDULUM_BLADE_WIDTH  (state->in_settings->render_height * .01f)
#define PENDULUM_BLADE_HEIGHT (state->in_camera_metrics->frustum.height * .2f)

constexpr easing_type EASE  = EASING_TYPE_SINE_INOUT;
constexpr f32 ANGLE_START = 75.0f;
constexpr f32 ANGLE_END   = -75.0f;

#define RANGE_X  115.0f
#define DIP_Y    30.0f
#define X_OFFSET 0.0f
#define Y_OFFSET -60.0f

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
  //state->in_ingame_info->player_state_dynamic->collision.height
  return true;
}

void upgrade_ability_pendulum(ability& abl) {
  if (abl.id <= ABILITY_ID_UNDEFINED or abl.id >= ABILITY_ID_MAX) {
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
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> upgr = {
    ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED
  };
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_PENDULUM), ABILITY_ID_PENDULUM,
    upgr,
    1.4f, 1.f, Vector2{1.f, 1.f}, 2, 900.f, 0.f, 20,
    Vector2{ PENDULUM_BLADE_WIDTH, PENDULUM_BLADE_HEIGHT }, Rectangle{2400, 1472, 32, 32}
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
  if (not abl.is_active or not abl.is_initialized or not abl.p_owner or abl.projectiles.size() < 2) {
    return;
  }
  if (abl.p_owner == nullptr or state->in_ingame_info == nullptr) return;

  const f32 delta_time = *state->in_ingame_info->delta_time;
  abl.ability_cooldown_accumulator += delta_time;

  auto update_blade = [&abl, &delta_time](projectile& prj, bool swing_backward) {
    prj.accumulator += delta_time;
    f32 cycle_time = std::fmod(prj.accumulator, abl.ability_cooldown_duration);

    f32 t = math_easing(cycle_time, 0.0f, 1.0f, abl.ability_cooldown_duration, EASE);

    f32 current_offset_x = 0.0f;
    f32 current_rotation = 0.0f;

    if (swing_backward) {
      current_offset_x = (RANGE_X) + ((-RANGE_X - RANGE_X) * t); 
      current_rotation = ANGLE_END + ((ANGLE_START - ANGLE_END) * t);
    } else {
      current_offset_x = (-RANGE_X) + ((RANGE_X - (-RANGE_X)) * t); 
      current_rotation = ANGLE_START + ((ANGLE_END - ANGLE_START) * t);
    }
    f32 vertical_offset = std::sin(t * 3.14159f) * DIP_Y;
    vertical_offset += 10.0f; 

    prj.position.x = state->in_ingame_info->player_state_dynamic->position.x + current_offset_x + X_OFFSET;
    prj.position.y = state->in_ingame_info->player_state_dynamic->position.y + vertical_offset + Y_OFFSET;
    
    prj.collision.x = prj.position.x - (prj.collision.width * 0.5f);
    prj.collision.y = prj.position.y - (prj.collision.height * 0.5f);

    spritesheet& draw_ctx = prj.animations[0];

    draw_ctx.rotation = current_rotation; 
    draw_ctx.coord.x = prj.position.x;
    draw_ctx.coord.y = prj.position.y;

    event_fire(EVENT_CODE_DAMAGE_SPAWN_ROTATED_RECT, event_context(
      static_cast<i16>(prj.collision.x),      
      static_cast<i16>(prj.collision.y),      
      static_cast<i16>(prj.collision.width),  
      static_cast<i16>(prj.collision.height), 
      static_cast<i16>(abl.base_damage),      
      static_cast<i16>(current_rotation),     
      static_cast<i16>(draw_ctx.origin.x),
      static_cast<i16>(draw_ctx.origin.y)
    ));
  };

  update_blade(abl.projectiles[0], true);
  update_blade(abl.projectiles[1], false);
}

void render_ability_pendulum(ability& abl) {
  if (abl.id != ABILITY_ID_PENDULUM) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or abl.projectiles.empty()) {
    return;
  }
  const atlas_texture * _body_tex = ss_get_atlas_texture_by_enum(ATLAS_TEX_ID_CRIMSON_FANTASY_ORNATE_FRAME);

  {
    projectile& prj = abl.projectiles[0];
    spritesheet& draw_ctx = prj.animations[0];

    DrawTexturePro( (*_body_tex->atlas_handle), _body_tex->source, draw_ctx.coord, draw_ctx.origin, draw_ctx.rotation, draw_ctx.tint);
  }

  {
    projectile& prj = abl.projectiles[1];
    spritesheet& draw_ctx = prj.animations[0];
    
    DrawTexturePro( (*_body_tex->atlas_handle), _body_tex->source, draw_ctx.coord, draw_ctx.origin, draw_ctx.rotation, draw_ctx.tint);
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

  const Rectangle blade_size = {0.f, 0.f, PENDULUM_BLADE_WIDTH, PENDULUM_BLADE_HEIGHT};
  {
    projectile& prj = abl.projectiles.emplace_back(projectile());
    prj.damage = abl.base_damage;
    prj.is_active = true;
    prj.active_sprite = 0;

    spritesheet& draw_ctx = prj.animations.emplace_back(spritesheet());
    draw_ctx.coord = blade_size;
    draw_ctx.origin = Vector2 { 
      draw_ctx.coord.width * 0.5f, 
      draw_ctx.coord.height * 0.5f 
    };
    draw_ctx.tint = WHITE;

    prj.collision.width  = blade_size.width  * abl.proj_collision_scale.x;
    prj.collision.height = blade_size.height * abl.proj_collision_scale.y;
  }

  {
    projectile& prj = abl.projectiles.emplace_back(projectile());
    prj.damage = abl.base_damage;
    prj.is_active = true;
    prj.active_sprite = 0;

    spritesheet& draw_ctx = prj.animations.emplace_back(spritesheet());
    draw_ctx.coord = blade_size;
    draw_ctx.origin = Vector2 { 
      draw_ctx.coord.width * 0.5f, 
      draw_ctx.coord.height * 0.5f 
    };
    draw_ctx.tint = WHITE;

    prj.collision.width  = blade_size.width  * abl.proj_collision_scale.x;
    prj.collision.height = blade_size.height * abl.proj_collision_scale.y;
  }
}

#undef PENDULUM_BLADE_WIDTH
#undef PENDULUM_BLADE_HEIGHT
