#include "ability_fireball.h"

#include "core/event.h"
#include "core/fmath.h"

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

void upgrade_fireball(ability_fireball* ability) {
  // LABEL: Upgrade fireball
}

void update_fireball(ability_fireball* _ability, Character2D owner) {
  _ability->rotation += 1;

  if (_ability->rotation > 359) _ability->rotation = 1;

  for (i16 i = 0; i < _ability->fire_ball_ball_count; i++) {
    _ability->projectiles[i].position = 
    get_a_point_of_a_circle(
      owner.position, 
      _ability->fire_ball_circle_radius, 
      (i32)(((360.f / _ability->fire_ball_ball_count) * (i+1)) + _ability->rotation) % 360
    );
    _ability->projectiles[i].collision.x = _ability->projectiles[i].position.x - _ability->fire_ball_ball_radius;
    _ability->projectiles[i].collision.y = _ability->projectiles[i].position.y - _ability->fire_ball_ball_radius;
    event_fire(EVENT_CODE_RELOCATE_PROJECTILE_COLLISION, 0, (event_context) {
      .data.u16[0] = _ability->projectiles[i].id,
      .data.u16[1] = _ability->projectiles[i].collision.x,
      .data.u16[2] = _ability->projectiles[i].collision.y
    });
  }
}

void render_fireball(ability_fireball* _ability) {
  for (i16 i = 0; i < _ability->fire_ball_ball_count; i++) {
    DrawCircleV(_ability->projectiles[i].position, _ability->fire_ball_ball_radius, RED);
    #if DEBUG_COLLISIONS
      DrawRectangleLines(_ability->projectiles[i].collision.x,
        _ability->projectiles[i].collision.y,
        _ability->projectiles[i].collision.width,
        _ability->projectiles[i].collision.height,
        WHITE);
    #endif
  }
}