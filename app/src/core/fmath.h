
#ifndef MATH_H
#define MATH_H

#include "defines.h"

Vector2 get_a_point_of_a_circle(Vector2 position, i16 radius, i16 angle);

Vector2 move_towards(Vector2 position, Vector2 target, f32 speed);

Vector2 vec2_zero(void);
bool vec2_equals(Vector2 v1, Vector2 v2, f32 tolerans);
Vector2 vec2_subtract(Vector2 v1, Vector2 v2);
Vector2 vec2_add(Vector2 v1, Vector2 v2);
Vector2 vec2_scale(Vector2 v1, float f1);
float vec2_lenght(Vector2 v1);

#endif
