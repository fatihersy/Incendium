#include "ability_bullet.h"
#include <reasings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"

#include "game/spritesheet.h"

typedef struct ability_bullet_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
  ability_bullet_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr; 
  }
} ability_bullet_state;

static ability_bullet_state * state = nullptr;

bool ability_bullet_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info) {
  if (state and state != nullptr) {
    IWARN("ability::ability_bullet_initialize()::Initialize called multiple times");
    return false;
  }
  state = (ability_bullet_state*)allocate_memory_linear(sizeof(ability_bullet_state), true);
  if (not state or state == nullptr) {
    IERROR("ability::ability_bullet_initialize()::Failed to allocate state");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_bullet(ability *const abl) {
  if (abl->id <= ABILITY_ID_UNDEFINED or abl->id >= ABILITY_ID_MAX) {
    IWARN("ability::upgrade_ability()::Ability is not initialized");
    return;
  }
  ++abl->level;

  for (size_t itr_000 = 0u; itr_000 < ABILITY_UPG_MAX; ++itr_000) {
    switch (abl->upgradables.at(itr_000)) {
      case ABILITY_UPG_DAMAGE: {
        abl->base_damage += abl->level * 2;
        break;
      }
      case ABILITY_UPG_AMOUNT: { 
        abl->proj_count++;
        break;
      }
      case ABILITY_UPG_HITBOX: { break; } // TODO: projectile hitbox upgrade
      case ABILITY_UPG_SPEED:  { break; } // TODO: projectile speed upgrade
      default: return;
    }
  }
}
ability get_ability_bullet(void) {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> bullet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_BULLET), ABILITY_ID_BULLET,
    bullet_upgr,
    0.f, 1.f, Vector2 {1.f, 1.f}, 1, 3, 1.75f, 15,
    Vector2{30.f, 30.f}, Rectangle{2368, 736, 32, 32}
  );
}
ability get_ability_bullet_next_level(ability abl) {
  upgrade_ability_bullet(__builtin_addressof(abl));
  return abl;
}

void update_ability_bullet(ability *const abl) {
  if (not abl or abl == nullptr) {
    IWARN("ability::update_bullet()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_BULLET) {
    IWARN("ability::update_bullet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_BULLET, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::update_bullet()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr) {
    return;
  }
  const player_state *const player = reinterpret_cast<player_state*>(abl->p_owner);
  abl->position = player->position;
  const f32& aoe_scale = player->stats.at(CHARACTER_STATS_AOE).buffer.f32[3];

  for (size_t iter = 0u; iter < abl->projectiles.size(); iter++) {
    projectile& prj = abl->projectiles.at(iter);
    if (not prj.is_active) { continue; }

    if (abl->mm_ex.f32[0] <= 0) {
      prj.position = player->position;
      prj.duration = abl->proj_duration;
      prj.direction = player->w_direction;
    }
    else {
     prj.duration -= GetFrameTime();
    }
    prj.position.x += prj.direction == WORLD_DIRECTION_RIGHT ? abl->proj_speed : -abl->proj_speed;
    prj.collision.x = prj.position.x - prj.collision.width  * .5f;
    prj.collision.y = prj.position.y - prj.collision.height * .5f;

    const f32 prj_collision_width = prj.collision.width * aoe_scale;
    const f32 prj_collision_height = prj.collision.height * aoe_scale;
    Rectangle prj_collision = Rectangle {
      prj.collision.x - (prj_collision_width  * .5f),
      prj.collision.y - (prj_collision_height * .5f),
      prj_collision_width,
      prj_collision_height,
    };

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj_collision.x), static_cast<i16>(prj_collision.y), static_cast<i16>(prj_collision.width), static_cast<i16>(prj_collision.height),
      static_cast<i16>(prj.damage + player->stats.at(CHARACTER_STATS_DAMAGE).buffer.i32[3]),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(__builtin_addressof(prj.animations.at(0)));
  }
}
void render_ability_bullet(ability *const abl){
  if (not abl or abl == nullptr) {
    IWARN("ability::render_bullet()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_BULLET) {
    IWARN("ability::render_bullet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_BULLET, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::render_bullet()::Ability is not active or not initialized");
    return;
  }
  const f32& aoe_scale = state->in_ingame_info->player_state_dynamic->stats.at(CHARACTER_STATS_AOE).buffer.f32[3];

  for (size_t itr_000 = 0u; itr_000 < abl->projectiles.size(); ++itr_000) {
    projectile& prj = abl->projectiles.at(itr_000);
    if (not prj.is_active) { continue; }

    Vector2 dim = Vector2 {
      prj.animations.at(prj.active_sprite).current_frame_rect.width  * abl->proj_sprite_scale,
      prj.animations.at(prj.active_sprite).current_frame_rect.height * abl->proj_sprite_scale
    };
    dim.x += (dim.x * aoe_scale);
    dim.y += (dim.y * aoe_scale);
    prj.animations.at(prj.active_sprite).origin.x = dim.x / 2.f;
    prj.animations.at(prj.active_sprite).origin.y = dim.y / 2.f;
    play_sprite_on_site(__builtin_addressof(prj.animations.at(prj.active_sprite)), WHITE, Rectangle { prj.position.x, prj.position.y, dim.x, dim.y });
  }
}
void refresh_ability_bullet(ability *const abl) { 
  if (not abl or abl == nullptr) {
    IWARN("ability::refresh_ability_bullet()::Ability is null");
    return;
  }
  if (abl->id != ABILITY_ID_BULLET) {
    IWARN("ability::refresh_ability_bullet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_BULLET, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::refresh_ability_bullet()::Ability is not initialized or activated");
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    IWARN("ability::refresh_ability_bullet()::Ability projectile count exceed");
    return;
  }

  abl->projectiles.clear();
  abl->animation_ids.clear();
  abl->projectiles.reserve(abl->proj_count);
  abl->animation_ids.push_back(SHEET_ID_FLAME_ENERGY_ANIMATION);

  for (int itr_000 = 0; itr_000 < abl->proj_count; ++itr_000) {
    projectile prj = projectile();
    prj.collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    for (size_t itr_111 = 0u; itr_111 < abl->animation_ids.size(); ++itr_111) {
      if (abl->animation_ids.at(itr_111) <= SHEET_ID_SPRITESHEET_UNSPECIFIED or abl->animation_ids.at(itr_111) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        IWARN("ability::refresh_ability_bullet()::Ability sprite is not initialized or corrupted");
        return;
      }
      spritesheet spr = spritesheet();
      spr.sheet_id = abl->animation_ids.at(itr_111);
      set_sprite(__builtin_addressof(spr), true, false);
      spr.origin = VECTOR2( spr.coord.width * .5f,  spr.coord.height * .5f );

      prj.animations.push_back(spr); 
      prj.active_sprite = 0;
    }
    abl->projectiles.push_back(prj);
  }
}

