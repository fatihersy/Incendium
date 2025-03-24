#include "ability.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

#include "game/spritesheet.h"


typedef struct ability_system_state {
  ability abilities[ABILITY_TYPE_MAX];

  camera_metrics* in_camera_metrics;
  app_settings* settings;
} ability_system_state;

static ability_system_state* state;

void movement_satellite(ability* abl);
void movement_bullet(ability* abl);
void movement_comet(ability* abl);

void register_ability(
  char _display_name[MAX_ABILITY_NAME_LENGTH], 
  Rectangle icon_loc, 
  ability_upgradables _upgradables[ABILITY_UPG_MAX],
  ability_type type,
  movement_pattern move_pattern, 
  u16 proj_count_on_start,
  f32 proj_duration, 
  u16 _damage, 
  Vector2 proj_size, 
  spritesheet_id proj_anim, 
  bool _should_center
);

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

  register_ability("Fireball", (Rectangle){704, 768, 32, 32}, 
    (ability_upgradables[ABILITY_UPG_MAX]){ABILITY_UPG_DAMAGE, ABILITY_UPG_SPEED, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED},
    ABILITY_TYPE_FIREBALL, MOVE_TYPE_SATELLITE, 
    1, 0, 15, (Vector2) {30, 30}, SHEET_ID_FIREBALL_ANIMATION, true
  );
  register_ability("Bullet", (Rectangle){544, 128, 32, 32}, 
    (ability_upgradables[ABILITY_UPG_MAX]){ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED},
    ABILITY_TYPE_BULLET, MOVE_TYPE_BULLET, 
    1, 1.75, 15, (Vector2) {30, 30}, SHEET_ID_FIREBALL_ANIMATION, true
  );
  register_ability("Comet",  (Rectangle) {576, 128, 32, 32}, 
    (ability_upgradables[ABILITY_UPG_MAX]){ABILITY_UPG_DAMAGE, ABILITY_UPG_HITBOX, ABILITY_UPG_AMOUNT, ABILITY_UPG_UNDEFINED, ABILITY_UPG_UNDEFINED},
    ABILITY_TYPE_COMET, MOVE_TYPE_COMET, 
    2, 0, 15, (Vector2) {30, 30}, SHEET_ID_FIREBALL_ANIMATION, true
  );
  return true;
}

void upgrade_ability(ability* abl) {
  if (abl->type <= ABILITY_TYPE_UNDEFINED || abl->type >= ABILITY_TYPE_MAX) {
    TraceLog(LOG_WARNING, "ability::upgrade_ability()::Ability is not initialized");
    return;
  }

  ++abl->level;

  for (int i=0; i<ABILITY_UPG_MAX; ++i) {
    switch (abl->upgradables[i]) {
      // TODO: Complete all cases
      case ABILITY_UPG_DAMAGE: { 
        abl->base_damage += abl->level * 2;
        break;
      }
      case ABILITY_UPG_AMOUNT: { 
        u16 new_proj_count = abl->proj_count + 1;
        for (int i = abl->proj_count; i < new_proj_count; ++i) {
          abl->projectiles[i].collision = (Rectangle) {0, 0, abl->proj_dim.x, abl->proj_dim.y};
          abl->projectiles[i].damage = abl->base_damage + abl->level * 2;
          abl->projectiles[i].is_active = true;
        }
        abl->proj_count = new_proj_count;
        break;
      }
      case ABILITY_UPG_HITBOX: { 
        
        break;
      }
      case ABILITY_UPG_SPEED:  { 
        
        break;
      }
      
      default: return;
    }
  }


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

    for (i32 j = 0; j<MAX_ABILITY_PROJECTILE_SLOT; ++j) {
      if (!abl->projectiles[j].is_active) { continue; }

      event_fire(EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE, (event_context) {
        .data.u16[0] = abl->projectiles[j].collision.x, .data.u16[1] = abl->projectiles[j].collision.y,
        .data.u16[2] = abl->projectiles[j].collision.width, .data.u16[3] = abl->projectiles[j].collision.height,
        .data.u16[4] = abl->projectiles[j].damage,
      });
    }

    // TODO: Handle all situations
  }
  for (i32 i=0; i<MAX_ABILITY_SLOT; ++i) {
    if (!state->abilities[i].is_active) { continue; }

    for (i32 j=0; j<MAX_ABILITY_PROJECTILE_SLOT; ++j) {
      if (!state->abilities[i].projectiles[j].is_active) { continue; }

      update_sprite(&state->abilities[i].projectiles[j].animation);
    }
  }
}
void movement_satellite(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::movement_satellite()::Ability is not active or not initialized");
    return;
  }
  if (abl->move_pattern >= MOVE_TYPE_MAX || abl->move_pattern <= MOVE_TYPE_UNDEFINED) {
    TraceLog(LOG_WARNING, "ability::movement_satelite()::Recieved pattern out of bound");
    return ;
  }
  player_state* player = (player_state*)abl->p_owner; // HACK: Hardcoded character state

  abl->position.x  = player->collision.x + player->collision.width / 2.f;
  abl->position.y  = player->collision.y + player->collision.height / 2.f;

  abl->rotation += 1;
    
  if (abl->rotation > 359) abl->rotation = 0;

  for (i16 i = 0; i < abl->proj_count; i++) {

    i16 angle = (i32)(((360.f / abl->proj_count) * i) + abl->rotation) % 360;

    abl->projectiles[i].position = get_a_point_of_a_circle(
      abl->position, 
      (abl->level * 3.f) + player->collision.height + 15,  // NOTE: Better radius calculation?
      angle);

    abl->projectiles[i].collision.x = abl->projectiles[i].position.x;
    abl->projectiles[i].collision.y = abl->projectiles[i].position.y;
  }
}
void movement_bullet(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::movement_bullet()::Ability is not active or not initialized");
    return;
  }
  if (abl->move_pattern >= MOVE_TYPE_MAX || abl->move_pattern <= MOVE_TYPE_UNDEFINED) {
    TraceLog(LOG_WARNING, "ability::movement_bullet()::Recieved pattern out of bound");
    return ;
  }
  player_state* player = (player_state*)abl->p_owner; // HACK: Hardcoded character state

  abl->position.x  = player->collision.x + player->collision.width / 2.f;
  abl->position.y  = player->collision.y + player->collision.height / 2.f;

  for (i16 i = 0; i < abl->proj_count; i++) {

    if (abl->projectiles[i].duration <= 0) {
      abl->projectiles[i].position = player->position;
      abl->projectiles[i].duration = abl->proj_duration;
      abl->projectiles[i].direction = player->w_direction;
    }
    else {
      abl->projectiles[i].duration -= GetFrameTime();
    }
    
    abl->projectiles[i].position.x += abl->projectiles[i].direction == WORLD_DIRECTION_RIGHT ? 2.75 : -2.75;

    abl->projectiles[i].collision.x = abl->projectiles[i].position.x;
    abl->projectiles[i].collision.y = abl->projectiles[i].position.y;
    
  }
}
void movement_comet(ability* abl) {
  if (!abl->is_active || !abl->is_initialized) {
    TraceLog(LOG_WARNING, "ability::movement_comet()::Ability is not active or not initialized");
    return;
  }
  if (abl->move_pattern >= MOVE_TYPE_MAX || abl->move_pattern <= MOVE_TYPE_UNDEFINED) {
    TraceLog(LOG_WARNING, "ability::movement_comet()::Recieved pattern out of bound");
    return ;
  }
  player_state* player = (player_state*)abl->p_owner; // HACK: Hardcoded character state

  const f32 rand_percent[MAX_ABILITY_PROJECTILE_SLOT] = { 0.72, 0.45, 0.89,  0.16,  0.33, 0.58};
  const f32 rand_recoil[MAX_ABILITY_PROJECTILE_SLOT]  = {-0.58, 0.33, 0.16, -0.45, -0.89, 0.72};

  abl->position.x  = player->collision.x + player->collision.width / 2.f;
  abl->position.y  = player->collision.y + player->collision.height / 2.f;

  for (i16 i = 0; i < abl->proj_count; i++) {

    if (vec2_equals(abl->projectiles[i].position, pVECTOR2(abl->projectiles[i].buffer.f32), .1)) {
      f32* rand_count = &abl->projectiles[i].buffer.f32[2];
      const f32 rand = rand_percent[(i32)*rand_count] * BASE_RENDER_RES.x;
      const Vector2 screen_min_world = GetScreenToWorld2D((Vector2) {0}, state->in_camera_metrics->handle);
      const Vector2 screen_max_world = GetScreenToWorld2D((Vector2) {BASE_RENDER_RES.x, BASE_RENDER_RES.y}, state->in_camera_metrics->handle);
      abl->projectiles[i].position = GetScreenToWorld2D((Vector2) {rand, -abl->proj_dim.y},state->in_camera_metrics->handle);
      const Vector2 new_prj_pos = (Vector2) {
        .x = FCLAMP(abl->projectiles[i].position.x + (BASE_RENDER_DIV4.x * rand_recoil[(i32)*rand_count]), 
                      screen_min_world.x, screen_max_world.x),
        .y = player->position.y
      };
      abl->projectiles[i].buffer.f32[0] = new_prj_pos.x;
      abl->projectiles[i].buffer.f32[1] = new_prj_pos.y;
      abl->projectiles[i].buffer.f32[2] = (*rand_count) + 1 >= MAX_RAND ? 0 : ++(*rand_count);
    }
    else {
      abl->projectiles[i].duration -= GetFrameTime();
    }
    
    abl->projectiles[i].position = move_towards(abl->projectiles[i].position, pVECTOR2(abl->projectiles[i].buffer.f32), 3);

    abl->projectiles[i].collision.x = abl->projectiles[i].position.x;
    abl->projectiles[i].collision.y = abl->projectiles[i].position.y;
  }
}

void render_abilities(ability_play_system* system) {
  for (int i = 1; i < ABILITY_TYPE_MAX; ++i) {
    if (!system->abilities[i].is_active || !system->abilities[i].is_initialized) { continue; }

    for (int j = 0; j < system->abilities[i].proj_count; ++j) {
      if (!system->abilities[i].projectiles[j].is_active) { continue; }
      play_sprite_on_site(
        &system->abilities[i].projectiles[j].animation, 
        WHITE,
        system->abilities[i].projectiles[j].collision
      );
    }
  }
}

/**
 * @param proj_duration in secs. affects not to all abilities like fireball ability
 * @param _should_center for projectile spritesheet
 */
void register_ability(
  char _display_name[MAX_ABILITY_NAME_LENGTH], Rectangle icon_loc, ability_upgradables _upgradables[ABILITY_UPG_MAX], 
  ability_type type, movement_pattern move_pattern, u16 proj_count_on_start, f32 proj_duration, u16 _damage, Vector2 proj_size, 
  spritesheet_id proj_anim, bool _should_center
) {
  ability abl = {0};
  if (TextLength(_display_name) >= MAX_ABILITY_NAME_LENGTH) {
    TraceLog(LOG_WARNING, "ability::register_ability()::Ability:'%s's name length is out of bound!", _display_name);
    return;
  }
  abl.type = type;
  abl.anim_id = proj_anim;
  abl.level = 1;
  abl.base_damage = _damage;
  abl.proj_count = proj_count_on_start;
  abl.proj_duration = proj_duration;
  abl.move_pattern = move_pattern;
  abl.center_proj_anim = _should_center;
  abl.proj_dim = proj_size;
  abl.icon_src = icon_loc;
  copy_memory(abl.display_name, _display_name, TextLength(_display_name));
  copy_memory(abl.upgradables, _upgradables, sizeof(abl.upgradables));

  for (int i = 0; i < MAX_ABILITY_PROJECTILE_SLOT; ++i) {
    abl.projectiles[i] = (projectile){0};
    abl.projectiles[i].collision = (Rectangle) {0, 0, abl.proj_dim.x, abl.proj_dim.y};
    abl.projectiles[i].damage = abl.base_damage;
    abl.projectiles[i].is_active = false;
    abl.projectiles[i].animation.sheet_id = abl.anim_id;
    set_sprite(&abl.projectiles[i].animation, true, true, abl.center_proj_anim);
  }

  state->abilities[type] = abl;
}
ability get_ability(ability_type _type) {
  if (!state) {
    return (ability){0};
  }
  ability abl = state->abilities[_type];
  return abl;
}

ability get_next_level(ability abl) {
  upgrade_ability(&abl);
  return abl;
}
