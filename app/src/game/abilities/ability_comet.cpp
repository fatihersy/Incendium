#include "ability_comet.h"
#include <reasings.h>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "game/spritesheet.h"

typedef struct ability_comet_state {
  std::array<ability, ABILITY_TYPE_MAX> abilities;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
} ability_comet_state;
static ability_comet_state * state;

bool ability_comet_initialize(const camera_metrics* _camera_metrics,const app_settings* _settings,const ingame_info* _ingame_info) {
  if (state) {
    TraceLog(LOG_WARNING, "ability::ability_system_initialize()::Init callet multiple times");
    return false;
  }
  state = (ability_comet_state*)allocate_memory_linear(sizeof(ability_comet_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_comet(ability* abl) {
  if (abl->type <= ABILITY_TYPE_UNDEFINED || abl->type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "ability::upgrade_ability()::Ability is not initialized");
    return;
  }
  ++abl->level;

  for (int i=0; i<ABILITY_UPG_MAX; ++i) {
    switch (abl->upgradables.at(i)) {
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
  if (!state) {
    return ability();
  }

  std::array<ability_upgradables, ABILITY_UPG_MAX> comet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability("Comet", ABILITY_TYPE_COMET,
    comet_upgr,
    0.f, 1.2f, Vector2 {1.2f, 1.2f}, 1, 7, 0.f, 15,
    Vector2{30.f, 30.f}, Rectangle{2144, 736, 32, 32}
  );
}
ability get_ability_comet_next_level(ability abl) {
  upgrade_ability_comet(&abl);
  return abl;
}

void update_ability_comet(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_comet()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_COMET) {
    TraceLog(LOG_WARNING, "ability::update_comet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_COMET, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_comet()::Ability is not active or not initialized");
    return;
  }
  player_state* p_player = reinterpret_cast<player_state*>(abl->p_owner);

  if (state->in_camera_metrics == nullptr || p_player == nullptr) {
    return;
  }
  const Rectangle* frustum = __builtin_addressof(state->in_camera_metrics->frustum);

  for (size_t iter = 0; iter < abl->projectiles.size(); iter++) {
    projectile& prj = abl->projectiles.at(iter);
    if (!prj.is_active) { continue; }
    if (vec2_equals(prj.position, pVECTOR2(prj.vec_ex.f32), .1) || prj.active_sprite == 1) {
      prj.active_sprite = 1;
      if (prj.animations.at(prj.active_sprite).current_frame >= prj.animations.at(prj.active_sprite).fps) {
        reset_sprite(&prj.animations.at(prj.active_sprite), true);      
        prj.active_sprite = 0;
        
        const f32 rand = get_random(frustum->x, frustum->x + frustum->width);
        prj.position = VECTOR2(rand, frustum->y - abl->proj_dim.y);
        Vector2 new_pos = {
          (f32)get_random(frustum->x + frustum->width  * .1f, frustum->x + frustum->width  - frustum->width  * .1f),
          (f32)get_random(frustum->y + frustum->height * .2f, frustum->y + frustum->height - frustum->height * .2f)
        };
        prj.vec_ex.f32[0] = new_pos.x;
        prj.vec_ex.f32[1] = new_pos.y;
        prj.animations.at(prj.active_sprite).rotation = get_movement_rotation(prj.position, pVECTOR2(prj.vec_ex.f32)) + 270.f;
      }
    }
    else {
      prj.active_sprite = 0;
      prj.position = move_towards(prj.position, pVECTOR2(prj.vec_ex.f32), abl->proj_speed);

      prj.collision.x = prj.position.x - prj.collision.width  * .5f;
      prj.collision.y = prj.position.y - prj.collision.height * .5f;
    }

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height), 
      static_cast<i16>(prj.damage + p_player->stats.at(CHARACTER_STATS_DAMAGE).buffer.i32[3]),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    
    update_sprite(&prj.animations.at(prj.active_sprite));
  }
}
void render_ability_comet(ability* abl){
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::render_comet()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_COMET) {
    TraceLog(LOG_WARNING, "ability::render_comet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_COMET, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_comet()::Ability is not active or not initialized");
    return;
  }

  for (size_t itr_000 = 0; itr_000 < abl->projectiles.size(); ++itr_000) {
    projectile& prj = abl->projectiles.at(itr_000);
    if (!prj.is_active) { continue; }
    Vector2 dim = Vector2 {
      prj.animations.at(prj.active_sprite).current_frame_rect.width  * abl->proj_sprite_scale,
      prj.animations.at(prj.active_sprite).current_frame_rect.height * abl->proj_sprite_scale
    };
    prj.animations.at(prj.active_sprite).origin.x = dim.x / 2.f;
    prj.animations.at(prj.active_sprite).origin.y = dim.y / 2.f;
    play_sprite_on_site(__builtin_addressof(prj.animations.at(prj.active_sprite)), WHITE, Rectangle { prj.position.x, prj.position.y, dim.x, dim.y });
  }
}
void refresh_ability_comet(ability* abl) { 
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_comet()::Ability is null");
    return;
  }
  if (abl->type != ABILITY_TYPE_COMET) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_comet()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_COMET, abl->type);
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_comet()::Ability projectile count exceed");
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
    for (size_t itr_111 = 0; itr_111 < abl->animation_ids.size(); ++itr_111) {
      if (abl->animation_ids.at(itr_111) <= SHEET_ID_SPRITESHEET_UNSPECIFIED || abl->animation_ids.at(itr_111) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        TraceLog(LOG_WARNING, "ability::refresh_ability_comet()::Ability sprite is not initialized or corrupted");
        return;
      }
      spritesheet spr = spritesheet();
      spr.sheet_id = abl->animation_ids.at(itr_111);
      set_sprite(__builtin_addressof(spr), true, false);
      spr.origin = VECTOR2( spr.coord.width * .5f,  spr.coord.height * .5f );

      prj.animations.push_back(spr); 
    }
    const Rectangle* frustum = __builtin_addressof(state->in_camera_metrics->frustum);
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
