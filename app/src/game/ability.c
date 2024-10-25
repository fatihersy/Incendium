#include "ability.h"
#include "core/fmath.h"

#include "collision.h"

#define MAX_ABILITY_AMOUNT 10

#define FIRE_BALL_BALL_RADIUS 12
#define FIRE_BALL_BALL_DIAMETER FIRE_BALL_BALL_RADIUS * 2
#define FIRE_BALL_BALL_COUNT 4
#define FIRE_BALL_CIRCLE_RADIUS 50
#define FIRE_BALL_CIRCLE_RADIUS_DIV_2 FIRE_BALL_CIRCLE_RADIUS / 2
#define FIRE_BALL_CIRCLE_RADIUS_DIV_4 FIRE_BALL_CIRCLE_RADIUS / 4

ability_system_state* ability_system_initialize(actor_type _owner_type) {
    ability_system_state* ability_state = (ability_system_state*)malloc(sizeof(ability_system_state));

    ability_state->abilities = (ability*)malloc(MAX_ABILITY_AMOUNT * sizeof(ability));
    ability_state->ability_amount = -1;
    ability_state->owner_type = _owner_type;

    return ability_state;
}

void add_ability(ability_system_state* system, ability_type type) {
    system->ability_amount++;
    system->abilities[system->ability_amount].type = type;
    system->abilities[system->ability_amount].rotation = 0;

    switch (type) {
        case fireball: {
            system->abilities->projectiles = (Character2D*)malloc(FIRE_BALL_BALL_COUNT * sizeof(Character2D));

            system->abilities->projectiles->initialized = true;

            for (size_t i = 1; i <= FIRE_BALL_BALL_COUNT; i++) {
                system->abilities->projectiles[i - 1].position = 
                    get_a_point_of_a_circle
                    (
                        system->owner_position, 
                        FIRE_BALL_CIRCLE_RADIUS, 
                        ((90*i) + system->abilities[i].rotation) % 360
                    );

                system->abilities->projectiles[i - 1].collision_rect.x = system->abilities->projectiles[i - 1].position.x;
                system->abilities->projectiles[i - 1].collision_rect.y = system->abilities->projectiles[i - 1].position.y;
            }

            break;
        }

        default:
            break;
    }
}

bool update_abilities(ability_system_state* system, Vector2 position) {
    system->owner_position = position;

    for (size_t i = 0; i < system->ability_amount + 1; i++) {
        switch (system->abilities[i].type) {
            case fireball: {
                system->abilities[i].rotation += 1;

                if (system->abilities[i].rotation > 359) system->abilities[i].rotation = 1;

                for (size_t j = 1; j <= FIRE_BALL_BALL_COUNT; j++) {
                    system->abilities->projectiles[j - 1].position = 
                        get_a_point_of_a_circle
                        (
                            system->owner_position, 
                            FIRE_BALL_CIRCLE_RADIUS, 
                            ((90*j) + system->abilities[i].rotation) % 360
                        );
                    system->abilities->projectiles[j - 1].collision_rect.x = system->abilities->projectiles[j - 1].position.x;
                    system->abilities->projectiles[j - 1].collision_rect.y = system->abilities->projectiles[j - 1].position.y;
                }

                break;
            }

            default:
                break;
        }
    }

    return true;
}

bool render_abilities(ability_system_state* system) 
{
    for (size_t i = 0; i < system->ability_amount + 1; i++) 
    {
        switch (system->abilities[i].type) 
        {
            case fireball: {
                for (size_t j = 0; j < FIRE_BALL_BALL_COUNT; j++) 
                {
                    DrawCircleV(
                        system->abilities[i].projectiles[j].position,
                        FIRE_BALL_BALL_RADIUS,
                        RED);
                    
                    DrawRectangleLines
                    (
                        system->abilities[i].projectiles[j].position.x - FIRE_BALL_CIRCLE_RADIUS_DIV_4,
                        system->abilities[i].projectiles[j].position.y - FIRE_BALL_CIRCLE_RADIUS_DIV_4,
                        FIRE_BALL_BALL_DIAMETER,
                        FIRE_BALL_BALL_DIAMETER,
                        WHITE
                    );
                };
                break;
            }
            default:break;
        }
    }

    return true;
}
