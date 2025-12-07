#include "ability_scissor.h"
#include <cmath>
#include "loc_types.h"

#include "core/fmemory.h"
#include "core/fmath.h"
#include "core/event.h"

#include "game/spritesheet.h"

struct ability_scissor_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;
  ability_scissor_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
};

static ability_scissor_state* state = nullptr;

#define SCISSOR_BLADE_WIDTH  state->in_settings->render_height * .015f
#define SCISSOR_BLADE_HEIGHT state->in_settings->render_height * .25f

constexpr easing_type SWING_EASE = EASING_TYPE_SINE_INOUT;
//constexpr f32 SCISSOR_SWING_MIN_DEG = -60.0f;
constexpr f32 SCISSOR_SWING_MAX_DEG =  60.0f;

bool ability_scissor_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info) {
  if (state && state != nullptr) {
    return false;
  }
  state = (ability_scissor_state*)allocate_memory_linear(sizeof(ability_scissor_state), true);
  if (not state || state == nullptr) {
    return false;
  }
  state->in_settings = settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_scissor(ability& abl) {
  if (abl.id <= ABILITY_ID_UNDEFINED or abl.id >= ABILITY_ID_MAX) {
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

ability get_ability_scissor(void) {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> upgr = {
    ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED
  };
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_SCISSOR), ABILITY_ID_SCISSOR,
    upgr,
    0.f, 1.f, Vector2{1.f, 1.f}, 2, 900.f, 0.f, 20,
    Vector2{ SCISSOR_BLADE_WIDTH, SCISSOR_BLADE_HEIGHT }, Rectangle{2464, 1152, 32, 32}
  );
}
ability get_ability_scissor_next_level(ability abl) {
  upgrade_ability_scissor(abl);
  return abl;
}

void update_ability_scissor(ability& abl) {
  if (abl.id != ABILITY_ID_SCISSOR) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or abl.projectiles.size() < 2) {
    return;
  }
  if (not state->in_ingame_info or not state->in_ingame_info->player_state_dynamic) return;
  
  const player_state& player = (*state->in_ingame_info->player_state_dynamic);
  
  const bool face_left = (player.w_direction == WORLD_DIRECTION_LEFT);
  const f32 back_offset_x = player.collision.width * 1.1f;
  const f32 front_offset_x = player.collision.width * 0.9f;
  abl.position.x = player.position.x + (face_left ? back_offset_x : -back_offset_x);
  abl.position.y = player.position.y + 20.0f;

  for (size_t i = 0u; i < 2; ++i) {
    projectile& prj = abl.projectiles[i];
    prj.position = abl.position;
    prj.collision.x = prj.position.x - (prj.collision.width * 0.5f);
    prj.collision.y = prj.position.y - (prj.collision.height * 0.5f);
    spritesheet& spr = prj.animations[0];
    spr.coord.x = prj.position.x;
    spr.coord.y = prj.position.y;
  }
  
  const bool has_valid_animation = player.current_anim_to_play.fps > 0.0001f;
  const f32 attack_duration = has_valid_animation ? (static_cast<f32>(player.current_anim_to_play.frame_total) / player.current_anim_to_play.fps) : 0.f;

  if (state->in_ingame_info->player_state_dynamic->anim_state == PL_ANIM_STATE_ATTACK and attack_duration > 0.f) {
    const f32 delta_time = *state->in_ingame_info->delta_time;
    abl.ability_cooldown_accumulator += delta_time;

    const f32 half_duration = attack_duration * 0.5f;
    const f32 dir_offset = face_left ? 180.0f : 0.0f;
    const f32 cycle_time = std::fmod(abl.ability_cooldown_accumulator, static_cast<f64>(attack_duration));

    const bool is_returning = (cycle_time > half_duration);
    const f32 local_time = is_returning ? (cycle_time - half_duration) : cycle_time;
    const f32 dir_sign = face_left ? -1.0f : 1.0f;
    const f32 back_signed  = -dir_sign * back_offset_x;
    const f32 front_signed =  dir_sign * front_offset_x;
    const f32 eased_offset = math_easing(
      local_time,
      (is_returning ? front_signed : back_signed),
      (is_returning ? (back_signed - front_signed) : (front_signed - back_signed)),
      half_duration,
      SWING_EASE
    );
    abl.position.x = player.position.x + eased_offset;
    for (size_t i = 0u; i < 2; ++i) {
      projectile& prj = abl.projectiles[i];
      prj.position = abl.position;
      prj.collision.x = prj.position.x - (prj.collision.width * 0.5f);
      prj.collision.y = prj.position.y - (prj.collision.height * 0.5f);
      spritesheet& spr = prj.animations[0];
      spr.coord.x = prj.position.x;
      spr.coord.y = prj.position.y;
    }
    f32 opening_deg = 0.f;
    if (!is_returning) {
      opening_deg = math_easing(local_time, SCISSOR_SWING_MAX_DEG, (0.f - SCISSOR_SWING_MAX_DEG), half_duration, SWING_EASE);
    } else {
      opening_deg = math_easing(local_time, 0.f, (SCISSOR_SWING_MAX_DEG - 0.f), half_duration, SWING_EASE);
    }

    abl.projectiles[0u].animations[0u].rotation =  opening_deg + 90.0f + dir_offset;
    abl.projectiles[1u].animations[0u].rotation = -opening_deg + 90.0f + dir_offset;

    for (size_t i = 0u; i < 2; ++i) {
      projectile& prj = abl.projectiles[i];
      spritesheet& spr = prj.animations[0u];
      event_fire(EVENT_CODE_DAMAGE_SPAWN_ROTATED_RECT, event_context(
        static_cast<i16>(prj.collision.x),
        static_cast<i16>(prj.collision.y),
        static_cast<i16>(prj.collision.width),
        static_cast<i16>(prj.collision.height),
        static_cast<i16>(abl.base_damage),
        static_cast<i16>(spr.rotation),
        static_cast<i16>(spr.origin.x),
        static_cast<i16>(spr.origin.y)
      ));
    }
  } else {
    const f32 dir_offset = face_left ? 180.0f : 0.0f;
    abl.ability_cooldown_accumulator = 0.0f;
    abl.projectiles[0u].animations[0u].rotation =  SCISSOR_SWING_MAX_DEG + 90.0f + dir_offset;
    abl.projectiles[1u].animations[0u].rotation = -SCISSOR_SWING_MAX_DEG + 90.0f + dir_offset;
  }
}

void render_ability_scissor(ability& abl) {
  if (abl.id != ABILITY_ID_SCISSOR) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    return;
  }
  const atlas_texture * _body_tex = ss_get_atlas_texture_by_enum(ATLAS_TEX_ID_SCISSER_BLADE);
  if (not _body_tex) { return; }

  bool face_left = false;
  if (state and state->in_ingame_info and state->in_ingame_info->player_state_dynamic) {
    face_left = (state->in_ingame_info->player_state_dynamic->w_direction == WORLD_DIRECTION_LEFT);
  }

  for (size_t i = 0u; i < abl.projectiles.size(); ++i) {
    projectile& prj = abl.projectiles[i];
    spritesheet& draw_ctx = prj.animations[0];
    Rectangle src = _body_tex->source;
    if (face_left) {
      src.width *= -1.f;
    }
    DrawTexturePro((*_body_tex->atlas_handle), src, draw_ctx.coord, draw_ctx.origin, draw_ctx.rotation, draw_ctx.tint);
  }
}

void refresh_ability_scissor(ability& abl) {
  if (abl.id != ABILITY_ID_SCISSOR) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    return;
  }
  abl.projectiles.clear();
  abl.animation_ids.clear();
  abl.projectiles.reserve(abl.proj_count);

  const Rectangle blade_size = {0.f, 0.f, 
    SCISSOR_BLADE_WIDTH * abl.proj_sprite_scale, 
    SCISSOR_BLADE_HEIGHT * abl.proj_sprite_scale
  };
  for (int k = 0; k < 2; ++k) {
    projectile& prj = abl.projectiles.emplace_back(projectile());
    prj.damage = abl.base_damage;
    prj.is_active = true;
    prj.active_sprite = 0;

    spritesheet& draw_ctx = prj.animations.emplace_back(spritesheet());
    draw_ctx.coord = blade_size;
    draw_ctx.origin = Vector2 { draw_ctx.coord.width * 0.5f, draw_ctx.coord.height * 0.635f };
    draw_ctx.tint = WHITE;

    prj.collision.width  = blade_size.width  * abl.proj_collision_scale.x;
    prj.collision.height = blade_size.height * abl.proj_collision_scale.y;
  }
}

#undef SCISSOR_BLADE_WIDTH
#undef SCISSOR_BLADE_HEIGHT
