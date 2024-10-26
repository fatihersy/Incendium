
#include "fmath.h"
#include "math.h"
#include "raymath.h"

Vector2 get_a_point_of_a_circle(Vector2 position, f32 radius, f32 angle) {
    return (Vector2)
    {
        position.x + (radius * cos(angle * 3.1415f / 180.f)),
        position.y + (radius * sin(angle * 3.1415f / 180.f))
    };
}

Vector2 move_towards(Vector2 position, Vector2 target, f32 speed) 
{
    return Vector2MoveTowards(position, target, speed);
}