#include "ability_radience.h"
#include <reasings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"

#include "game/spritesheet.h"

#define RADIENCE_COLOR_TRANSPARENT  CLITERAL(Color){ 255, 255, 255, 0 }
#define RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW  CLITERAL(Color){ 253, 203, 110, 32 }
#define RADIENCE_CIRCLE_RADIUS_CHANGE 50.f
#define RADIENCE_PULSE_IN_DURATION .75f
#define RADIENCE_PULSE_OUT_DURATION .75f
#define RADIENCE_CIRCLE_RADIUS_INNER_SCALE 1.0f
#define RADIENCE_CIRCLE_RADIUS_OUTER_SCALE 1.2f
#define RADIENCE_CIRCLE_RADIUS_PULSE_SCALE 1.3f

enum radience_sequence {
  RADIENCE_SEQUENCE_UNDEFINED,
  RADIENCE_SEQUENCE_PULSE_OUT,
  RADIENCE_SEQUENCE_PULSE_IN,
  RADIENCE_SEQUENCE_MAX,
};

struct ability_radience_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info;

  radience_sequence current_sequence;
  f32 pulse_in_duration;
  f32 pulse_out_duration;
  f32 pulse_accumulator;

  ability_radience_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
    this->current_sequence = RADIENCE_SEQUENCE_UNDEFINED;
    this->pulse_in_duration = 0.f;
    this->pulse_out_duration = 0.f;
    this->pulse_accumulator = 0.f;
  }
};
static ability_radience_state * state = nullptr;

bool ability_radience_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info) {
  if (state and state != nullptr) {
    IWARN("ability::ability_system_initialize()::Init called multiple times");
    return false;
  }
  state = (ability_radience_state*)allocate_memory_linear(sizeof(ability_radience_state),true);
  if (not state or state == nullptr) {
    IERROR("ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  *state = ability_radience_state();
  
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_radience(ability& abl) {
  if (abl.id <= ABILITY_ID_UNDEFINED or abl.id >= ABILITY_ID_MAX) {
    IWARN("ability::upgrade_ability()::Ability is not initialized");
    return;
  }
  ++abl.level;

  for (size_t itr_000 = 0u; itr_000 < ABILITY_UPG_MAX; ++itr_000) {
    switch (abl.upgradables.at(itr_000)) {
      case ABILITY_UPG_DAMAGE: {
        abl.base_damage += abl.level * 2;
        break;
      }
      case ABILITY_UPG_AMOUNT: { break; }
      case ABILITY_UPG_HITBOX: { break; } // TODO: projectile hitbox upgrade
      case ABILITY_UPG_SPEED:  { break; } // TODO: projectile speed upgrade
      default: return;
    }
  }
}
ability get_ability_radience(void) {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> radience_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_RADIENCE), ABILITY_ID_RADIANCE,
    radience_upgr,
    0.f, 2.2f, Vector2 {0.8f, 0.8f}, 1, 0, 0.f, 11,
    Vector2{128.f, 128.f}, Rectangle{2272, 1376, 32, 32}
  );
}
ability get_ability_radience_next_level(ability abl) {
  upgrade_ability_radience(abl);
  return abl;
}

void update_ability_radience(ability& abl) {
  if (not abl.is_active or not abl.is_initialized) {
    IWARN("ability::update_ability_radience()::Ability is not active or not initialized");
    return;
  }
  if (abl.id != ABILITY_ID_RADIANCE) {
    IWARN("ability::update_ability_radience()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_RADIANCE, abl.id);
    return;
  }
  if (abl.p_owner == nullptr or state->in_ingame_info == nullptr) {
    return;
  }
  if (state->current_sequence <= RADIENCE_SEQUENCE_UNDEFINED or state->current_sequence >= RADIENCE_SEQUENCE_MAX) {
    IWARN("ability::update_ability_radience()::Ability state not initialized correctly");
    return;
  }
  const player_state *const player = reinterpret_cast<player_state*>(abl.p_owner);
  abl.position = player->position;

  projectile& prj = abl.projectiles.at(0);
  if (not prj.is_active) { return; }

  prj.position    = abl.position;
  prj.collision.x = prj.position.x;
  prj.collision.y = prj.position.y;

  //u16  frame_counter_max           = TARGET_FPS * 1;
  //u16  circle_inner_radius_base    = prj.mm_ex.u16[0];
  //u16  circle_outer_radius_base    = prj.mm_ex.u16[1];
  //u16& frame_counter               = prj.mm_ex.u16[2];
  //f32  circle_inner_radius_current = static_cast<f32>(prj.mm_ex.u16[3]);
  //f32  circle_outer_radius_current = static_cast<f32>(prj.mm_ex.u16[4]);

  f32  circle_inner_radius_base    = prj.vec_ex.f32[0];
  f32  circle_outer_radius_base    = prj.vec_ex.f32[1];
  f32& circle_inner_radius_current = prj.vec_ex.f32[2];
  f32& circle_outer_radius_current = prj.vec_ex.f32[3];
  f32 distance_inner = circle_inner_radius_base * RADIENCE_CIRCLE_RADIUS_PULSE_SCALE;
  f32 distance_outer = circle_outer_radius_base * RADIENCE_CIRCLE_RADIUS_PULSE_SCALE;
  state->pulse_accumulator += (*state->in_ingame_info->delta_time);

  switch (state->current_sequence) {
    case RADIENCE_SEQUENCE_PULSE_OUT:{
      if (state->pulse_accumulator >= state->pulse_out_duration) {
        state->pulse_accumulator = 0.f;
        state->current_sequence = RADIENCE_SEQUENCE_PULSE_IN;
      }
      circle_inner_radius_current = EaseCircIn((f32)state->pulse_accumulator, circle_inner_radius_base, distance_inner, state->pulse_out_duration);
      circle_outer_radius_current = EaseCircIn((f32)state->pulse_accumulator, circle_outer_radius_base, distance_outer, state->pulse_out_duration);
      break;
    }
    case RADIENCE_SEQUENCE_PULSE_IN:{
      if (state->pulse_accumulator >= state->pulse_out_duration) {
        state->pulse_accumulator = 0.f;
        state->current_sequence = RADIENCE_SEQUENCE_PULSE_IN;
      }
      circle_inner_radius_current = EaseQuadOut((f32)state->pulse_accumulator, circle_inner_radius_base + distance_inner, -distance_inner,  state->pulse_in_duration);
      circle_outer_radius_current = EaseQuadOut((f32)state->pulse_accumulator, circle_outer_radius_base + distance_outer, -distance_outer,  state->pulse_in_duration);
      break;
    }
    default:
  }
  i16 base_damage = static_cast<i16>(prj.damage);
  i16 final_damage = base_damage + (base_damage * player->stats.at(CHARACTER_STATS_OVERALL_DAMAGE).buffer.f32[3]);
  event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
    static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
    final_damage, static_cast<i16>(COLLISION_TYPE_CIRCLE_RECTANGLE)
  ));
  update_sprite(prj.animations.at(prj.active_sprite), (*state->in_ingame_info->delta_time) );
}

void render_ability_radience(ability& abl){
  if (abl.id != ABILITY_ID_RADIANCE) {
    IWARN("ability::render_radience()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_RADIANCE, abl.id);
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    IWARN("ability::update_radience()::Ability is not active or not initialized");
    return;
  }
  if (abl.p_owner == nullptr or state->in_ingame_info == nullptr) {
    return;
  }
  if (not abl.projectiles.at(0).is_active) { return; }
  projectile *const prj = __builtin_addressof(abl.projectiles.at(0));
  
  Vector2 dim = Vector2 {
    prj->animations.at(prj->active_sprite).current_frame_rect.width  * abl.proj_sprite_scale,
    prj->animations.at(prj->active_sprite).current_frame_rect.height * abl.proj_sprite_scale
  };
  prj->animations.at(prj->active_sprite).origin.x = dim.x / 2.f;
  prj->animations.at(prj->active_sprite).origin.y = dim.y / 2.f;
  play_sprite_on_site(prj->animations.at(prj->active_sprite), WHITE, Rectangle { prj->position.x, prj->position.y, dim.x, dim.y });

  //u16  frame_counter_max     = TARGET_FPS * 1;
  //u16& circle_inner_radius_base = prj->mm_ex.u16[0];
  //u16& circle_outer_radius_base = prj->mm_ex.u16[1];
  //u16& frame_counter         = prj->mm_ex.u16[2];
  f32 circle_inner_radius_current = static_cast<f32>(prj->vec_ex.f32[2]);
  f32 circle_outer_radius_current = static_cast<f32>(prj->vec_ex.f32[3]);
  //bool  is_increment = static_cast<bool>(prj->mm_ex.u16[5]);

  DrawCircleGradient(prj->collision.x, prj->collision.y, circle_outer_radius_current, RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW, RADIENCE_COLOR_TRANSPARENT);
  DrawCircleGradient(prj->collision.x, prj->collision.y, circle_inner_radius_current, RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW, RADIENCE_COLOR_TRANSPARENT);
}

void refresh_ability_radience(ability& abl) { 
  if (abl.id != ABILITY_ID_RADIANCE) {
    IWARN("ability::refresh_ability_radience()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_RADIANCE, abl.id);
    return;
  }
  if (abl.proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    IWARN("ability::refresh_ability_radience()::Ability projectile count exceed");
    return;
  }
  abl.projectiles.clear();
  abl.animation_ids.clear();
  abl.projectiles.reserve(abl.proj_count);
  abl.animation_ids.push_back(SHEET_ID_GENERIC_LIGHT);
  
  projectile& prj = abl.projectiles.emplace_back(projectile());
  
  prj.collision = Rectangle {0.f, 0.f, abl.proj_dim.x * abl.proj_collision_scale.x, abl.proj_dim.y * abl.proj_collision_scale.y};
  prj.damage = abl.base_damage;
  prj.is_active = true;
  prj.duration = abl.proj_duration;
  for (size_t itr_000 = 0u; itr_000 < abl.animation_ids.size(); ++itr_000) {
    if (abl.animation_ids.at(itr_000) <= SHEET_ID_SPRITESHEET_UNSPECIFIED or abl.animation_ids.at(itr_000) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
      IWARN("ability::refresh_ability_radience()::One of spritesheets is not initialized or ability corrupted");
      return;
    }
    spritesheet spr = spritesheet();
    spr.sheet_id = abl.animation_ids.at(itr_000);
    set_sprite(spr, true, false);
    spr.origin = VECTOR2( spr.coord.width * .5f,  spr.coord.height * .5f );
    prj.animations.push_back(spr); 
  }
  
  prj.active_sprite = 0;

  //u16& circle_inner_radius_base = prj.mm_ex.u16[0];
  //u16& circle_outer_radius_base = prj.mm_ex.u16[1];
  //u16& frame_counter            = prj.mm_ex.u16[2];
  //f32  circle_inner_radius_current = static_cast<f32>(prj.mm_ex.u16[3]);
  //f32  circle_outer_radius_current = static_cast<f32>(prj.mm_ex.u16[4]);
  //u16&  is_increment            = prj.mm_ex.u16[5];

  f32& circle_inner_radius_base = prj.vec_ex.f32[0];
  f32& circle_outer_radius_base = prj.vec_ex.f32[1];

  circle_inner_radius_base = (abl.proj_dim.x * abl.proj_collision_scale.x * RADIENCE_CIRCLE_RADIUS_INNER_SCALE) + (abl.level * .1f);
  circle_outer_radius_base = (abl.proj_dim.x * abl.proj_collision_scale.x * RADIENCE_CIRCLE_RADIUS_OUTER_SCALE) + (abl.level * .1f);
  
  state->current_sequence = RADIENCE_SEQUENCE_PULSE_OUT;
  state->pulse_in_duration = RADIENCE_SEQUENCE_PULSE_IN;
  state->pulse_out_duration = RADIENCE_SEQUENCE_PULSE_OUT;
}
