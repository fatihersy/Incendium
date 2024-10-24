
#include "fmath.h"
#include "math.h"



Vector2 get_a_point_of_a_circle(Vector2 position, f32 radius, f32 angle) {
    return (Vector2)
    {
        position.x + (radius * cos(angle * 3.1415f / 180.f)),
        position.y + (radius * sin(angle * 3.1415f / 180.f))
    };
}
