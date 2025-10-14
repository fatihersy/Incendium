#include "ability_codex.h"
#include <reasings.h>
#include <loc_types.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/fmath.h"

#include "game/spritesheet.h"

typedef struct ability_codex_state {
  const camera_metrics* in_camera_metrics;
  const app_settings* in_settings;
  const ingame_info* in_ingame_info; 

  ability_codex_state(void) {
    this->in_camera_metrics = nullptr;
    this->in_settings = nullptr;
    this->in_ingame_info = nullptr;
  }
} ability_codex_state;
static ability_codex_state * state = nullptr;

#define CODEX_BOOK_DIM_SOURCE 32.f
#define CODEX_BOOK_DIM_DEST 32.f

#define PROJECTILE_COOLDOWN 0.65f

#define PROJ_F32_ACCUMULATOR_F32 mm_ex.f32[0]
#define PROJ_F32_COOLDOWN_F32 mm_ex.f32[1]

#define PROJ_TARGET_X vec_ex.f32[0]
#define PROJ_TARGET_Y vec_ex.f32[1]

bool ability_codex_initialize(const camera_metrics *const _camera_metrics, const app_settings *const _settings, const ingame_info *const _ingame_info) {
  if (state and state != nullptr) {
    return true;
  }
  state = (ability_codex_state*)allocate_memory_linear(sizeof(ability_codex_state),true);
  if (not state or state == nullptr) {
    IERROR("ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  *state = ability_codex_state();

  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;
  return true;
}

void upgrade_ability_codex(ability *const abl) {
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
ability get_ability_codex() {
  if (not state or state == nullptr) {
    return ability();
  }
  std::array<ability_upgradables, ABILITY_UPG_MAX> codex_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  return ability(static_cast<i32>(LOC_TEXT_PLAYER_ABILITY_NAME_ARCANE_CODEX), ABILITY_ID_CODEX,
    codex_upgr,
    0.f, 0.2f, Vector2 {.8f, .8f}, 1, 3, 0.f, 11,
    Vector2{30.f, 30.f}, Rectangle{1920, 608, 32, 32}
  );
}
ability get_ability_codex_next_level(ability abl) {
  upgrade_ability_codex(__builtin_addressof(abl));
  return abl;
}

void update_ability_codex(ability *const abl) {
  if (not abl or abl == nullptr) {
    IWARN("ability::update_codex()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_CODEX) {
    IWARN("ability::update_codex()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_CODEX, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::update_codex()::Ability is not active or not initialized");
    return;
  }
  if (not abl->p_owner or abl->p_owner == nullptr or not state->in_ingame_info or state->in_ingame_info == nullptr) {
    return;
  }
  const player_state *const p_player = reinterpret_cast<player_state*>(abl->p_owner);

  Rectangle book_position = Rectangle { 
    p_player->collision.x - p_player->collision.width * .2f, 
    p_player->collision.y - p_player->collision.height * .2f, 
    CODEX_BOOK_DIM_DEST, CODEX_BOOK_DIM_DEST 
  };
  abl->position.x = book_position.x;
  abl->position.y = book_position.y;

  for (size_t itr_000 = 0u; itr_000 < static_cast<size_t>(abl->proj_count); itr_000++) {
    projectile& prj = abl->projectiles.at(itr_000);
    spritesheet& sheet = prj.animations.at(prj.active_sprite);
    if (not prj.is_active) { continue; }

    if (not sheet.is_played) {
      update_sprite(__builtin_addressof(sheet));
      continue;
    }
    if (prj.PROJ_F32_ACCUMULATOR_F32 > 0.f) {
      prj.PROJ_F32_ACCUMULATOR_F32 -= GetFrameTime();
      continue;
    }
    if (not state->in_ingame_info->nearest_spawn or state->in_ingame_info->nearest_spawn == nullptr ) {
      continue;
    }
    reset_sprite(__builtin_addressof(sheet), true);
    prj.PROJ_F32_ACCUMULATOR_F32 = prj.PROJ_F32_COOLDOWN_F32;

    prj.PROJ_TARGET_X = state->in_ingame_info->nearest_spawn->collision.x + state->in_ingame_info->nearest_spawn->collision.width  * .5f;
    prj.PROJ_TARGET_Y = state->in_ingame_info->nearest_spawn->collision.y + state->in_ingame_info->nearest_spawn->collision.height * .5f;

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.PROJ_TARGET_X), static_cast<i16>(prj.PROJ_TARGET_Y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
      static_cast<i16>(prj.damage + p_player->stats.at(CHARACTER_STATS_DAMAGE).buffer.i32[3]),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
  }
}

void render_ability_codex(ability *const abl) {
  if (not abl or abl == nullptr) {
    IWARN("ability::render_codex()::Ability is not valid");
    return;
  }
  if (abl->id != ABILITY_ID_CODEX) {
    IWARN("ability::render_codex()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_CODEX, abl->id);
    return;
  }
  if (not abl->is_active or not abl->is_initialized) {
    IWARN("ability::render_codex()::Ability is not active or not initialized");
    return;
  }
  if (not state->in_ingame_info->nearest_spawn or state->in_ingame_info->nearest_spawn == nullptr ) {
    return;
  }
  for (size_t itr_000 = 0u; itr_000 < abl->projectiles.size(); ++itr_000) {
    projectile& prj = abl->projectiles.at(itr_000);
    spritesheet& sheet = prj.animations.at(prj.active_sprite);
    if (not prj.is_active or sheet.is_played) { continue; }
    Vector2 dim = Vector2 {sheet.current_frame_rect.width * abl->proj_sprite_scale, sheet.current_frame_rect.height * abl->proj_sprite_scale};

    const i32 col = sheet.current_frame % sheet.col_total;
    const i32 row = sheet.current_frame / sheet.col_total;

    const Vector2 origin = Vector2 {dim.x * .5f, 0.f};
    const Vector2 target = Vector2 {prj.PROJ_TARGET_X, prj.PROJ_TARGET_Y};
    const f32 rotation = get_movement_rotation(abl->position, target) + 270.f;
    const f32 distance = vec2_distance(abl->position, target);

    const Rectangle source = Rectangle {
      sheet.offset.x + sheet.current_frame_rect.width * col, 
      sheet.offset.y + sheet.current_frame_rect.height * row, 
      sheet.current_frame_rect.width, 
      sheet.current_frame_rect.height
    };
    const Rectangle dest = Rectangle {
      abl->position.x + CODEX_BOOK_DIM_DEST * .5f, 
      abl->position.y + CODEX_BOOK_DIM_DEST * .5f, 
      dim.x, 
      distance
    };

    play_sprite_on_site_ex(__builtin_addressof(sheet), source, dest, origin, rotation, WHITE);
    event_fire(EVENT_CODE_PLAY_SOUND_GROUP, event_context(SOUNDGROUP_ID_ZAP, static_cast<i32>(true)));
  }
  const Texture2D *const icon_tex = ss_get_texture_by_enum(TEX_ID_ASSET_ATLAS);
  Rectangle codex_book_dest = Rectangle { abl->position.x, abl->position.y, CODEX_BOOK_DIM_DEST, CODEX_BOOK_DIM_DEST };

  DrawTexturePro( (*icon_tex), abl->icon_src, codex_book_dest, VECTOR2(0.f, 0.f), 0.f, WHITE);
}

void refresh_ability_codex(ability *const abl) {
  if (not abl or abl == nullptr) {
    IWARN("ability::refresh_ability_codex()::Ability is null");
    return;
  }
  if (abl->proj_count > MAX_ABILITY_PROJECTILE_COUNT) {
    IWARN("ability::refresh_ability_codex()::Ability projectile count exceed");
    return;
  }
  if (abl->id != ABILITY_ID_CODEX) {
    IWARN("ability::refresh_ability_codex()::Ability type is incorrect. Expected: %d, Recieved:%d", ABILITY_ID_CODEX, abl->id);
    return;
  }
  // INFO: This ability has one projectile
  abl->proj_count = 1;

  abl->projectiles.clear();
  abl->animation_ids.clear();
  abl->projectiles.reserve(abl->proj_count);
  abl->animation_ids.push_back(SHEET_ID_ABILITY_CODEX);

  for (size_t itr_000 = 0u; itr_000 < static_cast<size_t>(abl->proj_count); ++itr_000) {
    projectile prj = projectile();
    prj.collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale.x, abl->proj_dim.y * abl->proj_collision_scale.y};
    prj.PROJ_F32_COOLDOWN_F32 = PROJECTILE_COOLDOWN;
    prj.PROJ_F32_ACCUMULATOR_F32 = prj.PROJ_F32_COOLDOWN_F32;
    prj.damage = abl->base_damage;
    prj.is_active = true;
    prj.duration = abl->proj_duration;
    for (size_t itr_111 = 0u; itr_111 < abl->animation_ids.size(); ++itr_111) {
      if (abl->animation_ids.at(itr_111) <= SHEET_ID_SPRITESHEET_UNSPECIFIED or abl->animation_ids.at(itr_111) >= SHEET_ID_SPRITESHEET_TYPE_MAX) {
        IWARN("ability::refresh_ability_codex()::Ability sprite not initialized or corrupted");
        return;
      }
      spritesheet spr = spritesheet();
      spr.sheet_id = abl->animation_ids.at(itr_111);
      set_sprite(__builtin_addressof(spr), false, false);
      spr.origin = VECTOR2( spr.coord.width * .5f,  spr.coord.height * .5f );

      prj.animations.push_back(spr);
      prj.active_sprite = 0;
    }
    abl->projectiles.push_back(prj);
  }
}
