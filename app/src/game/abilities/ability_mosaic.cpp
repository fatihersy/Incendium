#include "ability_mosaic.h"
#include <cmath>
#include "loc_types.h"

#include "core/fmemory.h"
#include "core/fmath.h"
#include "core/event.h"

#include "game/spritesheet.h"

struct ability_mosaic_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;
  ability_mosaic_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
};
static ability_mosaic_state* state = nullptr;

constexpr f32 mosaic_transition_speed = 6.f;

bool ability_mosaic_initialize(const camera_metrics *const _camera_metrics, const app_settings *const settings, const ingame_info *const _ingame_info) {
  if (state && state != nullptr) {
    return false;
  }
  state = (ability_mosaic_state*)allocate_memory_linear(sizeof(ability_mosaic_state), true);
  if (not state || state == nullptr) {
    return false;
  }
  state->in_settings = settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_mosaic(ability& abl) {
  if (abl.id <= ABILITY_ID_UNDEFINED or abl.id >= ABILITY_ID_MAX) {
    return;
  }
  if (abl.level >= MAX_ABILITY_LEVEL) {
    return;
  }
  ++abl.level;
  for (ability_upgradables& upg : abl.upgradables) {
    switch (upg) {
      case ABILITY_UPG_DAMAGE: {
        abl.base_damage += level_curve[abl.level] * 0.01f;
        break;
      }
      case ABILITY_UPG_HITBOX: { break; }
      case ABILITY_UPG_SPEED:  { break; }
      case ABILITY_UPG_AMOUNT: { abl.proj_count = (i32)fminf((f32)MAX_ABILITY_PROJECTILE_COUNT, (f32)(abl.proj_count + 2)); break; }
      default: return;
    }
  }
}

ability get_ability_mosaic(void) {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> upgr = {
    ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED
  };
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_SHATTERED_MOSAIC), ABILITY_ID_SHATTERED_MOSAIC,
    upgr,
    0.f, 1.f, Vector2{1.f, 1.f}, 5, 12.f, 0.f, 5,
    Vector2{ 14.f, 14.f }, Rectangle{2240, 1440, 32, 32}
  );
}
ability get_ability_mosaic_next_level(ability abl) {
  upgrade_ability_mosaic(abl);
  return abl;
}

static inline void mosaic_apply_visuals(spritesheet& spr, f32 health_perc) {
  const f32 injured = 1.f - fmaxf(0.f, fminf(1.f, health_perc));
  const f32 r = 80.f + 175.f * injured;
  const f32 g = 200.f - 120.f * injured;
  const f32 b = 255.f - 155.f * injured;
  spr.tint = Color{ (u8)r, (u8)g, (u8)b, 255u };
}

void update_ability_mosaic(ability& abl) {
  if (abl.id != ABILITY_ID_SHATTERED_MOSAIC) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or abl.projectiles.empty()) {
    return;
  }
  if (not state->in_ingame_info or not state->in_ingame_info->player_state_dynamic or not state->in_ingame_info->delta_time) return;

  const player_state& player = (*state->in_ingame_info->player_state_dynamic);
  const f32 dt = (*state->in_ingame_info->delta_time);
  const f32 health_perc = fmaxf(0.f, fminf(1.f, player.health_perc));

  const f32 view_w = state->in_camera_metrics->frustum.width;
  const f32 view_h = state->in_camera_metrics->frustum.height;
  const f32 tight_ax = fmaxf(player.collision.width * 0.6f, 24.f);
  const f32 tight_ay = fmaxf(player.collision.height * 0.6f, 24.f);
  const f32 wide_ax = view_w * 0.55f;
  const f32 wide_ay = view_h * 0.55f;
  const f32 target_ax = tight_ax + (wide_ax - tight_ax) * health_perc;
  const f32 target_ay = tight_ay + (wide_ay - tight_ay) * health_perc;

  abl.vec_ex.f32[0] = (abl.vec_ex.f32[0] == 0.f ? target_ax : abl.vec_ex.f32[0] + (target_ax - abl.vec_ex.f32[0]) * fminf(1.f, dt * mosaic_transition_speed));
  abl.vec_ex.f32[1] = (abl.vec_ex.f32[1] == 0.f ? target_ay : abl.vec_ex.f32[1] + (target_ay - abl.vec_ex.f32[1]) * fminf(1.f, dt * mosaic_transition_speed));
  const f32 speed_scale = (abl.proj_speed > 0 ? static_cast<f32>(abl.proj_speed) / 12.f : 1.f);
  abl.vec_ex.f32[3] += dt * speed_scale;

  abl.position.x = player.position.x;
  abl.position.y = player.position.y;

  const size_t N = abl.projectiles.size();
  for (size_t i = 0u; i < N; ++i) {
    projectile& prj = abl.projectiles[i];
    spritesheet& spr = prj.animations[0];

    const f32 t = abl.vec_ex.f32[3];
    const f32 ax = abl.vec_ex.f32[0];
    const f32 ay = abl.vec_ex.f32[1];
    const f32 phx = prj.mm_ex.f32[0];
    const f32 phy = prj.mm_ex.f32[1];
    const f32 freq_x = 2.0f + 0.07f * (f32)(i % 11);
    const f32 freq_y = 3.0f + 0.11f * (f32)(i % 7);
    const f32 drift_x = 0.25f * ax * sinf((1.3f + 0.05f * (f32)(i % 13)) * t + phy);
    const f32 drift_y = 0.25f * ay * sinf((0.9f + 0.03f * (f32)(i % 9)) * t + phx);

    const f32 nx = abl.position.x + ax * sinf(freq_x * t + phx) + drift_x;
    const f32 ny = abl.position.y + ay * cosf(freq_y * t + phy) + drift_y;

    const Vector2 prev = Vector2{ prj.vec_ex.f32[0], prj.vec_ex.f32[1] };
    const Vector2 curr = Vector2{ nx, ny };

    prj.position.x = nx;
    prj.position.y = ny;
    prj.vec_ex.f32[0] = nx;
    prj.vec_ex.f32[1] = ny;

    spr.coord.x = prj.position.x;
    spr.coord.y = prj.position.y;
    spr.rotation = get_movement_rotation(prev, curr);

    prj.collision.x = prj.position.x - (prj.collision.width * 0.5f);
    prj.collision.y = prj.position.y - (prj.collision.height * 0.5f);

    mosaic_apply_visuals(spr, health_perc);

    i16 base_damage = static_cast<i16>(prj.damage);
    i16 final_damage = base_damage + static_cast<i16>(base_damage * player.stats.at(CHARACTER_STATS_OVERALL_DAMAGE).buffer.f32[3]);
    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
      final_damage,
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
  }
}

void render_ability_mosaic(ability& abl) {
  if (abl.id != ABILITY_ID_SHATTERED_MOSAIC) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    return;
  }
  const atlas_texture * _tex = ss_get_atlas_texture_by_enum(ATLAS_TEX_ID_SCISSER_BLADE);
  if (not _tex) { return; }

  for (size_t i = 0u; i < abl.projectiles.size(); ++i) {
    projectile& prj = abl.projectiles[i];
    spritesheet& draw_ctx = prj.animations[0];
    Rectangle src = _tex->source;
    src.width = 12.f;
    src.height = 12.f;
    DrawTexturePro((*_tex->atlas_handle), src, draw_ctx.coord, draw_ctx.origin, draw_ctx.rotation, draw_ctx.tint);
  }
}

void refresh_ability_mosaic(ability& abl) {
  if (abl.id != ABILITY_ID_SHATTERED_MOSAIC) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    return;
  }
  abl.projectiles.clear();
  abl.animation_ids.clear();
  abl.projectiles.reserve(abl.proj_count);

  const Rectangle shard_size = {0.f, 0.f,
    abl.proj_dim.x * abl.proj_sprite_scale,
    abl.proj_dim.y * abl.proj_sprite_scale
  };

  for (i32 k = 0; k < abl.proj_count; ++k) {
    projectile& prj = abl.projectiles.emplace_back(projectile());
    prj.damage = abl.base_damage;
    prj.is_active = true;
    prj.active_sprite = 0;
    prj.mm_ex.f32[0] = 0.314f * (f32)(k + 1);
    prj.mm_ex.f32[1] = 0.618f * (f32)(k + 3);
    prj.vec_ex.f32[0] = abl.position.x;
    prj.vec_ex.f32[1] = abl.position.y;

    spritesheet& draw_ctx = prj.animations.emplace_back(spritesheet());
    draw_ctx.coord = shard_size;
    draw_ctx.origin = Vector2 { draw_ctx.coord.width * 0.5f, draw_ctx.coord.height * 0.5f };
    draw_ctx.tint = WHITE;

    prj.collision.width  = shard_size.width  * abl.proj_collision_scale.x;
    prj.collision.height = shard_size.height * abl.proj_collision_scale.y;
  }
}
