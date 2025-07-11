#include "ability_codex.h"
#include <reasings.h>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "game/spritesheet.h"

#define CODEX_BOOK_POSITION_BY_PLAYER(PLAYER_POS, PLAYER_DIM) {\
  Rectangle { PLAYER_POS.x - PLAYER_DIM.x * .2f, PLAYER_POS.y - PLAYER_DIM.y * .2f, 32, 32 }\
}
#define CODEX_BOOK_POSITION_BY_ABILITY(ABILITY_POS) {\
  Rectangle { ABILITY_POS.x, ABILITY_POS.y, 32, 32 }\
}

typedef struct ability_codex_state {
  std::array<ability, ABILITY_TYPE_MAX> abilities;

  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 
} ability_codex_state;
static ability_codex_state * state;

bool ability_codex_initialize(const camera_metrics* _camera_metrics,const app_settings* _settings,const ingame_info* _ingame_info) {
  if (state) {
    TraceLog(LOG_WARNING, "ability::ability_system_initialize()::Init called multiple times");
    return false;
  }
  state = (ability_codex_state*)allocate_memory_linear(sizeof(ability_codex_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_codex(ability* abl) {
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
ability get_ability_codex() {
  if (!state) {
    return ability();
  }

  std::array<ability_upgradables, ABILITY_UPG_MAX> codex_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability("Arcane Codex", ABILITY_TYPE_CODEX,
    codex_upgr,
    0.f, 0.8f, Vector2 {.8f, .8f}, 1, 3, 0.f, 11,
    Vector2{30.f, 30.f}, Rectangle{2560, 992, 32, 32}
  );
}
ability get_ability_codex_next_level(ability abl) {
  upgrade_ability_codex(&abl);
  return abl;
}

void update_ability_codex(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::update_codex()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_CODEX) {
    TraceLog(LOG_WARNING, "ability::update_codex()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_CODEX, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_codex()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr || state->in_ingame_info == nullptr || state->in_ingame_info->nearest_spawn == nullptr) {
    return;
  }
  u16 frame_counter_max = 1 * TARGET_FPS;
  player_state* p_player = reinterpret_cast<player_state*>(abl->p_owner);
  Rectangle book_position = CODEX_BOOK_POSITION_BY_PLAYER(p_player->position, p_player->dimentions);
  abl->position.x = book_position.x;
  abl->position.y = book_position.y;

  for (size_t iter = 0; iter < 1; iter++) {
    projectile& prj = abl->projectiles.at(iter);
    if (!prj.is_active) { continue; }
    prj.is_active = true;

    u16& frame_counter = prj.mm_ex.u16[0];
    if ((frame_counter >= frame_counter_max || frame_counter < 0) && !state->in_ingame_info->nearest_spawn->is_dead ) {
      prj.position = abl->position;
      
      prj.vec_ex.f32[0]  = state->in_ingame_info->nearest_spawn->collision.x + state->in_ingame_info->nearest_spawn->collision.width  * .5f;
      prj.vec_ex.f32[1]  = state->in_ingame_info->nearest_spawn->collision.y + state->in_ingame_info->nearest_spawn->collision.height * .5f;
      prj.vec_ex.f32[2]  = prj.position.x;
      prj.vec_ex.f32[3]  = prj.position.y;

      prj.mm_ex.u16[0] = 0;
      prj.mm_ex.u16[1] = state->in_ingame_info->nearest_spawn->character_id;
    }
    else {
      Vector2 distance = vec2_subtract(pVECTOR2(prj.vec_ex.f32), Vector2 {prj.vec_ex.f32[2], prj.vec_ex.f32[3]});
      prj.position.x = EaseQuadIn(frame_counter, prj.vec_ex.f32[2],  distance.x, frame_counter_max);
      prj.position.y = EaseQuadIn(frame_counter, prj.vec_ex.f32[3],  distance.y, frame_counter_max);

      prj.collision.x = prj.position.x;
      prj.collision.y = prj.position.y;

      if (frame_counter < frame_counter_max) frame_counter++;
      frame_counter = FCLAMP(frame_counter, 0, frame_counter_max);
    }
    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
      static_cast<i16>(prj.damage + p_player->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(&prj.animations.at(0));
  }
}

void render_ability_codex(ability* abl) {
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::render_codex()::Ability is not valid");
    return;
  }
  if (abl->type != ABILITY_TYPE_CODEX) {
    TraceLog(LOG_WARNING, "ability::render_codex()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_CODEX, abl->type);
    return;
  }
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_codex()::Ability is not active or not initialized");
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
  const Texture2D * icon_tex = ss_get_texture_by_enum(TEX_ID_ASSET_ATLAS);
  Rectangle codex_book_dest = CODEX_BOOK_POSITION_BY_ABILITY(abl->position);

  DrawTexturePro(*icon_tex, abl->icon_src, codex_book_dest, VECTOR2(0, 0), 0.f, WHITE);
}

void refresh_ability_codex(ability* abl) { 
  if (abl == nullptr) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_codex()::Ability is null");
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_codex()::Ability projectile count exceed");
    return;
  }
  if (abl->type != ABILITY_TYPE_CODEX) {
    TraceLog(LOG_WARNING, "ability::refresh_ability_codex()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_TYPE_CODEX, abl->type);
    return;
  }

  abl->projectiles.clear();
  abl->animation_ids.clear();
  abl->projectiles.reserve(abl->proj_count);
  abl->animation_ids.push_back(SHEET_ID_FLAME_ENERGY_ANIMATION);

  for (size_t itr_000 = 0; itr_000 < abl->proj_count; ++itr_000) {
    projectile prj = projectile();
    prj.collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    for (size_t itr_111 = 0; itr_111 < abl->animation_ids.size(); ++itr_111) {
      if (abl->animation_ids.at(itr_111) <= SHEET_ID_SPRITESHEET_UNSPECIFIED || abl->animation_ids.at(itr_111) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        TraceLog(LOG_WARNING, "ability::refresh_ability_codex()::Ability sprite not initialized or corrupted");
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

