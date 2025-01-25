#include "ability.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"

typedef struct ability_system_state {
  spritesheet_play_system spritesheet_system;
  ability abilities[ABILITY_TYPE_MAX];
  f32 randoms[MAX_ABILITY_PROJECTILE_SLOT];

  camera_metrics* camera_metrics;
  app_settings* settings;
} ability_system_state;

static ability_system_state* state;

#define PSPRITESHEET_SYSTEM state->spritesheet_system
#include "game/spritesheet.h"

void movement_satellite(ability* abl);
void movement_bullet(ability* abl);
void movement_comet(ability* abl);

Vector2 p_to_vec2(f32*);

void register_ability(ability_type type, movement_pattern move_pattern, f32 proj_duration, u16 _damage, Vector2 proj_size, spritesheet_type proj_anim, bool _should_center);

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
  state->camera_metrics = _camera_metrics;

  f32 cache_randoms[MAX_ABILITY_PROJECTILE_SLOT] = {
    120, -352, -848, 25, 449, -593
  };
  for (int i=0; i<MAX_ABILITY_PROJECTILE_SLOT; ++i) {
    state->randoms[i] = cache_randoms[i];
  }

  register_ability(ABILITY_TYPE_FIREBALL, MOVE_TYPE_SATELLITE, 0, 15, (Vector2) {30, 30}, FIREBALL_ANIMATION, true);
  register_ability(ABILITY_TYPE_BULLET, MOVE_TYPE_BULLET, 1.75, 15, (Vector2) {30, 30}, FIREBALL_ANIMATION, true);
  register_ability(ABILITY_TYPE_COMET, MOVE_TYPE_COMET, 0, 15, (Vector2) {30, 30}, FIREBALL_ANIMATION, true);
  return true;
}

void upgrade_ability(ability* abl, ability_play_system* system) {
  ++abl->level;
  abl->proj_count = abl->level;

  for (int i = 0; i < abl->proj_count; ++i) {
    abl->projectiles[i].animation_sprite_queueindex = register_sprite(abl->proj_anim_sprite, true, false, abl->center_proj_anim);
    abl->projectiles[i].damage = abl->base_damage + abl->level * 2;
    abl->projectiles[i].collision = (Rectangle) {0, 0, abl->proj_dim.x, abl->proj_dim.y};
    abl->projectiles[i].is_active = true;
  }
}
void update_abilities(ability_play_system* system, Character2D owner) {

  for (int i=0; i<ABILITY_TYPE_MAX; ++i) {
    ability* abl = &system->abilities[i];
    if (!abl->is_active || !abl->is_initialized) continue;

    switch (abl->move_pattern) {
      case MOVE_TYPE_SATELLITE: movement_satellite(abl); continue;
      case MOVE_TYPE_BULLET: movement_bullet(abl); continue;
      case MOVE_TYPE_COMET: movement_comet(abl); continue;
      default: continue;
    } 

    // TODO: Handle all situations
  }

  update_sprite_renderqueue();
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
    
    event_fire(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, (event_context) {
      .data.u16[0] = abl->projectiles[i].id,
      .data.u16[1] = abl->projectiles[i].collision.x,
      .data.u16[2] = abl->projectiles[i].collision.y
    });
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
    
    event_fire(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, (event_context) {
      .data.u16[0] = abl->projectiles[i].id,
      .data.u16[1] = abl->projectiles[i].collision.x,
      .data.u16[2] = abl->projectiles[i].collision.y
    });
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

  abl->position.x  = player->collision.x + player->collision.width / 2.f;
  abl->position.y  = player->collision.y + player->collision.height / 2.f;

  for (i16 i = 0; i < abl->proj_count; i++) {

    if (vec2_equals(abl->projectiles[i].position, p_to_vec2(abl->projectiles[i].buffer.f32), .1)) {
      abl->projectiles[i].buffer.f32[0] = player->position.x + state->randoms[(i32)abl->projectiles[i].buffer.f32[2]];
      abl->projectiles[i].buffer.f32[1] =  + state->randoms[(i32)abl->projectiles[i].buffer.f32[2]];
      //abl->projectiles[i].position.x = GetScreenToWorld2D((Vector2) {}, );
      abl->projectiles[i].position.y = player->position.y + state->randoms[(i32)abl->projectiles[i].buffer.f32[2]];
    }
    else {
      abl->projectiles[i].duration -= GetFrameTime();
    }
    
    abl->projectiles[i].position = move_towards(abl->projectiles[i].position, p_to_vec2(abl->projectiles[i].buffer.f32), 3);

    abl->projectiles[i].collision.x = abl->projectiles[i].position.x;
    abl->projectiles[i].collision.y = abl->projectiles[i].position.y;
    
    event_fire(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, (event_context) {
      .data.u16[0] = abl->projectiles[i].id,
      .data.u16[1] = abl->projectiles[i].collision.x,
      .data.u16[2] = abl->projectiles[i].collision.y
    });
  }
}

void render_abilities(ability_play_system* system) {
  for (int i = 1; i < ABILITY_TYPE_MAX; ++i) {
    if (!system->abilities[i].is_active || !system->abilities[i].is_initialized) continue;

    for (int j = 0; j < system->abilities[i].proj_count; ++j) {
      if (!system->abilities[i].projectiles[j].is_active) continue;
      play_sprite_on_site(
        system->abilities[i].projectiles[j].animation_sprite_queueindex, 
        system->abilities[i].projectiles[j].collision,
        WHITE);
    }
  }
}

/**
 * @param proj_duration in secs. affects not to all abilities like fireball ability
 * @param _should_center for projectile spritesheet
 */
void register_ability(ability_type type, movement_pattern move_pattern, f32 proj_duration, u16 _damage, Vector2 proj_size, spritesheet_type proj_anim, bool _should_center) {
  ability abl = {0};

  abl.type = type;
  abl.level = 0;
  abl.base_damage = _damage;
  abl.proj_count = 0;
  abl.proj_duration = proj_duration;
  abl.proj_anim_sprite = proj_anim;
  abl.move_pattern = move_pattern;
  abl.center_proj_anim = _should_center;
  abl.proj_dim = proj_size;

  state->abilities[type] = abl;
}
ability get_ability(ability_type _type) {
  if (!state) {
    return (ability){0};
  }

  return state->abilities[_type];
}

Vector2 p_to_vec2(f32* vec1) {
  return (Vector2) {vec1[0], vec1[1]};
}

#undef PSPRITESHEET_SYSTEM

/* 

typedef struct ability_fireball {
  Vector2 position;
  f32 rotation;
  u8 level;
  bool is_active;
  projectile projectiles[MAX_FIREBALL_PROJECTILE_COUNT];

  u16 fireball_ball_count;
  u16 fireball_ball_radius;
  u16 fireball_ball_diameter; 
  u16 fireball_circle_radius;
  u16 fireball_circle_radius_div_2;
  u16 fireball_circle_radius_div_4;
} ability_fireball;

typedef struct ability_radiation {
  Vector2 position;
  u8 level;
  bool is_active;
  Character2D damage_area;
} ability_radiation;


typedef struct ability_package {
  ability_type type;

  union {
    ability_fireball fireball;
    ability_radiation radiation;
  } data;
} ability_package;


typedef struct add_ability_result {
  u16 projectile_count;
  projectile projectiles[MAX_ABILITY_COLLISION_SLOT];

  bool is_success;
}add_ability_result;





----------------------------------------------------------------------------------------------------





void upgrade_fireball(ability_fireball* ability) {
  // LABEL: Upgrade fireball
}

void update_fireball(ability_fireball* _ability, Character2D owner) {
  _ability->rotation += 1;

  if (_ability->rotation > 359) _ability->rotation = 1;

  for (i16 i = 0; i < _ability->fireball_ball_count; i++) {
    _ability->projectiles[i].position = 
    get_a_point_of_a_circle(
      owner.position, 
      _ability->fireball_circle_radius, 
      (i32)(((360.f / _ability->fireball_ball_count) * (i+1)) + _ability->rotation) % 360
    );
    _ability->projectiles[i].collision.x = _ability->projectiles[i].position.x - _ability->fireball_ball_radius;
    _ability->projectiles[i].collision.y = _ability->projectiles[i].position.y - _ability->fireball_ball_radius;
    event_fire(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, (event_context) {
      .data.u16[0] = _ability->projectiles[i].id,
      .data.u16[1] = _ability->projectiles[i].collision.x,
      .data.u16[2] = _ability->projectiles[i].collision.y
    });
  }
}

void render_fireball(ability_fireball* _ability) {
  for (i16 i = 0; i < _ability->fireball_ball_count; i++) {
    DrawCircleV(_ability->projectiles[i].position, _ability->fireball_ball_radius, RED);
    #if DEBUG_COLLISIONS
      DrawRectangleLines(_ability->projectiles[i].collision.x,
        _ability->projectiles[i].collision.y,
        _ability->projectiles[i].collision.width,
        _ability->projectiles[i].collision.height,
        WHITE);
    #endif
  }
}


#define fireball_level_one_ball_count 4
#define fireball_level_one_ball_radius 15
#define fireball_level_one_ball_diameter fireball_level_one_ball_radius * 2
#define fireball_level_one_circle_radius 90
#define fireball_level_one_circle_radius_div_2 fireball_level_one_circle_radius / 2.f
#define fireball_level_one_circle_radius_div_4 fireball_level_one_circle_radius / 4.f

ability_fireball get_ability_fireball() {
  ability_fireball fireball = (ability_fireball){
    .is_active = false,
    .level = 1,
    .rotation = 0,
    .fire_ball_ball_count = fireball_level_one_ball_count,
    .fire_ball_ball_radius = fireball_level_one_ball_radius,
    .fire_ball_ball_diameter = fireball_level_one_ball_diameter,
    .fire_ball_circle_radius = fireball_level_one_circle_radius,
    .fire_ball_circle_radius_div_2 = fireball_level_one_circle_radius_div_2,
    .fire_ball_circle_radius_div_4 = fireball_level_one_circle_radius_div_4,
  };
  for (int i = 0; i<fireball_level_one_ball_count; ++i) {
    fireball.projectiles[i] = (projectile) {
      .id = 0,
      .collision = (Rectangle) { 
        .x = 0, .y = 0, 
        .width = fireball_level_one_ball_diameter,
        .height = fireball_level_one_ball_diameter
      },
      .damage = 15,
    };
  }

  return fireball;
}
 */


