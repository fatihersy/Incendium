#include "ability_harvester.h"
#include <reasings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/fmath.h"

#include "game/spritesheet.h"

enum harvester_phase { UNDEFINED, IDLE, BURST, RETURN, MAX };

struct ability_harvester {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
  ability_harvester(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr; 
  }
};

static ability_harvester * state = nullptr;

#define HARVESTER_EFFECT_WIDTH state->in_settings->render_height * .25f
#define HARVESTER_EFFECT_HEIGHT state->in_settings->render_height * .25f
#define HARVESTER_HEAD_WIDTH state->in_settings->render_height * .05f
#define HARVESTER_HEAD_HEIGHT state->in_settings->render_height * .05f
#define HARVESTER_TRAIL_WIDTH state->in_settings->render_height * .15f
#define HARVESTER_TRAIL_HEIGHT state->in_settings->render_height * .15f

#define PRJ_TARGET_INDEX mm_ex.i32[0]
#define PRJ_TARGET_ID mm_ex.i32[1]
#define PRJ_PHASE mm_ex.i32[2]

#define PRJ_TARGET_ORIGIN_X vec_ex.f32[0]
#define PRJ_TARGET_ORIGIN_Y vec_ex.f32[1]

#define ANIM_IDX_EFFECT 0
#define ANIM_IDX_HEAD   1
#define ANIM_IDX_TRAIL  2

//constexpr f32 idle_phase_blinking_frequency = 0.4f;
constexpr i32 ability_animation_count = 3;
constexpr f32 burst_phase_duration = 1.1f;
constexpr f32 arrival_threshold = 15.0f; 

bool is_on_screen_spawn_valid(const std::vector<Character2D>& spawns, const element_handle* handle) {
  if (not handle) return false;
  return handle->index < spawns.size() and spawns[handle->index].character_id == handle->id;
}

void begin_idle(ability& abl) {
  abl.ability_cooldown_accumulator = 0.f;
  projectile& prj = abl.projectiles[0];
  prj.PRJ_TARGET_ID = 0;
  prj.PRJ_TARGET_INDEX = std::numeric_limits<i32>::max();
  prj.PRJ_PHASE = static_cast<i32>(harvester_phase::IDLE);
  prj.accumulator = 0.f;

  const player_state* player = reinterpret_cast<player_state*>(abl.p_owner);
  if (player) {
    prj.position = player->position;
    prj.PRJ_TARGET_ORIGIN_X = player->position.x;
    prj.PRJ_TARGET_ORIGIN_Y = player->position.y;

    prj.animations[ANIM_IDX_HEAD].coord.x = prj.position.x;
    prj.animations[ANIM_IDX_HEAD].coord.y = prj.position.y;
    prj.animations[ANIM_IDX_TRAIL].coord.x = prj.position.x;
    prj.animations[ANIM_IDX_TRAIL].coord.y = prj.position.y;
    prj.animations[ANIM_IDX_EFFECT].coord.x = prj.position.x;
    prj.animations[ANIM_IDX_EFFECT].coord.y = prj.position.y;
  }
}

void begin_burst(projectile& prj) {
  prj.PRJ_PHASE = harvester_phase::BURST;
  prj.accumulator = 0.f;
}

void begin_return(ability& abl, const std::vector<Character2D>& spawns, const element_handle* handle) {
  if (is_on_screen_spawn_valid(spawns, handle)) {
    abl.projectiles[0].PRJ_PHASE = harvester_phase::RETURN;
    const Character2D& spw = spawns[handle->index];
    projectile& prj = abl.projectiles[0];

    Vector2 burst_pos {
      spw.collision.x + spw.collision.width * .5f,
      spw.collision.y + spw.collision.height * .5f
    };

    prj.animations[ANIM_IDX_EFFECT].coord.x = burst_pos.x;
    prj.animations[ANIM_IDX_EFFECT].coord.y = burst_pos.y;

    prj.PRJ_TARGET_ORIGIN_X = burst_pos.x;
    prj.PRJ_TARGET_ORIGIN_Y = burst_pos.y;

    prj.position = burst_pos;
    reset_sprite(prj.animations[ANIM_IDX_EFFECT], true);
  }
  else begin_burst(abl.projectiles[0]);
}


bool ability_harvester_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info) {
  if (state and state != nullptr) {
    IWARN("ability::ability_harvester()::Initialize called multiple times");
    return false;
  }
  state = (ability_harvester*)allocate_memory_linear(sizeof(ability_harvester), true);
  if (not state or state == nullptr) {
    IERROR("ability::ability_harvester()::Failed to allocate state");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_harvester(ability& abl) {
  if (abl.id <= ABILITY_ID_UNDEFINED or abl.id >= ABILITY_ID_MAX) {
    IWARN("ability::upgrade_ability_harvester()::Ability is not initialized");
    return;
  }
  ++abl.level;

  for (ability_upgradables& upg : abl.upgradables) {
    switch (upg) {
      case ABILITY_UPG_DAMAGE: {
        abl.base_damage += abl.level * 2;
        break;
      }
      case ABILITY_UPG_HITBOX: { break; } // TODO: projectile hitbox upgrade
      case ABILITY_UPG_SPEED:  { break; } // TODO: projectile speed upgrade
      default: return;
    }
  }
}
ability get_ability_harvester(void) {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> harvester_upgr = {
    ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED
  };
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_HARVESTER), ABILITY_ID_HARVESTER,
    harvester_upgr,
    0.75f, 1.f, Vector2 {1.f, 1.f}, 1, 800.f, 1.75f, 15,
    Vector2{30.f, 30.f}, Rectangle{2400, 928, 32, 32}
  );
}
ability get_ability_harvester_next_level(ability abl) {
  upgrade_ability_harvester(abl);
  return abl;
}

void update_ability_harvester(ability& abl) {
  if (abl.id != ABILITY_ID_HARVESTER) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or not abl.p_owner or abl.p_owner == nullptr) {
    return;
  }
  if (abl.projectiles.empty() or abl.projectiles[0].animations.size() != ability_animation_count or not state->in_ingame_info->first_spawn_on_screen_handle) {
    return;
  }
  const std::vector<Character2D> * const spawns_ptr = state->in_ingame_info->in_spawns;
  const element_handle * const spw_on_screen = state->in_ingame_info->first_spawn_on_screen_handle;
  projectile& prj = abl.projectiles[0];

  switch (static_cast<harvester_phase>(prj.PRJ_PHASE)) {
    case IDLE: {
      if ( abl.ability_cooldown_accumulator < abl.ability_cooldown_duration) {
        abl.ability_cooldown_accumulator += (*state->in_ingame_info->delta_time);
        return;
      }
      if (is_on_screen_spawn_valid((*spawns_ptr), spw_on_screen)) {
        prj.PRJ_TARGET_ID = spw_on_screen->id;
        prj.PRJ_TARGET_INDEX = spw_on_screen->index;
        begin_burst(prj);
      } 
      return;
    }
    case BURST: {
      if (prj.accumulator > burst_phase_duration) {
        begin_return(abl, (*spawns_ptr), spw_on_screen);
        return;
      }
      if (not is_on_screen_spawn_valid( (*spawns_ptr), spw_on_screen)) {
        return;
      }
      prj.accumulator += (*state->in_ingame_info->delta_time);

      //const f32 blink_t = static_cast<f32>(fast_sin(static_cast<f64>(prj.accumulator * idle_phase_blinking_frequency)));
      //const i16 a_channel = static_cast<i16>(((blink_t + 1.0f) * 0.5f) * 200.0f);
      event_fire(EVENT_CODE_SET_SPAWN_TINT, event_context(static_cast<i16>(spw_on_screen->id), static_cast<i16>(spw_on_screen->index), 255, 0, 0, 255));
      return;
    }
    case RETURN: {
      if (not prj.animations[ANIM_IDX_EFFECT].is_played) {
        update_sprite(prj.animations[ANIM_IDX_EFFECT], (*state->in_ingame_info->delta_time));
      }
      const player_state* player = reinterpret_cast<player_state*>(abl.p_owner);

      const f32 dx = player->position.x - prj.position.x;
      const f32 dy = player->position.y - prj.position.y;
      const f32 distance = sqrt(dx * dx + dy * dy);

      if (distance <= arrival_threshold) {
        begin_idle(abl);
        return;
      } else {
        if (distance > 0.0001f) {
          prj.position.x += (dx / distance) * (abl.proj_speed * (*state->in_ingame_info->delta_time));
          prj.position.y += (dy / distance) * (abl.proj_speed * (*state->in_ingame_info->delta_time));
        }
      }

      {
        spritesheet& sheet = prj.animations[ANIM_IDX_HEAD];
        sheet.coord.x = prj.position.x;
        sheet.coord.y = prj.position.y;
        update_sprite(sheet, (*state->in_ingame_info->delta_time));
      }
      {
        spritesheet& sheet = prj.animations[ANIM_IDX_TRAIL];
        sheet.coord.x = prj.position.x;
        sheet.coord.y = prj.position.y;
        sheet.rotation = get_movement_rotation(Vector2 { prj.PRJ_TARGET_ORIGIN_X, prj.PRJ_TARGET_ORIGIN_Y }, prj.position) + 280.f;

        update_sprite(sheet, (*state->in_ingame_info->delta_time));
      }
      return;
    }
    default: {
      begin_idle(abl);
      return;
    }
  }
}
void render_ability_harvester(ability& abl){
  if (abl.id != ABILITY_ID_HARVESTER) {
    return;
  }
  if (not abl.is_active or not abl.is_initialized or not abl.p_owner) {
    return;
  }
  if (abl.projectiles.empty() or abl.projectiles[0].animations.size() != ability_animation_count or not state->in_ingame_info->first_spawn_on_screen_handle) {
    return;
  }
  projectile& prj = abl.projectiles[0];

  if (static_cast<harvester_phase>(prj.PRJ_PHASE) == harvester_phase::RETURN) {
    spritesheet& sheet_effect = prj.animations[ANIM_IDX_EFFECT];
    play_sprite_on_site_pro(sheet_effect, sheet_effect.coord, sheet_effect.origin, sheet_effect.rotation, sheet_effect.tint);
    spritesheet& sheet_head = prj.animations[ANIM_IDX_HEAD];
    play_sprite_on_site_pro(sheet_head, sheet_head.coord, sheet_head.origin, sheet_head.rotation, sheet_head.tint);

    spritesheet& sheet_trail = prj.animations[ANIM_IDX_TRAIL];
    play_sprite_on_site_pro(sheet_trail, sheet_trail.coord, sheet_trail.origin, sheet_trail.rotation, sheet_trail.tint);
  }
}
void refresh_ability_harvester(ability& abl) {
  if (abl.id != ABILITY_ID_HARVESTER) {
    IWARN("ability::refresh_ability_harvester()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_HARVESTER, abl.id);
    return;
  }
  if (not abl.is_active or not abl.is_initialized) {
    IWARN("ability::refresh_ability_harvester()::Ability is not initialized or activated");
    return;
  }
  abl.projectiles.clear();
  abl.animation_ids.clear();
  abl.projectiles.reserve(abl.proj_count);
  abl.animation_ids.push_back(SHEET_ID_HARVESTER_EFFECT);
  abl.animation_ids.push_back(SHEET_ID_HARVESTER_HEAD);
  abl.animation_ids.push_back(SHEET_ID_HARVESTER_TRAIL);
  abl.proj_count = 1; // INFO: This ability has only 1 projectile

  projectile& prj = abl.projectiles.emplace_back(projectile());
  prj.collision = Rectangle {0.f, 0.f, abl.proj_dim.x * abl.proj_collision_scale.x, abl.proj_dim.y * abl.proj_collision_scale.y};
  prj.damage = abl.base_damage;
  prj.is_active = true;
  prj.duration = abl.proj_duration;
  prj.PRJ_PHASE = harvester_phase::IDLE;

  {
    spritesheet& sheet = prj.animations.emplace_back(spritesheet()); 
    sheet.sheet_id = abl.animation_ids.at(ANIM_IDX_EFFECT);
    set_sprite(sheet, false, false);

    sheet.coord  = { 0.f, 0.f, HARVESTER_EFFECT_WIDTH, HARVESTER_EFFECT_HEIGHT };
    sheet.origin = { sheet.coord.width * .5f, sheet.coord.height * .5f};
  }

  {
    spritesheet& sheet = prj.animations.emplace_back(spritesheet()); 
    sheet.sheet_id = abl.animation_ids.at(ANIM_IDX_HEAD);
    set_sprite(sheet, true, true);

    sheet.coord  = { 0.f, 0.f, HARVESTER_HEAD_WIDTH, HARVESTER_HEAD_HEIGHT };
    sheet.origin = { sheet.coord.width * .5f, sheet.coord.height * .5f};
  }

  {
    spritesheet& sheet = prj.animations.emplace_back(spritesheet()); 
    sheet.sheet_id = abl.animation_ids.at(ANIM_IDX_TRAIL);
    set_sprite(sheet, true, true);

    sheet.coord  = { 0.f, 0.f, HARVESTER_TRAIL_WIDTH, HARVESTER_TRAIL_HEIGHT };
    sheet.origin = { sheet.coord.width * .5f, sheet.coord.height * .5f};
  }

  
  prj.active_sprite = ANIM_IDX_HEAD;
}

