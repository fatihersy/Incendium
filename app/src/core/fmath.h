
#ifndef MATH_H
#define MATH_H

#include "defines.h"

Vector2 get_a_point_of_a_circle(Vector2 position, f32 radius, f32 angle);

Vector2 move_towards(Vector2 position, Vector2 target, f32 speed);

#endif