#include "ability_firetrail.h"
#include <reasings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/spritesheet.h"

typedef struct ability_firetrail_state {
  std::array<ability, ABILITY_TYPE_MAX> abilities;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
} ability_firetrail_state;
static ability_firetrail_state * state;

bool ability_firetrail_initialize(const camera_metrics* _camera_metrics,const app_settings* _settings,const ingame_info* _ingame_info) {
  if (state) {
    TraceLog(LOG_WARNING, "ability::ability_system_initialize()::Init called multiple times");
    return false;
  }
  state = (ability_firetrail_state*)allocate_memory_linear(sizeof(ability_firetrail_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_firetrail(ability* abl) {
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

ability get_ability_firetrail(void) {
  if (!state) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> fire_trail_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability("Fire Trail", ABILITY_TYPE_FIRETRAIL,
    fire_trail_upgr,
    0.f, 3.5f, Vector2 {.25f, .25f}, 0u, 0u, 3.f, 11u,
    Vector2{128.f, 128.f}, Rectangle{2304.f, 736.f, 32.f, 32.f}
  );
}
ability get_ability_firetrail_next_level(ability abl) {
  upgrade_ability_firetrail(&abl);
  return abl;
}

void update_ability_firetrail(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_firetrail()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_FIRETRAIL) {
    TraceLog(LOG_WARNING, "ability::update_firetrail()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_FIRETRAIL, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_firetrail()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr || state->in_ingame_info == nullptr || state->in_ingame_info->nearest_spawn == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_firetrail()::Ability pointer(s) not valid");
    return;
  }
  player_state* p_player = reinterpret_cast<player_state*>(abl->p_owner);

  Vector2 ability_projectile_dimentions = Vector2 {abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
  Vector2 ability_position = VECTOR2(
    p_player->position.x + p_player->dimentions_div2.x, 
    p_player->position.y + p_player->dimentions.y
  );
  abl->position = ability_position;

  for (size_t itr_000 = 0; itr_000 < abl->projectiles.size(); itr_000++) {
    projectile* prj = __builtin_addressof(abl->projectiles.at(itr_000));
    if (prj->duration <= 0) {
      abl->projectiles.erase(abl->projectiles.begin() + itr_000);
      continue;
    }
    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj->collision.x), static_cast<i16>(prj->collision.y), static_cast<i16>(prj->collision.width), static_cast<i16>(prj->collision.height), 
      static_cast<i16>(prj->damage + p_player->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(__builtin_addressof(prj->animations.at(prj->active_sprite)));
    prj->duration -= GetFrameTime();
  }

  Rectangle last_placed_projectile_dest = Rectangle { 
    abl->vec_ex.f32[0] + abl->vec_ex.f32[2] * .25f, abl->vec_ex.f32[1] + abl->vec_ex.f32[3] * .5f,
    abl->vec_ex.f32[2] * .5f, abl->vec_ex.f32[3] * .5f
  };

  Rectangle player_collision = Rectangle {
    p_player->collision.x + (p_player->dimentions.x * .25f), p_player->collision.y + (p_player->dimentions.y * .8f),
    p_player->dimentions.x * .5f, p_player->dimentions.y * .2f 
  };

  if (!CheckCollisionRecs(last_placed_projectile_dest, player_collision)) {
    projectile prj = projectile();
    prj.position = ability_position;

    prj.collision = Rectangle {
      prj.position.x - (ability_projectile_dimentions.x * .5f), prj.position.y - ability_projectile_dimentions.y,
      ability_projectile_dimentions.x, ability_projectile_dimentions.y
    };

    abl->vec_ex.f32[0] = prj.collision.x;
    abl->vec_ex.f32[1] = prj.collision.y;
    abl->vec_ex.f32[2] = prj.collision.width;
    abl->vec_ex.f32[3] = prj.collision.height;

    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    prj.active_sprite = 0;
    for (size_t itr_000 = 0; itr_000 < abl->animation_ids.size(); ++itr_000) {
      if (abl->animation_ids.at(itr_000) <= SHEET_ID_SPRITESHEET_UNSPECIFIED || abl->animation_ids.at(itr_000) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        return;
      }
      spritesheet spr = spritesheet();
      spr.sheet_id = abl->animation_ids.at(itr_000);
      set_sprite(__builtin_addressof(spr), true, false);
      spr.coord.width = spr.current_frame_rect.width * abl->proj_sprite_scale;
      spr.coord.height = spr.current_frame_rect.height * abl->proj_sprite_scale;
      spr.origin = VECTOR2(spr.coord.width * .5f ,  spr.coord.height);

      prj.animations.push_back(spr);
    }

    abl->projectiles.push_back(prj);
  }
}

void render_ability_firetrail(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::render_firetrail()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_FIRETRAIL) {
    TraceLog(LOG_WARNING, "ability::render_firetrail()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_FIRETRAIL, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_firetrail()::Ability is not active or not initialized");
    return;
  }
  //player_state* p_player = reinterpret_cast<player_state*>(abl->p_owner);
  for (size_t itr_000 = 0; itr_000 < abl->projectiles.size(); ++itr_000) {
    projectile& prj = abl->projectiles.at(itr_000);
    if (!prj.is_active) { continue; }
    play_sprite_on_site(__builtin_addressof(prj.animations.at(prj.active_sprite)), WHITE, Rectangle { 
      prj.position.x, prj.position.y, prj.animations.at(prj.active_sprite).coord.width, 
      prj.animations.at(prj.active_sprite).coord.height
    });
  }
}

void refresh_ability_firetrail(ability* abl) { 
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_firetrail()::Ability is null");
    return;
  }
  if (abl->type != ABILITY_TYPE_FIRETRAIL) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_firetrail()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_FIRETRAIL, abl->type);
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_firetrail()::Ability projectile count exceed");
    return;
  }
  abl->animation_ids.clear();
  abl->animation_ids.push_back(SHEET_ID_ABILITY_FIRETRAIL_START_ANIMATION);
  abl->animation_ids.push_back(SHEET_ID_ABILITY_FIRETRAIL_LOOP_ANIMATION);
  abl->animation_ids.push_back(SHEET_ID_ABILITY_FIRETRAIL_END_ANIMATION);

  abl->vec_ex.f32[0] = -5000.f; // x INFO: Random numbers where player can't go for placing first projectile, maybe there is better way to do it but whatever
  abl->vec_ex.f32[1] = -5000.f; // y
  abl->vec_ex.f32[2] = 0.f; // width
  abl->vec_ex.f32[3] = 0.f; // height
}
