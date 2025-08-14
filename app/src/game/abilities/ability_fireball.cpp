#include "ability_fireball.h"
#include <reasings.h>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "game/spritesheet.h"

typedef struct ability_fireball_state {
  std::array<ability, ABILITY_ID_MAX> abilities;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
} ability_fireball_state;
static ability_fireball_state * state;

bool ability_fireball_initialize(const camera_metrics* _camera_metrics,const app_settings* _settings,const ingame_info* _ingame_info) {
  if (state) {
    return false;
  }
  state = (ability_fireball_state*)allocate_memory_linear(sizeof(ability_fireball_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_fireball(ability* abl) {
  if (abl->id <= ABILITY_ID_UNDEFINED || abl->id >= ABILITY_ID_MAX) {
    TraceLog(LOG_WARNING, "ability::upgrade_ability()::Ability is not initialized");
    return;
  }
  ++abl->level;

  for (size_t itr_000 = 0; itr_000 < ABILITY_UPG_MAX; ++itr_000) {
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
ability get_ability_fireball(void) {
  if (!state) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> fireball_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_SPEED, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability("Fireball", ABILITY_ID_FIREBALL,
    fireball_upgr,
    0.f, 1.f, Vector2 {1.f, 1.f}, 1, 1, 0.f, 15,
    Vector2{30.f, 30.f}, Rectangle{2176, 736, 32, 32}
  );
}
ability get_ability_fireball_next_level(ability abl) {
  upgrade_ability_fireball(&abl);
  return abl;
}

void update_ability_fireball(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_fireball()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_FIREBALL) {
    TraceLog(LOG_WARNING, "ability::update_fireball()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_FIREBALL, abl->id);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_fireball()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr) {
    return;
  }
  player_state* player = reinterpret_cast<player_state*>(abl->p_owner);
  abl->position = player->position;
  abl->rotation += abl->proj_speed;
  const f32& aoe_scale = player->stats.at(CHARACTER_STATS_AOE).buffer.f32[3];

  if (abl->rotation > 360) abl->rotation = 0;

  for (size_t iter = 0; iter < abl->projectiles.size(); iter++) {
    projectile& prj = abl->projectiles.at(iter);
    if (!prj.is_active) { continue; }

    i16 angle = (i32)(((360.f / abl->projectiles.size()) * iter) + abl->rotation) % 360;

    prj.position = get_a_point_of_a_circle( abl->position, (abl->level * 3.f) + player->collision.height + 15, angle);
    prj.collision.x = prj.position.x - (prj.collision.width * .5f);
    prj.collision.y = prj.position.y - (prj.collision.width * .5f);
    
    const f32 prj_collision_width = prj.collision.width * aoe_scale;
    const f32 prj_collision_height = prj.collision.height * aoe_scale;
    Rectangle prj_collision = Rectangle {
      prj.collision.x - (prj_collision_width  * .5f),
      prj.collision.y - (prj_collision_height * .5f),
      prj_collision_width,
      prj_collision_height,
    };

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj_collision.x),     static_cast<i16>(prj_collision.y), static_cast<i16>(prj_collision.width), static_cast<i16>(prj_collision.height),
      static_cast<i16>(prj.damage + player->stats.at(CHARACTER_STATS_DAMAGE).buffer.i32[3]),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(&prj.animations.at(0));
  }
}

void render_ability_fireball(ability* abl){
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::render_fireball()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_FIREBALL) {
    TraceLog(LOG_WARNING, "ability::render_fireball()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_FIREBALL, abl->id);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_fireball()::Ability is not active or not initialized");
    return;
  }
  const f32& aoe_scale = state->in_ingame_info->player_state_dynamic->stats.at(CHARACTER_STATS_AOE).buffer.f32[3];

  for (size_t itr_000 = 0; itr_000 < abl->projectiles.size(); ++itr_000) {
    projectile& prj = abl->projectiles.at(itr_000);
    if (!prj.is_active) { continue; }
    Vector2 dim = Vector2 {
      prj.animations.at(prj.active_sprite).current_frame_rect.width  * abl->proj_sprite_scale,
      prj.animations.at(prj.active_sprite).current_frame_rect.height * abl->proj_sprite_scale
    };
    dim.x += (dim.x * aoe_scale * .5f);
    dim.y += (dim.y * aoe_scale * .5f);
    prj.animations.at(prj.active_sprite).origin.x = dim.x / 2.f;
    prj.animations.at(prj.active_sprite).origin.y = dim.y / 2.f;
    play_sprite_on_site(__builtin_addressof(prj.animations.at(prj.active_sprite)), WHITE, Rectangle { prj.position.x, prj.position.y, dim.x, dim.y });
  }
}

void refresh_ability_fireball(ability* abl) { 
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_fireball()::Ability is null");
    return;
  }
  if (abl->id != ABILITY_ID_FIREBALL) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_fireball()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_FIREBALL, abl->id);
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_fireball()::Ability projectile count exceed");
    return;
  }
  abl->projectiles.clear();
  abl->projectiles.reserve(abl->proj_count);
  abl->animation_ids.clear();
  abl->animation_ids.push_back(SHEET_ID_FLAME_ENERGY_ANIMATION);

  for (int itr_000 = 0; itr_000 < abl->proj_count; ++itr_000) {
    projectile prj = projectile();
    prj.collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    for (size_t itr_111 = 0; itr_111 < abl->animation_ids.size(); ++itr_111) {
      if (abl->animation_ids.at(itr_111) <= SHEET_ID_SPRITESHEET_UNSPECIFIED || abl->animation_ids.at(itr_111) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        TraceLog(LOG_WARNING, "ability::refresh_ability_fireball()::Ability sprite is not initialized or corrupted");
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
