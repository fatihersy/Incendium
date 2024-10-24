#include "ability.h"
#include "core/fmath.h"

#define MAX_ABILITY_AMOUNT 50

#define FIRE_BALL_BALL_RADIUS 12
#define FIRE_BALL_CIRCLE_RADIUS 50
#define FIRE_BALL_CIRCLE_RADIUS_DIV_2 FIRE_BALL_CIRCLE_RADIUS/2

ability_system_state* ability_system_initialize() {
    ability_system_state* ability_state = (ability_system_state*)malloc(sizeof(ability_system_state));

    ability_state->abilities = (ability*)malloc(MAX_ABILITY_AMOUNT * sizeof(ability));
    ability_state->amount = -1;

    return ability_state;
}

void add_ability(ability_system_state* ability_system, ability_type type) {
    ability_system->abilities[ability_system->amount++].type = type;
    ability_system->abilities[ability_system->amount++].rotation = 0;
}

bool update_abilities(ability_system_state* system, Vector2 position) {
    return true;
}

bool render_abilities(ability_system_state* system, Vector2 position) {
    for (size_t i = 0; i < system->amount + 1; i++) {
        switch (system->abilities[i].type) {
            case fireball: {

                system->abilities[i].rotation += 1;

                i16 ball1 = (90 + system->abilities[i].rotation) % 360;
                i16 ball2 = (180 + system->abilities[i].rotation) % 360;
                i16 ball3 = (270 + system->abilities[i].rotation) % 360;
                i16 ball4 = system->abilities[i].rotation;

                if(system->abilities[i].rotation > 359) system->abilities[i].rotation = 1;

                DrawCircleV(
                    (Vector2){
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball1).x,
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball1).y,
                    },
                    FIRE_BALL_BALL_RADIUS,
                    RED);

                DrawCircleV(
                    (Vector2){
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball2).x,
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball2).y,
                    },
                    FIRE_BALL_BALL_RADIUS,
                    RED);
                DrawCircleV(
                    (Vector2){
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball3).x,
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball3).y,
                    },
                    FIRE_BALL_BALL_RADIUS,
                    RED);
                DrawCircleV(
                    (Vector2){
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball4).x,
                        get_a_point_of_a_circle(position, FIRE_BALL_CIRCLE_RADIUS, ball4).y,
                    },
                    FIRE_BALL_BALL_RADIUS,
                    RED);
            } break;

            default:
                break;
        }
    }

    return true;
}
