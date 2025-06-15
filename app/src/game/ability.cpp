#include "ability.h"
#include <reasings.h>

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"
#include "core/ftime.h"

#include "game/spritesheet.h"

#define ABILITIES_BASE_SCALE 2.5f
#define CODEX_BOOK_POSITION_BY_PLAYER(PLAYER_POS, PLAYER_DIM) {\
  Rectangle { PLAYER_POS.x - PLAYER_DIM.x * .2f, PLAYER_POS.y - PLAYER_DIM.y * .2f, 32, 32 }\
}
#define CODEX_BOOK_POSITION_BY_ABILITY(ABILITY_POS) {\
  Rectangle { ABILITY_POS.x, ABILITY_POS.y, 32, 32 }\
}

#define RADIENCE_COLOR_TRANSPARENT  CLITERAL(Color){ 253, 203, 110, 0 }
#define RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW  CLITERAL(Color){ 253, 203, 110, 128 }

typedef struct ability_system_state {
  std::array<ability, ABILITY_TYPE_MAX> abilities;

  camera_metrics* in_camera_metrics;
  app_settings* in_settings;
  ingame_info* in_ingame_info; 
} ability_system_state;
static ability_system_state * state;

void register_ability(ability abl) { state->abilities.at(abl.type) = abl; }
void render_projectile(projectile* prj, f32 scale);

void update_fireball(ability* abl);
void update_bullet(ability* abl);
void update_comet(ability* abl);
void update_codex(ability* abl);
void update_radience(ability* abl);

void render_fireball(ability* abl);
void render_bullet(ability* abl);
void render_comet(ability* abl);
void render_codex(ability* abl);
void render_radience(ability* abl);

bool ability_system_initialize(camera_metrics* _camera_metrics, app_settings* _settings, ingame_info* _ingame_info) {
  if (state) {
    return false;
  }
  state = (ability_system_state*)allocate_memory_linear(sizeof(ability_system_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }
  state->in_settings = _settings;
  state->in_camera_metrics = _camera_metrics;
  state->in_ingame_info = _ingame_info;

  std::array<ability_upgradables, ABILITY_UPG_MAX> fireball_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_SPEED, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Fireball",
    fireball_upgr,
    ABILITY_TYPE_FIREBALL,SHEET_ID_FLAME_ENERGY_ANIMATION, SHEET_ID_SPRITESHEET_UNSPECIFIED,
    1.f, 1.f, 1, 1, 0.f,
    Vector2{30.f, 30.f}, Rectangle{704, 768, 32, 32},
    15,
    false
  ));
  std::array<ability_upgradables, ABILITY_UPG_MAX> bullet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Bullet",
    bullet_upgr,
    ABILITY_TYPE_BULLET,SHEET_ID_FLAME_ENERGY_ANIMATION, SHEET_ID_SPRITESHEET_UNSPECIFIED,
    1.f, 1.f, 1, 3, 1.75f,
    Vector2{30.f, 30.f}, Rectangle{544, 128, 32, 32},
    15,
    false
  ));
  std::array<ability_upgradables, ABILITY_UPG_MAX> comet_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Comet",
    comet_upgr,
    ABILITY_TYPE_COMET,SHEET_ID_FIREBALL_ANIMATION,SHEET_ID_FIREBALL_EXPLOTION_ANIMATION,
    1.2f, 1.2f, 1, 7, 0.f,
    Vector2{30.f, 30.f}, Rectangle{576, 128, 32, 32},
    15,
    false,true
  ));
  std::array<ability_upgradables, ABILITY_UPG_MAX> codex_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Arcane Codex",
    codex_upgr,
    ABILITY_TYPE_CODEX, SHEET_ID_FLAME_ENERGY_ANIMATION, SHEET_ID_SPRITESHEET_UNSPECIFIED,
    0.8f, 0.8f, 1, 3, 0.f,
    Vector2{30.f, 30.f}, Rectangle{832, 0, 32, 32},
    11,
    false,true
  ));
  std::array<ability_upgradables, ABILITY_UPG_MAX> radience_upgr = {ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED};
  register_ability(ability("Radience",
    radience_upgr,
    ABILITY_TYPE_RADIANCE, SHEET_ID_GENERIC_LIGHT, SHEET_ID_SPRITESHEET_UNSPECIFIED,
    2.0f, 1.f, 1, 0, 0.f,
    Vector2{128.f, 128.f}, Rectangle{2080, 1240, 32, 32},
    11,
    false,true
  ));
  return true;
}

void update_abilities(ability_play_system* system) {
  for (size_t iter=0; iter<ABILITY_TYPE_MAX; ++iter) {
    ability* abl = __builtin_addressof(system->abilities.at(iter));
    if (!abl->is_active || !abl->is_initialized) continue;

    switch (abl->type) {
      case ABILITY_TYPE_FIREBALL:  update_fireball(abl); break;
      case ABILITY_TYPE_BULLET:    update_bullet(abl); break;
      case ABILITY_TYPE_COMET:     update_comet(abl); break;
      case ABILITY_TYPE_CODEX:     update_codex(abl); break;
      case ABILITY_TYPE_RADIANCE:  update_radience(abl); break;
      default: {
        TraceLog(LOG_ERROR, "ability::update_abilities()::Unsuppported ability movement type");
        break;
      }
    }
  }
}
void render_abilities(ability_play_system* system) {
  for (size_t iter = 1; iter < ABILITY_TYPE_MAX; ++iter) {
    ability* abl = __builtin_addressof(system->abilities.at(iter));
    if (abl == nullptr || system == nullptr || !abl->is_active || !abl->is_initialized) { continue; }

    switch (abl->type) {
      case ABILITY_TYPE_FIREBALL:  render_fireball(abl); break;
      case ABILITY_TYPE_BULLET:    render_bullet(abl);   break;
      case ABILITY_TYPE_COMET:     render_comet(abl);    break;
      case ABILITY_TYPE_CODEX:     render_codex(abl);    break;
      case ABILITY_TYPE_RADIANCE:  render_radience(abl); break;
      default: {
        TraceLog(LOG_ERROR, "ability::update_abilities()::Unsuppported ability movement type");
        break;
      }
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
    return ability();
  }
  ability abl = state->abilities.at(_type);
  return abl;
}
ability get_next_level(ability abl) {
  upgrade_ability(&abl);
  return abl;
}

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
    prj->collision = Rectangle {0.f, 0.f, abl->proj_dim.x * abl->proj_collision_scale, abl->proj_dim.y * abl->proj_collision_scale};
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

void update_fireball(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_fireball()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr) {
    return;
  }
  player_state* player = reinterpret_cast<player_state*>(abl->p_owner);

  abl->position.x  = player->position.x + player->dimentions_div2.x;
  abl->position.y  = player->position.y + player->dimentions_div2.y;
  abl->rotation += abl->proj_speed;

  if (abl->rotation > 360) abl->rotation = 0;

  for (size_t iter = 0; iter < abl->projectiles.size(); iter++) {
    projectile& prj = abl->projectiles.at(iter);
    if (!prj.is_active) { continue; }

    i16 angle = (i32)(((360.f / abl->projectiles.size()) * iter) + abl->rotation) % 360;

    prj.position = get_a_point_of_a_circle( abl->position, (abl->level * 3.f) + player->collision.height + 15, angle);
    prj.collision.x = prj.position.x - (prj.collision.width * .5f);
    prj.collision.y = prj.position.y - (prj.collision.width * .5f);

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.collision.x),     static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
      static_cast<i16>(prj.damage + player->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(&prj.default_animation);

    if (prj.play_explosion_animation) {
      update_sprite(&prj.explotion_animation);

      if (prj.explotion_animation.current_frame >= prj.explotion_animation.frame_total - 1) {
        prj.play_explosion_animation = false;
        reset_sprite(&prj.explotion_animation, true);
      }
    }
  }
}
void update_bullet(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_bullet()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr) {
    return;
  }
  player_state* player = reinterpret_cast<player_state*>(abl->p_owner);

  abl->position.x  = player->position.x + player->dimentions_div2.x;
  abl->position.y  = player->position.y + player->dimentions_div2.y;

  for (size_t iter = 0; iter < abl->projectiles.size(); iter++) {
    projectile& prj = abl->projectiles.at(iter);
    if (!prj.is_active) { continue; }

    if (prj.duration <= 0) {
      prj.position = player->position;
      prj.duration = abl->proj_duration;
      prj.direction = player->w_direction;
    }
    else {
     prj.duration -= GetFrameTime();
    }
    prj.position.x += prj.direction == WORLD_DIRECTION_RIGHT ? abl->proj_speed : -abl->proj_speed;

    prj.collision.x = prj.position.x - prj.collision.width  * .5f;
    prj.collision.y = prj.position.y - prj.collision.height * .5f;

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
      static_cast<i16>(prj.damage + player->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(&prj.default_animation);

    if (prj.play_explosion_animation) {
      update_sprite(&prj.explotion_animation);
      if (prj.explotion_animation.current_frame >= prj.explotion_animation.frame_total - 1) {
        prj.play_explosion_animation = false;
        reset_sprite(&prj.explotion_animation, true);
      }
    }
  }
}
void update_comet(ability* abl) {
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
    if (vec2_equals(prj.position, pVECTOR2(prj.vec_ex.f32), .1)) {
      prj.vec_ex.f32[2] =  prj.position.x;
      prj.vec_ex.f32[3] =  prj.position.y;
      prj.play_explosion_animation = true;

      const f32 rand = get_random(frustum->x, frustum->x + frustum->width);
      prj.position = VECTOR2(rand, frustum->y - abl->proj_dim.y);
      Vector2 new_pos = {
        (f32)get_random(frustum->x + frustum->width  * .1f, frustum->x + frustum->width  - frustum->width  * .1f),
        (f32)get_random(frustum->y + frustum->height * .2f, frustum->y + frustum->height - frustum->height * .2f)
      };
      prj.vec_ex.f32[0] = new_pos.x;
      prj.vec_ex.f32[1] = new_pos.y;
      prj.default_animation.rotation = get_movement_rotation(prj.position, pVECTOR2(prj.vec_ex.f32)) + 270.f;
    }
    else {
      prj.position = move_towards(prj.position, pVECTOR2(prj.vec_ex.f32), abl->proj_speed);

      prj.collision.x = prj.position.x - prj.collision.width  * .5f;
      prj.collision.y = prj.position.y - prj.collision.height * .5f;
    }

    event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
      static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height), 
      static_cast<i16>(prj.damage + p_player->damage),
      static_cast<i16>(COLLISION_TYPE_RECTANGLE_RECTANGLE)
    ));
    update_sprite(&prj.default_animation);
    if (prj.play_explosion_animation) {
      update_sprite(&prj.explotion_animation);

      if (prj.explotion_animation.current_frame >= prj.explotion_animation.frame_total - 1) {
        prj.play_explosion_animation = false;
        reset_sprite(&prj.explotion_animation, true);
      }
    }
  }
}
void update_codex(ability* abl) {
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
    update_sprite(&prj.default_animation);
    if (prj.play_explosion_animation) {
      update_sprite(&prj.explotion_animation);
      if (prj.explotion_animation.current_frame >= prj.explotion_animation.frame_total - 1) {
        prj.play_explosion_animation = false;
        reset_sprite(&prj.explotion_animation, true);
      }
    }
  }
}
void update_radience(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::update_codex()::Ability is not active or not initialized");
    return;
  }
  if (abl->p_owner == nullptr || state->in_ingame_info == nullptr || state->in_ingame_info->nearest_spawn == nullptr) {
    return;
  }
  player_state* p_player = reinterpret_cast<player_state*>(abl->p_owner);
  abl->position = p_player->position;
  abl->position.x += p_player->dimentions_div2.x;
  abl->position.y += p_player->dimentions_div2.y;

  projectile& prj = abl->projectiles.at(0);
  if (!prj.is_active) { return; }

  prj.position = abl->position;
  prj.collision.x = prj.position.x;
  prj.collision.y = prj.position.y;

  event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, event_context(
    static_cast<i16>(prj.collision.x), static_cast<i16>(prj.collision.y), static_cast<i16>(prj.collision.width), static_cast<i16>(prj.collision.height),
    static_cast<i16>(prj.damage + p_player->damage),
    static_cast<i16>(COLLISION_TYPE_CIRCLE_RECTANGLE)
  ));

  update_sprite(&prj.default_animation);
}

void render_fireball(ability* abl){
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_fireball()::Ability is not active or not initialized");
    return;
  }

  for (size_t iter = 0; iter < abl->projectiles.size(); ++iter) {
    if (!abl->projectiles.at(iter).is_active) { continue; }
    render_projectile(__builtin_addressof(abl->projectiles.at(iter)), abl->proj_sprite_scale);
  }
}
void render_bullet(ability* abl){
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_bullet()::Ability is not active or not initialized");
    return;
  }

  for (size_t iter = 0; iter < abl->projectiles.size(); ++iter) {
    if (!abl->projectiles.at(iter).is_active) { continue; }
    render_projectile(__builtin_addressof(abl->projectiles.at(iter)), abl->proj_sprite_scale);
  }
}
void render_comet(ability* abl){
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_comet()::Ability is not active or not initialized");
    return;
  }

  for (size_t iter = 0; iter < abl->projectiles.size(); ++iter) {
    if (!abl->projectiles.at(iter).is_active) { continue; }
    render_projectile(__builtin_addressof(abl->projectiles.at(iter)), abl->proj_sprite_scale);
  }
}
void render_codex(ability* abl){
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_codex()::Ability is not active or not initialized");
    return;
  }
  for (size_t iter = 0; iter < abl->projectiles.size(); ++iter) {
    if (!abl->projectiles.at(iter).is_active) { continue; }
    render_projectile(__builtin_addressof(abl->projectiles.at(iter)), abl->proj_sprite_scale);
  }
  atlas_texture * icon_atlas = ss_get_atlas_texture_by_enum(ATLAS_TEX_ID_ICON_ATLAS);
  Rectangle codex_book_source = Rectangle {icon_atlas->source.x, icon_atlas->source.y, 32, 32};
  codex_book_source.x += 832.f;
  Rectangle codex_book_dest = CODEX_BOOK_POSITION_BY_ABILITY(abl->position);

  DrawTexturePro(*icon_atlas->atlas_handle, codex_book_source, codex_book_dest, VECTOR2(0, 0), 0.f, WHITE);
}
void render_radience(ability* abl){
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::render_codex()::Ability is not active or not initialized");
    return;
  }
  if (!abl->projectiles.at(0).is_active) { return; }
  projectile * prj = __builtin_addressof(abl->projectiles.at(0));
  render_projectile(prj, abl->proj_sprite_scale);
  
  DrawCircleGradient(prj->collision.x, prj->collision.y, prj->collision.width, RADIENCE_COLOR_INNER_CIRCLE_BRIGHT_YARROW, RADIENCE_COLOR_TRANSPARENT);
}

void render_projectile(projectile* prj, f32 scale) {
  Vector2 dim = Vector2 {
    prj->default_animation.current_frame_rect.width  * scale,
    prj->default_animation.current_frame_rect.height * scale
  };
  prj->default_animation.origin.x = dim.x / 2.f;
  prj->default_animation.origin.y = dim.y / 2.f;
  play_sprite_on_site(__builtin_addressof(prj->default_animation), WHITE, Rectangle { prj->position.x, prj->position.y, dim.x, dim.y });
  if (prj->play_explosion_animation) {
    prj->explotion_animation.origin.x = dim.x / 2.f;
    prj->explotion_animation.origin.y = dim.y / 2.f;
    play_sprite_on_site(__builtin_addressof(prj->explotion_animation), WHITE, Rectangle { prj->vec_ex.f32[2], prj->vec_ex.f32[3], dim.x, dim.y });
  }
}
