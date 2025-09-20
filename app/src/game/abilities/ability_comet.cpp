#include "ability_comet.h"
#include <reasings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/ftime.h"
#include "core/logger.h"

#include "game/spritesheet.h"

// INFO: vec_ex.f32[0] = projectiles new pos x
//       vec_ex.f32[0] = projectiles new pos y
//
//       mm_ex.f32[0] = projectile explosion radius scale

typedef struct ability_comet_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;

  ability_comet_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
} ability_comet_state;
static ability_comet_state * state = nullptr;

bool ability_comet_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info) {
  if (state and state != nullptr) {
    return true;
  }
  state = (ability_comet_state*)allocate_memory_linear(sizeof(ability_comet_state),true);
  if (not state or state == nullptr) {
    IERROR("ability::ability_system_initialize()::Failed to allocate state");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_comet(ability *const abl) {
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
ability get_ability_comet(void) {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> comet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  ability comet = ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_COMET), ABILITY_ID_COMET,
    comet_upgr,
    0.f, 1.2f, Vector2 {1.2f, 1.2f}, 0, 12, 0.f, 64,
    Vector2{30.f, 30.f}, Rectangle{2144, 736, 32, 32}
  );
  comet.mm_ex.f32[0] = 1.5f;
  // TODO: Move other parameters here

  return comet;
}
ability get_ability_comet_next_level(ability abl) {
  upgrade_ability_comet(__builtin_addressof(abl));
  return abl;
}

void update_ability_comet(ability *const abl) {
  if (not abl or abl == nullptr) {
    IWARN("ability::update_ability_comet()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_COMET) {
    IWARN("ability::update_ability_comet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_COMET, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::update_ability_comet()::Ability is not active or not initialized");
    return;
  }
  if (not abl->p_owner) {
    IWARN("ability::update_ability_comet()::Player pointer is invalid");
    return;
  }
  const player_state *const p_player = reinterpret_cast<player_state*>(abl->p_owner);

  if (not p_player or p_player == nullptr or state->in_camera_metrics == nullptr) {
    return;
  }
  const Rectangle *const frustum = __builtin_addressof(state->in_camera_metrics->frustum);

  for (size_t itr_000 = 0u; itr_000 < abl->projectiles.size(); itr_000++) {
    projectile& prj = abl->projectiles.at(itr_000);
    if (not prj.is_active) { continue; }
    if (prj.active_sprite == 1 or vec2_equals(prj.position, pVECTOR2(prj.vec_ex.f32), .1)) {
      if (prj.active_sprite == 0) {
        prj.active_sprite = 1;
        reset_sprite(__builtin_addressof(prj.animations.at(prj.active_sprite)), true);
      }
      else if (prj.active_sprite == 1 and prj.animations.at(prj.active_sprite).is_played) {
        reset_sprite(__builtin_addressof(prj.animations.at(prj.active_sprite)), false);
        
        const f32 rand = get_random(frustum->x, frustum->x + frustum->width);
        prj.position = VECTOR2(rand, frustum->y - abl->proj_dim.y);
        const Vector2 new_pos = {
          (f32)get_random(frustum->x + frustum->width  * .1f, frustum->x + frustum->width  - frustum->width  * .1f),
          (f32)get_random(frustum->y + frustum->height * .2f, frustum->y + frustum->height - frustum->height * .2f)
        };
        prj.vec_ex.f32[0] = new_pos.x;
        prj.vec_ex.f32[1] = new_pos.y;
        
        prj.active_sprite = 0;
        prj.animations.at(prj.active_sprite).rotation = get_movement_rotation(prj.position, pVECTOR2(prj.vec_ex.f32)) + 270.f;
      }
      else if (prj.active_sprite == 1 and prj.animations.at(prj.active_sprite).current_frame == 0) {
        event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
          static_cast<i16>(prj.position.x), static_cast<i16>(prj.position.y), static_cast<i16>(prj.collision.width * prj.mm_ex.f32[0]), static_cast<i16>(0),
          static_cast<i16>(prj.damage + p_player->stats.at(CHARACTER_STATS_DAMAGE).buffer.i32[3]),
          static_cast<i16>(COLLISION_TYPE_CIRCLE_RECTANGLE)
        ));
      }
    }
    else {
      prj.active_sprite = 0;
      prj.position = move_towards(prj.position, pVECTOR2(prj.vec_ex.f32), abl->proj_speed);

      prj.collision.x = prj.position.x - prj.collision.width  * .5f;
      prj.collision.y = prj.position.y - prj.collision.height * .5f;
    }

    update_sprite(__builtin_addressof(prj.animations.at(prj.active_sprite)));
  }
}
void render_ability_comet(ability *const abl){
  if (not abl or abl == nullptr) {
    IWARN("ability::render_comet()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_COMET) {
    IWARN("ability::render_comet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_COMET, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::render_comet()::Ability is not active or not initialized");
    return;
  }

  for (size_t itr_000 = 0u; itr_000 < abl->projectiles.size(); ++itr_000) {
    projectile& prj = abl->projectiles.at(itr_000);
    if (not prj.is_active) { continue; }
    Vector2 dim = ZEROVEC2;
    if (prj.active_sprite == 1) {
      dim = Vector2 {
        prj.animations.at(prj.active_sprite).current_frame_rect.width  * abl->proj_sprite_scale * prj.mm_ex.f32[0],
        prj.animations.at(prj.active_sprite).current_frame_rect.height * abl->proj_sprite_scale * prj.mm_ex.f32[0]
      };
    }
    else {
      dim = Vector2 {
        prj.animations.at(prj.active_sprite).current_frame_rect.width  * abl->proj_sprite_scale,
        prj.animations.at(prj.active_sprite).current_frame_rect.height * abl->proj_sprite_scale
      };
    }

    prj.animations.at(prj.active_sprite).origin.x = dim.x / 2.f;
    prj.animations.at(prj.active_sprite).origin.y = dim.y / 2.f;
    play_sprite_on_site(__builtin_addressof(prj.animations.at(prj.active_sprite)), WHITE, Rectangle { prj.position.x, prj.position.y, dim.x, dim.y });
  }
}
void refresh_ability_comet(ability *const abl) { 
  if (not abl or abl == nullptr) {
    IWARN("ability::refresh_ability_comet()::Ability is null");
    return;
  }
  if (abl->id != ABILITY_ID_COMET) {
    IWARN("ability::refresh_ability_comet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_COMET, abl->id);
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    IWARN("ability::refresh_ability_comet()::Ability projectile count exceed");
    return;
  }

  abl->projectiles.clear();
  abl->animation_ids.clear();
  abl->projectiles.reserve(abl->proj_count);
  abl->animation_ids.push_back(SHEET_ID_FIREBALL_ANIMATION);
  abl->animation_ids.push_back(SHEET_ID_FIREBALL_EXPLOTION_ANIMATION);

  for (int itr_000 = 0; itr_000 < abl->proj_count; ++itr_000) {
    projectile prj = projectile();
    prj.collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    prj.mm_ex.f32[0] = abl->mm_ex.f32[0];

    spritesheet spr_fireball = spritesheet();
    spr_fireball.sheet_id = abl->animation_ids.at(0);
    set_sprite(__builtin_addressof(spr_fireball), true, false);
    spr_fireball.origin = VECTOR2( spr_fireball.coord.width * .5f,  spr_fireball.coord.height * .5f );
    prj.animations.push_back(spr_fireball);

    spritesheet spr_expl = spritesheet();
    spr_expl.sheet_id = abl->animation_ids.at(1);
    set_sprite(__builtin_addressof(spr_expl), false, true);
    spr_expl.origin = VECTOR2( spr_expl.coord.width * .5f,  spr_expl.coord.height * .5f );
    prj.animations.push_back(spr_expl); 
    
    const Rectangle *const frustum = __builtin_addressof(state->in_camera_metrics->frustum);
    const f32 rand = get_random(frustum->x, frustum->x + frustum->width);
    prj.position = VECTOR2(rand, frustum->y - abl->proj_dim.y);
    Vector2 new_pos = {
      (f32)get_random(frustum->x + frustum->width  * .1f, frustum->x + frustum->width  - frustum->width  * .1f),
      (f32)get_random(frustum->y + frustum->height * .2f, frustum->y + frustum->height - frustum->height * .2f)
    };
    prj.vec_ex.f32[0] = new_pos.x;
    prj.vec_ex.f32[1] = new_pos.y;
    prj.active_sprite = 0;
    prj.animations.at(prj.active_sprite).rotation = get_movement_rotation(prj.position, pVECTOR2(prj.vec_ex.f32)) + 270.f;

    abl->projectiles.push_back(prj);
  }
}
