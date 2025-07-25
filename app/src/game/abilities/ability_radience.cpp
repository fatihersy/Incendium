#include "ability_radience.h"
#include <reasings.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game/spritesheet.h"

#define RADIENCE_COLOR_TRANSPARENT  CLITERAL(Color){ 255, 255, 255, 0 }
#define RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW  CLITERAL(Color){ 253, 203, 110, 32 }
#define RADIENCE_CIRCLE_RADIUS_CHANGE 50.f

typedef struct ability_radience_state {
  std::array<ability, ABILITY_TYPE_MAX> abilities;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
} ability_radience_state;
static ability_radience_state * state;

bool ability_radience_initialize(const camera_metrics* _camera_metrics,const app_settings* _settings,const ingame_info* _ingame_info) {
  if (state) {
    TraceLog(LOG_WARNING, "ability::ability_system_initialize()::Init called multiple times");
    return false;
  }
  state = (ability_radience_state*)allocate_memory_linear(sizeof(ability_radience_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_radience(ability* abl) {
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
ability get_ability_radience(void) {
  if (!state) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> radience_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability("Radience", ABILITY_TYPE_RADIANCE,
    radience_upgr,
    0.f, 2.2f, Vector2 {0.8f, 0.8f}, 1, 0, 0.f, 11,
    Vector2{128.f, 128.f}, Rectangle{2272, 1376, 32, 32}
  );
}
ability get_ability_radience_next_level(ability abl) {
  upgrade_ability_radience(&abl);
  return abl;
}

void update_ability_radience(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_radience()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_RADIANCE) {
    TraceLog(LOG_WARNING, "ability::update_radience()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_RADIANCE, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_radience()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr || state->in_ingame_info == nullptr || state->in_ingame_info->nearest_spawn == nullptr) {
    return;
  }
  player_state* p_player = reinterpret_cast<player_state*>(abl->p_owner);
  abl->position    = p_player->position;
  abl->position.x += p_player->dimentions_div2.x;
  abl->position.y += p_player->dimentions_div2.y;

  projectile& prj  = abl->projectiles.at(0);
  if (!prj.is_active) { return; }

  prj.position    = abl->position;
  prj.collision.x = prj.position.x;
  prj.collision.y = prj.position.y;

  u16  frame_counter_max           = TARGET_FPS * 1;
  u16  circle_inner_radius_base    = prj.mm_ex.u16[0];
  u16  circle_outer_radius_base    = prj.mm_ex.u16[1];
  u16& frame_counter               = prj.mm_ex.u16[2];
  //f32  circle_inner_radius_current = static_cast<f32>(prj.mm_ex.u16[3]);
  //f32  circle_outer_radius_current = static_cast<f32>(prj.mm_ex.u16[4]);
  bool  is_increment               = static_cast<bool>(prj.mm_ex.u16[5]);
  
  f32 distance_inner = RADIENCE_CIRCLE_RADIUS_CHANGE;
  f32 distance_outer = RADIENCE_CIRCLE_RADIUS_CHANGE;

  frame_counter++;
  frame_counter = FCLAMP(frame_counter, 0, frame_counter_max);
  if (frame_counter >= frame_counter_max) {
    frame_counter = 0;
    prj.mm_ex.u16[5] = !is_increment;
    is_increment = prj.mm_ex.u16[5];
  }

  if (is_increment) {
    prj.mm_ex.u16[3] = EaseCircIn((f32)frame_counter, circle_inner_radius_base, distance_inner, (f32)frame_counter_max);
    prj.mm_ex.u16[4] = EaseCircIn((f32)frame_counter, circle_outer_radius_base, distance_outer, (f32)frame_counter_max);
  }
  else {
    prj.mm_ex.u16[3] = EaseQuadOut((f32)frame_counter, circle_inner_radius_base + distance_inner, -distance_inner,  (f32)frame_counter_max);
    prj.mm_ex.u16[4] = EaseQuadOut((f32)frame_counter, circle_outer_radius_base + distance_outer, -distance_outer,  (f32)frame_counter_max);
  }

  event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
    static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
    static_cast<i16>(prj.damage + p_player->damage),
    static_cast<i16>(COLLISION_TYPE_CIRCLE_RECTANGLE)
  ));

  update_sprite(__builtin_addressof(prj.animations.at(prj.active_sprite)));
}

void render_ability_radience(ability* abl){
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::render_radience()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_RADIANCE) {
    TraceLog(LOG_WARNING, "ability::render_radience()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_RADIANCE, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_radience()::Ability is not active or not initialized");
    return;
  }
  if (!abl->projectiles.at(0).is_active) { return; }
  projectile * prj = __builtin_addressof(abl->projectiles.at(0));
  
  Vector2 dim = Vector2 {
    prj->animations.at(prj->active_sprite).current_frame_rect.width  * abl->proj_sprite_scale,
    prj->animations.at(prj->active_sprite).current_frame_rect.height * abl->proj_sprite_scale
  };
  prj->animations.at(prj->active_sprite).origin.x = dim.x / 2.f;
  prj->animations.at(prj->active_sprite).origin.y = dim.y / 2.f;
  play_sprite_on_site(__builtin_addressof(prj->animations.at(prj->active_sprite)), WHITE, Rectangle { prj->position.x, prj->position.y, dim.x, dim.y });

  //u16  frame_counter_max     = TARGET_FPS * 1;
  //u16& circle_inner_radius_base = prj->mm_ex.u16[0];
  //u16& circle_outer_radius_base = prj->mm_ex.u16[1];
  //u16& frame_counter         = prj->mm_ex.u16[2];
  f32  circle_inner_radius_current = static_cast<f32>(prj->mm_ex.u16[3]);
  f32  circle_outer_radius_current = static_cast<f32>(prj->mm_ex.u16[4]);
  //bool  is_increment = static_cast<bool>(prj->mm_ex.u16[5]);

  DrawCircleGradient(prj->collision.x, prj->collision.y, circle_outer_radius_current, RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW, RADIENCE_COLOR_TRANSPARENT);
  DrawCircleGradient(prj->collision.x, prj->collision.y, circle_inner_radius_current, RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW, RADIENCE_COLOR_TRANSPARENT);
}

void refresh_ability_radience(ability* abl) { 
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_radience()::Ability is null");
    return;
  }
  if (abl->type != ABILITY_TYPE_RADIANCE) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_radience()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_RADIANCE, abl->type);
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_radience()::Ability projectile count exceed");
    return;
  }

  abl->projectiles.clear();
  abl->animation_ids.clear();
  abl->projectiles.reserve(abl->proj_count);
  abl->animation_ids.push_back(SHEET_ID_GENERIC_LIGHT);

  {
    projectile prj = projectile();
    prj.collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    for (size_t itr_000 = 0; itr_000 < abl->animation_ids.size(); ++itr_000) {
      if (abl->animation_ids.at(itr_000) <= SHEET_ID_SPRITESHEET_UNSPECIFIED || abl->animation_ids.at(itr_000) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        TraceLog(LOG_WARNING, "ability::refresh_ability_radience()::One of spritesheets is not initialized or ability corrupted");
        return;
      }
      spritesheet spr = spritesheet();
      spr.sheet_id = abl->animation_ids.at(itr_000);
      set_sprite(__builtin_addressof(spr), true, false);
      spr.origin = VECTOR2( spr.coord.width * .5f,  spr.coord.height * .5f );

      prj.animations.push_back(spr); 
    }
    abl->projectiles.push_back(prj);
  }

  projectile& prj = abl->projectiles.at(0);
  prj.active_sprite = 0;

  u16& circle_inner_radius_base = prj.mm_ex.u16[0];
  u16& circle_outer_radius_base = prj.mm_ex.u16[1];
  u16& frame_counter            = prj.mm_ex.u16[2];
  //f32  circle_inner_radius_current = static_cast<f32>(prj.mm_ex.u16[3]);
  //f32  circle_outer_radius_current = static_cast<f32>(prj.mm_ex.u16[4]);
  u16&  is_increment            = prj.mm_ex.u16[5];

  circle_inner_radius_base = (abl->proj_dim.x * abl->proj_collision_scale.x * 1.0f) + (abl->level * .1f);
  circle_outer_radius_base = (abl->proj_dim.x * abl->proj_collision_scale.x * 1.2f) + (abl->level * .1f);
  prj.mm_ex.u16[3] = circle_inner_radius_base;
  prj.mm_ex.u16[4] = circle_outer_radius_base;

  frame_counter = 0;
  is_increment = true;
}
