
#include "fmath.h"
#include "math.h"
#include "raymath.h"

inline Vector2 get_a_point_of_a_circle(Vector2 position, i16 radius, i16 angle) {
  return (Vector2){
    position.x + (radius * cos(angle * 3.1415f / 180.f)),
    position.y + (radius * sin(angle * 3.1415f / 180.f))
  };
}

inline Vector2 move_towards(Vector2 position, Vector2 target, f32 speed) {
    
  return Vector2MoveTowards(position, target, (float)speed);
}

inline bool vec2_equals(Vector2 v1, Vector2 v2, f32 tolerans) {

  f32 tolerans1 = fabsf(v1.x - v2.x);
  f32 tolerans2 = fabsf(v1.y - v2.y);
  return 
  (
    tolerans1 <= tolerans && 
    tolerans1 >= 0 && 
    tolerans2 <= tolerans &&
    tolerans2 >= 0);
}

inline Vector2 vec2_subtract(Vector2 v1, Vector2 v2) {
  return Vector2Subtract(v1, v2);
}

inline Vector2 vec2_add(Vector2 v1, Vector2 v2) {
  return Vector2Add(v1, v2);
}
inline Vector2 vec2_scale(Vector2 v1, float f1) {
  return Vector2Scale(v1, f1);
}

inline float vec2_lenght(Vector2 v1) { 
  return Vector2Length(v1); 
}

inline Vector2 vec2_zero() { return (Vector2) {0}; }
