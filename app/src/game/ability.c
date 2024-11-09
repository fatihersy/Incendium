#include "ability.h"

#include "core/fmath.h"

#include "game_manager.h"

#define FIRE_BALL_BALL_COUNT 4
#define FIRE_BALL_BALL_RADIUS 12
#define FIRE_BALL_BALL_DIAMETER FIRE_BALL_BALL_RADIUS * 2
#define FIRE_BALL_CIRCLE_RADIUS 50
#define FIRE_BALL_CIRCLE_RADIUS_DIV_2 FIRE_BALL_CIRCLE_RADIUS / 2
#define FIRE_BALL_CIRCLE_RADIUS_DIV_4 FIRE_BALL_CIRCLE_RADIUS / 4

#define RADIATION_CIRCLE_RADIUS 100
#define RADIATION_CIRCLE_DIAMETER RADIATION_CIRCLE_RADIUS * 2
#define RADIATION_CIRCLE_RADIUS_DIV_2 RADIATION_CIRCLE_RADIUS / 2
#define RADIATION_CIRCLE_RADIUS_DIV_4 RADIATION_CIRCLE_RADIUS / 4

#define DIRECT_FIRE_SQUARE_WIDTH 35
#define DIRECT_FIRE_SQUARE_HEIGHT 100
#define DIRECT_FIRE_SQUARE_HEIGHT_DIV_2 DIRECT_FIRE_SQUARE_HEIGHT / 2.f

#define SALVO_PROJECTILE_AT_A_TIME 2
#define SALVO_FIRE_COUNT 3
#define SALVO_PROJECTILE_COUNT SALVO_PROJECTILE_AT_A_TIME* SALVO_FIRE_COUNT
#define SALVO_FIRE_RATE 1  // in sec

ability_system_state ability_system_initialize(actor_type _owner_type) {

    ability_system_state _system;

    _system.ability_amount = -1;
    _system.owner_type = _owner_type;

    _system.abilities[0].rotation = 55;

    return _system;
}

void add_ability(ability_system_state* system, ability_type type) {
    system->ability_amount++;
    system->abilities[system->ability_amount].type = type;
    system->abilities[system->ability_amount].rotation = 0;

    ability* _ability = &system->abilities[system->ability_amount];

    switch (type) {
        case fireball: {
            _ability->position = system->owner_position;

            for (u32 i = 1; i <= FIRE_BALL_BALL_COUNT; i++) {
                _ability->projectiles[i - 1].initialized = true;
                _ability->projectiles[i - 1].position =
                    get_a_point_of_a_circle(
                        _ability->position,
                        FIRE_BALL_CIRCLE_RADIUS,
                        (((360 / FIRE_BALL_BALL_COUNT) * i) + _ability->rotation) % 360);

                _ability->projectiles[i - 1].collision_rect.x = _ability->projectiles[i - 1].position.x;
                _ability->projectiles[i - 1].collision_rect.y = _ability->projectiles[i - 1].position.y;
                _ability->projectiles[i - 1].collision_rect.width = FIRE_BALL_BALL_DIAMETER;
                _ability->projectiles[i - 1].collision_rect.height = FIRE_BALL_BALL_DIAMETER;
            }
            break;
        }

        case salvo: {
            _ability->overall_process = 0.f;

            _ability->position = system->owner_position;
            _ability->fire_rate = SALVO_FIRE_RATE;

            for (u32 i = 0; i <= SALVO_PROJECTILE_COUNT; i++) {
                _ability->projectiles[i].initialized = false;
                _ability->projectiles[i].position = _ability->position;
                _ability->projectiles[i].speed = 1.0f;
                _ability->projectiles[i].collision_rect.x = _ability->projectiles[i].position.x;
                _ability->projectiles[i].collision_rect.y = _ability->projectiles[i].position.y;
            }
            break;
        }
        case radiation: {
            _ability->position = system->owner_position;

            _ability->projectiles[0].initialized = true;
            _ability->projectiles[0].position = _ability->position;

            _ability->projectiles[0].collision_rect.x = _ability->projectiles[0].position.x;
            _ability->projectiles[0].collision_rect.y = _ability->projectiles[0].position.y;
            _ability->projectiles[0].collision_rect.width = RADIATION_CIRCLE_DIAMETER;
            _ability->projectiles[0].collision_rect.height = RADIATION_CIRCLE_DIAMETER;

            break;
        }
        case direct_fire: {
            _ability->projectiles[0].initialized = true;
            _ability->position.x = system->owner_position.x;
            _ability->position.y = system->owner_position.y - DIRECT_FIRE_SQUARE_HEIGHT / 2.f;

            _ability->projectiles[0].collision_rect.x = _ability->position.x;
            _ability->projectiles[0].collision_rect.y = _ability->position.y;
            _ability->projectiles[0].collision_rect.width = DIRECT_FIRE_SQUARE_WIDTH;
            _ability->projectiles[0].collision_rect.height = DIRECT_FIRE_SQUARE_HEIGHT;
            break;
        }

        default:
            break;
    }
}

bool update_abilities(ability_system_state* system, Vector2 position) {
    system->owner_position = position;

    for (i16 i = 0; i < system->ability_amount + 1; i++) {
        ability* _ability = &system->abilities[i];

        switch (_ability->type) {
            case fireball: {
                _ability->rotation += 1;

                if (_ability->rotation > 359) _ability->rotation = 1;

                for (i16 j = 1; j <= FIRE_BALL_BALL_COUNT; j++) {
                    _ability->projectiles[j - 1].position =
                        get_a_point_of_a_circle(
                            system->owner_position,
                            FIRE_BALL_CIRCLE_RADIUS,
                            (((360 / FIRE_BALL_BALL_COUNT) * j) + _ability->rotation) % 360);
                    _ability->projectiles[j - 1].collision_rect.x = _ability->projectiles[j - 1].position.x - FIRE_BALL_BALL_RADIUS;
                    _ability->projectiles[j - 1].collision_rect.y = _ability->projectiles[j - 1].position.y - FIRE_BALL_BALL_RADIUS;

                    damage_any_collade(&_ability->projectiles[j - 1]);
                }

                break;
            }

            case salvo: {
                _ability->is_on_fire = false;

                for (u32 j = 0; j < SALVO_PROJECTILE_COUNT; j++) {
                    if (_ability->projectiles[j].initialized) {
                        _ability->is_on_fire = true;

                        _ability->projectiles[j].position =
                            GetSplinePointBezierQuad(
                                _ability->position,
                                (Vector2){_ability->position.x, 0},
                                _ability->projectile_target_position[j],
                                _ability->projectile_process[j]);
                        _ability->projectiles[j].collision_rect.x = _ability->projectiles[j].position.x;
                        _ability->projectiles[j].collision_rect.y = _ability->projectiles[j].position.y;

                        _ability->projectile_process[j] += _ability->projectiles[j].speed;

                        if (_ability->projectile_process[j] > .3f && _ability->projectile_process[j + 1] == 0) {
                            _ability->projectiles[j + 1].initialized = true;
                        }

                        // if (damage_any_collade(&_ability->projectiles[j])) _ability->projectiles[j].initialized = false;

                        if (_ability->overall_process > 0.3f || Vec2Equals(_ability->projectiles[j].position, _ability->projectile_target_position[j], 5)) {
                            _ability->projectiles[j].initialized = false;
                            _ability->projectile_process[j] = 0.f;
                        }
                    }
                }

                if (_ability->is_on_fire) break;
                _ability->position = system->owner_position;
                _ability->projectiles[0].initialized = true;

                for (u32 j = 0; j < SALVO_PROJECTILE_COUNT; j++) {
                    _ability->projectiles[j].rotation = GetRandomValue(0, 360); // TODO: SOME WIERD SHIT HAPPENING HERE
                    _ability->projectiles[j].speed = GetRandomValue(0, 100) / 100.f + .5;

                    _ability->projectile_target_position[j] =
                        get_a_point_of_a_circle(
                            _ability->position,
                            GetRandomValue(50, 300),
                            _ability->projectiles[j].rotation);

                    _ability->projectiles[j].collision_rect.x = _ability->projectiles[j].position.x;
                    _ability->projectiles[j].collision_rect.y = _ability->projectiles[j].position.y;
                }

                _ability->is_on_fire = true;

                break;
            }

            case radiation: {
                _ability->position = system->owner_position;
                _ability->projectiles[0].position = _ability->position;

                _ability->projectiles[0].collision_rect.x = _ability->projectiles[0].position.x - RADIATION_CIRCLE_RADIUS;
                _ability->projectiles[0].collision_rect.y = _ability->projectiles[0].position.y - RADIATION_CIRCLE_RADIUS;

                break;
            }

            case direct_fire: {
                if (!_ability->is_on_fire) {
                    _ability->position.x = system->owner_position.x;
                    _ability->position.y = system->owner_position.y - DIRECT_FIRE_SQUARE_HEIGHT_DIV_2;
                    _ability->projectiles[0].position = _ability->position;

                    _ability->projectiles[0].collision_rect.x = _ability->position.x;
                    _ability->projectiles[0].collision_rect.y = _ability->position.y;

                    _ability->is_on_fire = true;
                } else {
                    _ability->projectiles[0].position = move_towards(
                        _ability->projectiles[0].position,
                        (Vector2){
                            _ability->position.x + 200,
                            _ability->position.y},
                        100 * GetFrameTime());

                    _ability->projectiles[0].collision_rect.x = _ability->projectiles[0].position.x;
                    _ability->projectiles[0].collision_rect.y = _ability->projectiles[0].position.y;

                    if (Vec2Equals(_ability->projectiles[0].position, (Vector2){_ability->position.x + 200, _ability->position.y}, 5)) {
                        _ability->is_on_fire = false;
                        TraceLog(LOG_INFO, "_ability->is_on_fire = false;");
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

bool render_abilities(ability_system_state* system) {
    for (i16 i = 0; i < system->ability_amount + 1; i++) {
        switch (system->abilities[i].type) {
            case fireball: {
                for (i16 j = 0; j < FIRE_BALL_BALL_COUNT; j++) {
                    DrawCircleV(
                        system->abilities[i].projectiles[j].position,
                        FIRE_BALL_BALL_RADIUS,
                        RED);

#if DEBUG_COLLISIONS
                    DrawRectangleLines(
                        system->abilities[i].projectiles[j].collision_rect.x,
                        system->abilities[i].projectiles[j].collision_rect.y,
                        system->abilities[i].projectiles[j].collision_rect.width,
                        system->abilities[i].projectiles[j].collision_rect.height,
                        WHITE);
#endif
                };
                break;
            }

            case salvo: {
                for (u32 j = 0; j < SALVO_PROJECTILE_COUNT; j++) {
                    if (system->abilities[i].projectiles[j].initialized) {
                        DrawCircleV(
                            system->abilities[i].projectiles[j].position,
                            12,
                            BLUE);
                    }
                }

                break;
            }

            case radiation: {
                if (system->abilities[i].projectiles[0].initialized) {
                    DrawCircleV(
                        system->abilities[i].projectiles[0].position,
                        RADIATION_CIRCLE_RADIUS,
                        (Color){32, 191, 107, 100});  // rgba(32, 191, 107,1.0)

#if DEBUG_COLLISIONS
                    DrawRectangleLines(
                        system->abilities[i].projectiles[0].collision_rect.x,
                        system->abilities[i].projectiles[0].collision_rect.y,
                        system->abilities[i].projectiles[0].collision_rect.width,
                        system->abilities[i].projectiles[0].collision_rect.height,
                        WHITE);
#endif
                }
                break;
            }
            case direct_fire: {
                if (system->abilities[i].projectiles[0].initialized) {
                    DrawRectangle(
                        system->abilities[i].projectiles[0].position.x,
                        system->abilities[i].projectiles[0].position.y,
                        DIRECT_FIRE_SQUARE_WIDTH,
                        DIRECT_FIRE_SQUARE_HEIGHT,
                        (Color){32, 191, 107, 100});  // rgba(32, 191, 107,1.0)
#if DEBUG_COLLISIONS
                    DrawRectangleLines(
                        system->abilities[i].projectiles[0].collision_rect.x,
                        system->abilities[i].projectiles[0].collision_rect.y,
                        system->abilities[i].projectiles[0].collision_rect.width,
                        system->abilities[i].projectiles[0].collision_rect.height,
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
