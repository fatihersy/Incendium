#include "fmath.h"
#include <cmath>

#include "raymath.h"

Vector2 get_a_point_of_a_circle(Vector2 position, i16 radius, i16 angle) {
  return {
    position.x + (radius * cos(angle * 3.1415f / 180.f)),
    position.y + (radius * sin(angle * 3.1415f / 180.f))
  };
}
Vector2 move_towards(Vector2 position, Vector2 target, f32 speed) {
  return Vector2MoveTowards(position, target, (float)speed);
}
bool vec2_equals(Vector2 v1, Vector2 v2, f32 tolerans) {
  f32 tolerans1 = fabsf(v1.x - v2.x);
  f32 tolerans2 = fabsf(v1.y - v2.y);
  return 
  (
    tolerans1 <= tolerans && 
    tolerans1 >= 0 && 
    tolerans2 <= tolerans &&
    tolerans2 >= 0);
}
 Vector2 vec2_subtract(Vector2 v1, Vector2 v2) {
  return Vector2Subtract(v1, v2);
}
 Vector2 vec2_add(Vector2 v1, Vector2 v2) {
  return Vector2Add(v1, v2);
}
 Vector2 vec2_scale(Vector2 v1, float f1) {
  return Vector2Scale(v1, f1);
}
Vector2 vec2_normalize(Vector2 v1) {
  return Vector2Normalize(v1);
}
float vec2_distance(Vector2 v1, Vector2 v2) {
  return Vector2Distance(v1, v2);
}
 float vec2_lenght(Vector2 v1) { 
  return Vector2Length(v1); 
}
/**
 * @brief still, you need to adjust sprite rotation
 */
float get_movement_rotation(Vector2 from, Vector2 to) {
  Vector2 direction = vec2_subtract(to, from);
  return atan2f(direction.y, direction.x) * RAD2DEG;
}
Vector2 vec2_zero(void) { return Vector2 { 0.f, 0.f }; }

double fast_sin(double x) {
int k;
double y;
double z;

z = x;
z *= 0.3183098861837907;
z += 6755399441055744.0;
k = *((int *) &z);
z = k;
z *= 3.1415926535897932;
x -= z;
y = x;
y *= x;
z = 0.0073524681968701;
z *= y;
z -= 0.1652891139701474;
z *= y;
z += 0.9996919862959676;
x *= z;
k &= 1;
k += k;
z = k;
z *= x;
x -= z;

return x;
}

f32 math_fmod(f32 x, f64 y) {
  if (std::isnan(x) or std::isinf(x) or std::isnan(y) or std::isinf(y)) {
    return F32_MAX;
  }
  f32 result = fmod(x, y);
  if (std::isnan(result) or std::isinf(result)) {
    return F32_MAX;
  }
  return result;
}
f32 math_floor(f32 x) {
  if (std::isnan(x) or std::isinf(x)) {
    return F32_MAX;
  }
  f32 result = std::floor(x);
  if (std::isnan(result) or std::isinf(result)) {
    return F32_MAX;
  }
  return result;
}
f32 math_ceil(f32 x) {
  if (std::isnan(x) or std::isinf(x)) {
    return F32_MAX;
  }
  f32 result = std::ceil(x);
  if (std::isnan(result) or std::isinf(result)) {
    return F32_MAX;
  }
  return result;
}
f32 math_abs(f32 x) {
  if (std::isnan(x) or std::isinf(x)) {
    return F32_MAX;
  }
  f32 result = std::abs(x);
  if (std::isnan(result) or std::isinf(result)) {
    return F32_MAX;
  }
  return result;
}

