#include "ability.h"
#include "defines.h"

#include "core/fmath.h"
#include "game/game_manager.h"

void update_dirty_ability_system(ability_system_state *system);

ability_system_state ability_system_initialize(actor_type _owner_type,
                                               Vector2 _dimentions) {
  ability_system_state _system;
  _system.ability_amount = -1;
  _system.owner_type = _owner_type;
  _system.player_width = _dimentions.x;
  _system.player_height = _dimentions.y;
  _system.fire_ball_ball_count = 4;
  _system.fire_ball_ball_radius = 12;
  _system.fire_ball_ball_diameter = 12 * 2;
  _system.fire_ball_circle_radius = 80;
  _system.fire_ball_circle_radius_div_2 = 40;
  _system.fire_ball_circle_radius_div_4 = 20;
  _system.radiation_circle_radius = 100;
  _system.radiation_circle_diameter = 200;
  _system.radiation_circle_radius_div_2 = 50;
  _system.radiation_circle_radius_div_4 = 25;
  _system.direct_fire_square_width = 35;
  _system.direct_fire_square_height = 100;
  _system.direct_fire_square_height_div_2 = 50;
  _system.salvo_projectile_at_a_time = 2;
  _system.salvo_fire_count = 3;
  _system.salvo_projectile_count = 6;
  _system.salvo_fire_rate = 1;
  _system.abilities[0].rotation = 55;

  return _system;
}

void add_ability(ability_system_state *system, ability_type type) {
  system->ability_amount++;
  system->abilities[system->ability_amount].type = type;
  system->abilities[system->ability_amount].rotation = 0;

  ability *_ability = &system->abilities[system->ability_amount];

  switch (type) {
  case FIREBALL: {
    _ability->position = system->owner_position;

    for (u32 i = 1; i <= system->fire_ball_ball_count; i++) {
      _ability->projectiles[i - 1].position = get_a_point_of_a_circle(
          _ability->position, system->fire_ball_circle_radius,
          (((360 / system->fire_ball_ball_count) * i) + _ability->rotation) %
              360);

      _ability->projectiles[i - 1].collision.x = _ability->projectiles[i - 1].position.x;
      _ability->projectiles[i - 1].collision.y = _ability->projectiles[i - 1].position.y;
      _ability->projectiles[i - 1].collision.width = system->fire_ball_ball_diameter;
      _ability->projectiles[i - 1].collision.height = system->fire_ball_ball_diameter;

      _ability->projectiles[i - 1].damage = 22;

      _ability->projectiles[i - 1].initialized = true;
    }
    break;
  }

  case SALVO: {
    _ability->overall_process = 0.f;

    _ability->position = system->owner_position;
    _ability->fire_rate = system->salvo_fire_rate;

    for (u32 i = 0; i <= system->salvo_projectile_count; i++) {
      _ability->projectiles[i].initialized = false;
      _ability->projectiles[i].position = _ability->position;
      _ability->projectiles[i].speed = 1.0f;
      _ability->projectiles[i].collision.x =
          _ability->projectiles[i].position.x;
      _ability->projectiles[i].collision.y =
          _ability->projectiles[i].position.y;
    }
    break;
  }
  case RADIATION: {
    _ability->position = system->owner_position;

    _ability->projectiles[0].initialized = true;
    _ability->projectiles[0].position = _ability->position;

    _ability->projectiles[0].collision.x = _ability->projectiles[0].position.x;
    _ability->projectiles[0].collision.y = _ability->projectiles[0].position.y;
    _ability->projectiles[0].collision.width =
        system->radiation_circle_diameter;
    _ability->projectiles[0].collision.height =
        system->radiation_circle_diameter;

    break;
  }
  case DIRECT_FIRE: {
    _ability->projectiles[0].initialized = true;
    _ability->position.x = system->owner_position.x;
    _ability->position.y =
        system->owner_position.y - system->direct_fire_square_height / 2.f;

    _ability->projectiles[0].collision.x = _ability->position.x;
    _ability->projectiles[0].collision.y = _ability->position.y;
    _ability->projectiles[0].collision.width = system->direct_fire_square_width;
    _ability->projectiles[0].collision.height =
        system->direct_fire_square_height;
    break;
  }

  default:
    break;
  }
}

bool update_abilities(ability_system_state *system, Vector2 position) {
  if (system->is_dirty_ability_system) {
    update_dirty_ability_system(system);
  }

  system->owner_position.x = position.x;
  system->owner_position.y = position.y;

  for (i16 i = 0; i < system->ability_amount + 1; i++) {
    ability *_ability = &system->abilities[i];

    switch (_ability->type) {
    case FIREBALL: {
      _ability->rotation += 1;

      if (_ability->rotation > 359) _ability->rotation = 1;

      for (i16 j = 1; j <= system->fire_ball_ball_count; j++) {
        _ability->projectiles[j - 1].position = 
          get_a_point_of_a_circle(
            system->owner_position, 
            system->fire_ball_circle_radius, 
            (((360 / system->fire_ball_ball_count) * j) + _ability->rotation) % 360
          );
        _ability->projectiles[j - 1].collision.x = _ability->projectiles[j - 1].position.x - system->fire_ball_ball_radius;
        _ability->projectiles[j - 1].collision.y = _ability->projectiles[j - 1].position.y - system->fire_ball_ball_radius;

        damage_any_spawn(&_ability->projectiles[j - 1]);
      }

      break;
    }

    case SALVO: {
      _ability->is_on_fire = false;

      for (u32 j = 0; j < system->salvo_projectile_count; j++) {
        if (_ability->projectiles[j].initialized) {
          _ability->is_on_fire = true;

          _ability->projectiles[j].position = GetSplinePointBezierQuad(
              _ability->position, (Vector2){_ability->position.x, 0},
              _ability->projectile_target_position[j],
              _ability->projectile_process[j]);
          _ability->projectiles[j].collision.x =
              _ability->projectiles[j].position.x;
          _ability->projectiles[j].collision.y =
              _ability->projectiles[j].position.y;

          _ability->projectile_process[j] += _ability->projectiles[j].speed;

          if (_ability->projectile_process[j] > .3f &&
              _ability->projectile_process[j + 1] == 0) {
            _ability->projectiles[j + 1].initialized = true;
          }

          // if (damage_any_collade(&_ability->projectiles[j]))
          // _ability->projectiles[j].initialized = false;

          if (_ability->overall_process > 0.3f ||
              vec2_equals(_ability->projectiles[j].position,
                          _ability->projectile_target_position[j], 5)) {
            _ability->projectiles[j].initialized = false;
            _ability->projectile_process[j] = 0.f;
          }
        }
      }

      if (_ability->is_on_fire)
        break;
      _ability->position = system->owner_position;
      _ability->projectiles[0].initialized = true;

      for (u32 j = 0; j < system->salvo_projectile_count; j++) {
        _ability->projectiles[j].rotation =
            GetRandomValue(0, 360); // TODO: SOME WIERD SHIT HAPPENING HERE
        _ability->projectiles[j].speed = GetRandomValue(0, 100) / 100.f + .5;

        _ability->projectile_target_position[j] =
            get_a_point_of_a_circle(_ability->position, GetRandomValue(50, 300),
                                    _ability->projectiles[j].rotation);

        _ability->projectiles[j].collision.x =
            _ability->projectiles[j].position.x;
        _ability->projectiles[j].collision.y =
            _ability->projectiles[j].position.y;
      }

      _ability->is_on_fire = true;

      break;
    }

    case RADIATION: {
      _ability->position = system->owner_position;
      _ability->projectiles[0].position = _ability->position;

      _ability->projectiles[0].collision.x =
          _ability->projectiles[0].position.x - system->radiation_circle_radius;
      _ability->projectiles[0].collision.y =
          _ability->projectiles[0].position.y - system->radiation_circle_radius;

      break;
    }

    case DIRECT_FIRE: {
      if (!_ability->is_on_fire) {
        _ability->position.x = system->owner_position.x;
        _ability->position.y =
            system->owner_position.y - system->direct_fire_square_height_div_2;
        _ability->projectiles[0].position = _ability->position;

        _ability->projectiles[0].collision.x = _ability->position.x;
        _ability->projectiles[0].collision.y = _ability->position.y;

        _ability->is_on_fire = true;
      } else {
        _ability->projectiles[0].position = move_towards(
            _ability->projectiles[0].position,
            (Vector2){_ability->position.x + 200, _ability->position.y},
            100 * GetFrameTime());

        _ability->projectiles[0].collision.x =
            _ability->projectiles[0].position.x;
        _ability->projectiles[0].collision.y =
            _ability->projectiles[0].position.y;

        if (vec2_equals(
                _ability->projectiles[0].position,
                (Vector2){_ability->position.x + 200, _ability->position.y},
                5)) {
          _ability->is_on_fire = false;
          // TraceLog(LOG_INFO, "_ability->is_on_fire = false;");
        }
      }

      break;
    }
    default:
      break;
    }
  }

  return true;
}

bool render_abilities(ability_system_state *system) {
  for (i16 i = 0; i < system->ability_amount + 1; i++) {
    switch (system->abilities[i].type) {
    case FIREBALL: {
      for (i16 j = 0; j < system->fire_ball_ball_count; j++) {
        DrawCircleV(system->abilities[i].projectiles[j].position,
                    system->fire_ball_ball_radius, RED);

#if DEBUG_COLLISIONS
        DrawRectangleLines(system->abilities[i].projectiles[j].collision.x,
                           system->abilities[i].projectiles[j].collision.y,
                           system->abilities[i].projectiles[j].collision.width,
                           system->abilities[i].projectiles[j].collision.height,
                           WHITE);
#endif
      };
      break;
    }

    case SALVO: {
      for (u32 j = 0; j < system->salvo_projectile_count; j++) {
        if (system->abilities[i].projectiles[j].initialized) {
          DrawCircleV(system->abilities[i].projectiles[j].position, 12, BLUE);
        }
      }

      break;
    }

    case RADIATION: {
      if (system->abilities[i].projectiles[0].initialized) {
        DrawCircleV(system->abilities[i].projectiles[0].position,
                    system->radiation_circle_radius,
                    (Color){32, 191, 107, 100}); // rgba(32, 191, 107,1.0)

#if DEBUG_COLLISIONS
        DrawRectangleLines(system->abilities[i].projectiles[0].collision.x,
                           system->abilities[i].projectiles[0].collision.y,
                           system->abilities[i].projectiles[0].collision.width,
                           system->abilities[i].projectiles[0].collision.height,
                           WHITE);
#endif
      }
      break;
    }
    case DIRECT_FIRE: {
      if (system->abilities[i].projectiles[0].initialized) {
        DrawRectangle(system->abilities[i].projectiles[0].position.x,
                      system->abilities[i].projectiles[0].position.y,
                      system->direct_fire_square_width,
                      system->direct_fire_square_height,
                      (Color){32, 191, 107, 100}); // rgba(32, 191, 107,1.0)
#if DEBUG_COLLISIONS
        DrawRectangleLines(system->abilities[i].projectiles[0].collision.x,
                           system->abilities[i].projectiles[0].collision.y,
                           system->abilities[i].projectiles[0].collision.width,
                           system->abilities[i].projectiles[0].collision.height,
                           WHITE);
#endif
      }
      break;
    }
    default:
      break;
    }
  }
  return true;
}

void update_dirty_ability_system(ability_system_state *system) {
  if (!system->is_dirty_ability_system) {
    return;
  }

  for (int i = 0; i <= system->ability_amount; i++) {
    switch (system->abilities[i].type) {
    case FIREBALL: {
      system->fire_ball_ball_count = 4 + system->abilities[i].level;
      break;
    }

    default:
      break;
    };
  }

  system->is_dirty_ability_system = false;
}
