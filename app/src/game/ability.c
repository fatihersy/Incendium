#include "ability.h"

#include "core/event.h"
#include "core/fmath.h"
#include "core/fmemory.h"


typedef struct ability_system_state {
  spritesheet_play_system spritesheet_system;
  ability abilities[ABILITY_TYPE_MAX];

} ability_system_state;

static ability_system_state* state;

#define PSPRITESHEET_SYSTEM state->spritesheet_system
#include "game/spritesheet.h"

void register_ability(ability_type type, u16 proj_count, u16 _damage, Vector2 proj_size, spritesheet_type proj_anim, bool _is_static, bool _should_center);

bool ability_system_initialize() {
  if (state) {
    return false;
  }

  state = (ability_system_state*)allocate_memory_linear(sizeof(ability_system_state),true);
  if (!state) {
    TraceLog(LOG_ERROR, "ability::ability_system_initialize()::Failed to allocate memory");
    return false;
  }

  register_ability(ABILITY_TYPE_FIREBALL, 4, 15, (Vector2) {30, 30}, FIREBALL_ANIMATION, true, true);
  return true;
}


void upgrade_ability(ability_type _type, ability_play_system* system) {}
void update_abilities(ability_play_system* system, Character2D owner) {

  ability* _ability = &system->abilities[ABILITY_TYPE_FIREBALL];

  _ability->position.x  = owner.collision.x + owner.collision.width / 2.f;
  _ability->position.y  = owner.collision.y + owner.collision.height / 2.f;

  _ability->rotation += 1;
    
  if (_ability->rotation > 359) _ability->rotation = 0;

  for (i16 i = 0; i < _ability->projectile_count; i++) {

    i16 angle = (i32)(((360.f / _ability->projectile_count) * i) + _ability->rotation) % 360;

    _ability->projectiles[i].position = get_a_point_of_a_circle(_ability->position, 90, angle);

    _ability->projectiles[i].collision.x = _ability->projectiles[i].position.x;
    _ability->projectiles[i].collision.y = _ability->projectiles[i].position.y;
        
    event_fire(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, (event_context) {
      .data.u16[0] = _ability->projectiles[i].id,
      .data.u16[1] = _ability->projectiles[i].collision.x,
      .data.u16[2] = _ability->projectiles[i].collision.y
    });
  }

  update_sprite_renderqueue();
}

void render_abilities(ability_play_system* system) {
  for (int i = 1; i < ABILITY_TYPE_MAX; ++i) {
    if (!system->abilities[i].is_active) continue;

    for (int j = 0; j < system->abilities[i].projectile_count; ++j) {
      if (!system->abilities[i].projectiles[j].is_active) continue;
      play_sprite_on_site(
        system->abilities[i].projectiles[j].animation_sprite_queueindex, 
        system->abilities[i].projectiles[j].collision,
        WHITE);
    }
  }

}

void register_ability(ability_type type, u16 proj_count, u16 _damage, Vector2 proj_size, spritesheet_type proj_anim, bool _is_static, bool _should_center) {
  ability abl = (ability){0};

  abl.type = type;
  abl.projectile_count = proj_count;
  abl.is_static = _is_static;

  for (int i = 0; i < proj_count; ++i) {
    abl.projectiles[i].projectile_anim_sprite = proj_anim;
    abl.projectiles[i].animation_sprite_queueindex = register_sprite(proj_anim, _is_static, !_is_static, _should_center);
    abl.projectiles[i].duration = abl.is_static ? 0 : 60;
    abl.projectiles[i].damage = _damage;
    abl.projectiles[i].collision = (Rectangle) {0, 0, proj_size.x, proj_size.y};
    abl.projectiles[i].is_active = true;
  }

  state->abilities[type] = abl;
}
ability get_ability(ability_type _type) {
  if (!state) {
    return (ability){0};
  }

  return state->abilities[_type];
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

// LABEL: ability package
typedef struct ability_package {
  ability_type type;

  union {
    ability_fireball fireball;
    ability_radiation radiation;
  } data;
} ability_package;

// LABEL: Add ability result
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


