#include "ability.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "game/spritesheet.h"

#define ABILITIES_BASE_SCALE 2.5f

typedef struct ability_system_state {
  ability abilities[ABILITY_TYPE_MAX];

  camera_metrics* in_camera_metrics;
  app_settings* settings;
} ability_system_state;
static ability_system_state * state;

void movement_satellite(ability* abl);
void movement_bullet(ability* abl);
void movement_comet(ability* abl);
void register_ability(ability abl);

bool ability_system_initialize(camera_metrics* _camera_metrics, app_settings* settings) {
  if (state) {
    return false;
  }
  state = (ability_system_state*)allocate_memory_linear(sizeof(ability_system_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->settings = settings;
  state->in_camera_metrics = _camera_metrics;
  std::array<ability_upgradables, ABILITY_UPG_MAX> fireball_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_SPEED, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Fireball",
    fireball_upgr,
    ABILITY_TYPE_FIREBALL,SHEET_ID_FLAME_ENERGY_ANIMATION, SHEET_ID_SPRITESHEET_UNSPECIFIED, MOVE_TYPE_SATELLITE,
    1.86f, 1, 1, 0.f,
    Vector2{30.f, 30.f}, Rectangle{704, 768, 32, 32},
    15,
    false
  ));
  std::array<ability_upgradables, ABILITY_UPG_MAX> bullet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Bullet", 
    bullet_upgr, 
    ABILITY_TYPE_BULLET,SHEET_ID_FLAME_ENERGY_ANIMATION, SHEET_ID_SPRITESHEET_UNSPECIFIED, MOVE_TYPE_BULLET, 
    3.f, 1, 3, 1.75f, 
    Vector2{30.f, 30.f}, Rectangle{544, 128, 32, 32}, 
    15, 
    false
  ));
  std::array<ability_upgradables, ABILITY_UPG_MAX> comet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Comet",
    comet_upgr,
    ABILITY_TYPE_COMET,SHEET_ID_FIREBALL_ANIMATION,SHEET_ID_FIREBALL_EXPLOTION_ANIMATION,MOVE_TYPE_COMET,
    3.f, 2, 4, 0.f,
    Vector2{30.f, 30.f}, Rectangle{576, 128, 32, 32},
    15,
    false,true
  ));
  return true;
}
void update_abilities(ability_play_system* system) {
  for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
    ability* abl = &system->abilities[i];
    if (!abl->is_active || !abl->is_initialized) continue;

    switch (abl->move_pattern) {
      case MOVE_TYPE_SATELLITE: movement_satellite(abl); break;
      case MOVE_TYPE_BULLET: movement_bullet(abl); break;
      case MOVE_TYPE_COMET: movement_comet(abl); break;
      default: {  
        TraceLog(LOG_ERROR, "ability::update_abilities()::Unsuppported ability movement type");
        break;
      }
    }

    for (size_t j = 0; j< abl->projectiles.size(); ++j) {
      if (!abl->projectiles.at(j).is_active) { continue; }
      player_state* player = (player_state*)abl->p_owner;
      event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
        (i16)abl->projectiles.at(j).collision.x, (i16)abl->projectiles.at(j).collision.y,
        (i16)abl->projectiles.at(j).collision.width, (i16)abl->projectiles.at(j).collision.height,
        static_cast<i16>(abl->projectiles.at(j).damage + player->damage)
      ));
      update_sprite(&abl->projectiles.at(j).default_animation);

      if (abl->projectiles.at(j).play_explosion_animation) {
        update_sprite(&abl->projectiles.at(j).explotion_animation);
        
        if (abl->projectiles.at(j).explotion_animation.current_frame >= abl->projectiles.at(j).explotion_animation.frame_total - 1) {
          abl->projectiles.at(j).play_explosion_animation = false;
          reset_sprite(&abl->projectiles.at(j).explotion_animation, true);
        }
      }
    }
  }
}
void render_abilities(ability_play_system* system) {

  for (int i = 1; i < ABILITY_TYPE_MAX; ++i) {
    if (!system->abilities[i].is_active || !system->abilities[i].is_initialized) { continue; }
    ability abl = system->abilities[i];

    for (size_t j = 0; j < system->abilities[i].projectiles.size(); ++j) {
      if (!abl.projectiles.at(j).is_active) { continue; }
      projectile& proj = abl.projectiles.at(j);
      proj.is_active = true;
      Vector2 dim = Vector2 {
        proj.default_animation.current_frame_rect.width * abl.proj_scale, 
        proj.default_animation.current_frame_rect.height * abl.proj_scale
      };
      proj.default_animation.origin.x = dim.x / 2.f;
      proj.default_animation.origin.y = dim.y / 2.f;
      play_sprite_on_site(&proj.default_animation, WHITE, Rectangle {
        proj.position.x, proj.position.y, dim.x, dim.y
      });

      if (proj.play_explosion_animation) {
        proj.explotion_animation.origin.x = dim.x / 2.f;
        proj.explotion_animation.origin.y = dim.y / 2.f;
        play_sprite_on_site(&proj.explotion_animation, WHITE, Rectangle { proj.buffer.f32[2], proj.buffer.f32[3], dim.x, dim.y });
      }
    }
  }
}

void movement_satellite(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::movement_satellite()::Ability is not active or not initialized");
    return;
  }
  player_state* player = (player_state*)abl->p_owner;

  abl->position.x  = player->collision.x + player->collision.width / 2.f;
  abl->position.y  = player->collision.y + player->collision.height / 2.f;

  abl->rotation += abl->proj_speed;

  if (abl->rotation > 360) abl->rotation = 0;

  for (size_t i = 0; i < abl->projectiles.size(); i++) {
    projectile& proj = abl->projectiles.at(i);
    i16 angle = (i32)(((360.f / abl->projectiles.size()) * i) + abl->rotation) % 360;

    proj.position = get_a_point_of_a_circle( abl->position, (abl->level * 3.f) + player->collision.height + 15, angle);
    proj.collision.x = proj.position.x + ((abl->proj_dim.x * abl->proj_scale) / 2.f) - ((proj.default_animation.current_frame_rect.width  * abl->proj_scale) / 2.f);
    proj.collision.y = proj.position.y + ((abl->proj_dim.y * abl->proj_scale) / 2.f) - ((proj.default_animation.current_frame_rect.height * abl->proj_scale) / 2.f);
  }
}
void movement_bullet(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::movement_bullet()::Ability is not active or not initialized");
    return;
  }
  player_state* player = (player_state*)abl->p_owner; // HACK: Hardcoded character state

  abl->position.x  = player->collision.x + player->collision.width / 2.f;
  abl->position.y  = player->collision.y + player->collision.height / 2.f;

  for (size_t i = 0; i < abl->projectiles.size(); i++) {

    if (abl->projectiles.at(i).duration <= 0) {
      abl->projectiles.at(i).position = player->position;
      abl->projectiles.at(i).duration = abl->proj_duration;
      abl->projectiles.at(i).direction = player->w_direction;
    }
    else {
      abl->projectiles.at(i).duration -= GetFrameTime();
    }
    
    abl->projectiles.at(i).position.x += abl->projectiles.at(i).direction == WORLD_DIRECTION_RIGHT ? abl->proj_speed : -abl->proj_speed;

    abl->projectiles.at(i).collision.x = abl->projectiles.at(i).position.x;
    abl->projectiles.at(i).collision.y = abl->projectiles.at(i).position.y;
    
  }
}

/**
 * @brief buffer summary: {f32[0], f32[1]}, {f32[2], f32[3]}  = {target x, target y}, {explosion.x, explosion.y}
 */
void movement_comet(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::movement_comet()::Ability is not active or not initialized");
    return;
  }
  const Rectangle* frustum = &state->in_camera_metrics->frustum;

  for (size_t i = 0; i < abl->projectiles.size(); i++) {
    if (vec2_equals(abl->projectiles.at(i).position, pVECTOR2(abl->projectiles.at(i).buffer.f32), .1)) {
      abl->projectiles.at(i).buffer.f32[2] =  abl->projectiles.at(i).position.x;
      abl->projectiles.at(i).buffer.f32[3] =  abl->projectiles.at(i).position.y;
      abl->projectiles.at(i).play_explosion_animation = true;

      const f32 rand = get_random(frustum->x, frustum->x + frustum->width);
      abl->projectiles.at(i).position = VECTOR2(rand, frustum->y - abl->proj_dim.y);
      Vector2 new_pos = {
        (f32)get_random(frustum->x + frustum->width * .1f, frustum->x + frustum->width - frustum->width * .1f),
        (f32)get_random(frustum->y + frustum->height * .2f, frustum->y + frustum->height - frustum->height * .2f)
      };
      abl->projectiles.at(i).buffer.f32[0] = new_pos.x;
      abl->projectiles.at(i).buffer.f32[1] = new_pos.y;
      abl->projectiles.at(i).default_animation.rotation = get_movement_rotation(abl->projectiles.at(i).position, pVECTOR2(abl->projectiles.at(i).buffer.f32)) + 270.f;
    }
    else {
      abl->projectiles.at(i).position = move_towards(abl->projectiles.at(i).position, pVECTOR2(abl->projectiles.at(i).buffer.f32), abl->proj_speed);
      abl->projectiles.at(i).collision.x = abl->projectiles.at(i).position.x;
      abl->projectiles.at(i).collision.y = abl->projectiles.at(i).position.y;
    }
  }
}

void upgrade_ability(ability* abl) {
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

ability get_ability(ability_type _type) {
  if (!state) {
    return ability{};
  }
  ability abl = state->abilities[_type];
  return abl;
}
ability get_next_level(ability abl) {
  upgrade_ability(&abl);
  return abl;
}
void register_ability(ability abl) { state->abilities[abl.type] = abl; }

void refresh_ability(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability()::Ability is null");
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability()::Ability projectile count exceed");
    return;
  }

  abl->projectiles.clear();
  abl->projectiles.reserve(abl->proj_count);

  for (int i = 0; i < abl->proj_count; ++i) {
    projectile* prj = __builtin_addressof(abl->projectiles.emplace_back());
    prj->collision = Rectangle {0.f, 0.f, abl->proj_dim.x, abl->proj_dim.y};
    prj->damage = abl->base_damage;
    prj->is_active = true;
    prj->duration = abl->proj_duration;
    if (abl->default_animation_id != SHEET_ID_SPRITESHEET_UNSPECIFIED) {
      prj->default_animation.sheet_id = abl->default_animation_id;
      set_sprite(__builtin_addressof(prj->default_animation), true, false, abl->center_proj_anim);
    }
    if (abl->explosion_animation_id != SHEET_ID_SPRITESHEET_UNSPECIFIED) {
      prj->explotion_animation.sheet_id = abl->explosion_animation_id;
      set_sprite(__builtin_addressof(prj->explotion_animation), false, true, abl->center_proj_anim);
    }
    if (abl->centered_origin) {
      prj->default_animation.origin = VECTOR2(
        prj->default_animation.coord.width * .5f, 
        prj->default_animation.coord.height * .5f
      );
    }
  }
}
