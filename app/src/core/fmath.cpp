#include "fmath.h"
#include <cmath>
#include "reasings.h"

#include "raymath.h"

static inline Vector2 rotate_point(Vector2 p, Vector2 pivot, f32 angle_deg) {
  const f32 rad = angle_deg * DEG2RAD;
  const f32 s = sinf(rad);
  const f32 c = cosf(rad);
  const f32 tx = p.x - pivot.x;
  const f32 ty = p.y - pivot.y;
  return Vector2{ pivot.x + (tx * c - ty * s), pivot.y + (tx * s + ty * c) };
}
static inline void project_points_on_axis(const Vector2* pts, int count, Vector2 axis, f32* out_min, f32* out_max) {
  const Vector2 n = Vector2Normalize(axis);
  f32 min_v = F32_MAX;
  f32 max_v = -F32_MAX;
  for (int i = 0; i < count; ++i) {
    const f32 d = Vector2DotProduct(pts[i], n);
    if (d < min_v) min_v = d;
    if (d > max_v) max_v = d;
  }
  *out_min = min_v;
  *out_max = max_v;
}

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
Vector2 vec2_clamp(Vector2 v, Vector2 min, Vector2 max) {
  return Vector2Clamp(v, min, max);
}
 Vector2 vec2_subtract(Vector2 v1, Vector2 v2) {
  return Vector2Subtract(v1, v2);
}
 Vector2 vec2_add(Vector2 v1, Vector2 v2) {
  return Vector2Add(v1, v2);
}
 Vector2 vec2_scale(Vector2 v1, f32 f1) {
  return Vector2Scale(v1, f1);
}
Vector2 vec2_normalize(Vector2 v1) {
  return Vector2Normalize(v1);
}
f32 vec2_distance(Vector2 v1, Vector2 v2) {
  return Vector2Distance(v1, v2);
}
f32 vec2_lenght(Vector2 v1) { 
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

f32 math_easing(f32 accumulator, f32 begin, f32 change, f32 duration, easing_type easing_function) {
  switch (easing_function) {
    case EASING_TYPE_LINEAR_NONE:  return EaseLinearNone(accumulator, begin, change, duration);
    case EASING_TYPE_LINEAR_IN:    return EaseLinearIn(accumulator, begin, change, duration);
    case EASING_TYPE_LINEAR_OUT:   return EaseLinearOut(accumulator, begin, change, duration);
    case EASING_TYPE_LINEAR_INOUT: return EaseLinearInOut(accumulator, begin, change, duration);
    case EASING_TYPE_SINE_IN:      return EaseSineIn(accumulator, begin, change, duration);
    case EASING_TYPE_SINE_OUT:     return EaseSineOut(accumulator, begin, change, duration);
    case EASING_TYPE_SINE_INOUT:   return EaseSineInOut(accumulator, begin, change, duration);
    case EASING_TYPE_CIRC_IN:      return EaseCircIn(accumulator, begin, change, duration);
    case EASING_TYPE_CIRC_OUT:     return EaseCircOut(accumulator, begin, change, duration);
    case EASING_TYPE_CIRC_INOUT:   return EaseCircInOut(accumulator, begin, change, duration);
    case EASING_TYPE_CUBIC_IN:     return EaseCubicIn(accumulator, begin, change, duration);
    case EASING_TYPE_CUBIC_OUT:    return EaseCubicOut(accumulator, begin, change, duration);
    case EASING_TYPE_CUBIC_INOUT:  return EaseCubicInOut(accumulator, begin, change, duration);
    case EASING_TYPE_QUAD_IN:      return EaseQuadIn(accumulator, begin, change, duration);
    case EASING_TYPE_QUAD_OUT:     return EaseQuadOut(accumulator, begin, change, duration);
    case EASING_TYPE_QUAD_INOUT:   return EaseQuadInOut(accumulator, begin, change, duration);
    case EASING_TYPE_EXPO_IN:      return EaseExpoIn(accumulator, begin, change, duration);
    case EASING_TYPE_EXPO_OUT:     return EaseExpoOut(accumulator, begin, change, duration);
    case EASING_TYPE_EXPO_INOUT:   return EaseExpoInOut(accumulator, begin, change, duration);
    case EASING_TYPE_BACK_IN:      return EaseBackIn(accumulator, begin, change, duration);
    case EASING_TYPE_BACK_OUT:     return EaseBackOut(accumulator, begin, change, duration);
    case EASING_TYPE_BACK_INOUT:   return EaseBackInOut(accumulator, begin, change, duration);
    case EASING_TYPE_BOUNCE_OUT:   return EaseBounceOut(accumulator, begin, change, duration);
    case EASING_TYPE_BOUNCE_IN:    return EaseBounceIn(accumulator, begin, change, duration);
    case EASING_TYPE_BOUNCE_INOUT: return EaseBounceInOut(accumulator, begin, change, duration);
    case EASING_TYPE_ELASTIC_IN:   return EaseElasticIn(accumulator, begin, change, duration);
    case EASING_TYPE_ELASTIC_OUT:  return EaseElasticOut(accumulator, begin, change, duration);
    case EASING_TYPE_ELASTIC_INOUT:return EaseElasticInOut(accumulator, begin, change, duration);
    default: {
      return F32_MAX;
    }
  }
}

bool check_collision_sat(Rectangle r1, f32 rot1, Vector2 origin1, Rectangle r2_aabb) {
  const Vector2 pivot = Vector2{ r1.x + origin1.x, r1.y + origin1.y };
  Vector2 r1_pts[4] = {
    rotate_point(Vector2{ r1.x,               r1.y               }, pivot, rot1),
    rotate_point(Vector2{ r1.x + r1.width,    r1.y               }, pivot, rot1),
    rotate_point(Vector2{ r1.x + r1.width,    r1.y + r1.height   }, pivot, rot1),
    rotate_point(Vector2{ r1.x,               r1.y + r1.height   }, pivot, rot1)
  };

  Vector2 r2_pts[4] = {
    Vector2{ r2_aabb.x,                 r2_aabb.y },
    Vector2{ r2_aabb.x + r2_aabb.width, r2_aabb.y },
    Vector2{ r2_aabb.x + r2_aabb.width, r2_aabb.y + r2_aabb.height },
    Vector2{ r2_aabb.x,                 r2_aabb.y + r2_aabb.height }
  };

  Vector2 axes[4] = {
    Vector2Subtract(r1_pts[1], r1_pts[0]),
    Vector2Subtract(r1_pts[3], r1_pts[0]),
    Vector2{ 1.f, 0.f },
    Vector2{ 0.f, 1.f }
  };

  for (int i = 0; i < 4; ++i) {
    const Vector2 axis = axes[i];
    if (Vector2Length(axis) <= 0.00001f) continue;
    f32 min1, max1, min2, max2;
    project_points_on_axis(r1_pts, 4, axis, &min1, &max1);
    project_points_on_axis(r2_pts, 4, axis, &min2, &max2);
    if (max1 < min2 || max2 < min1) {
      return false;
    }
  }
  return true;
}

Rectangle get_rotated_rect_aabb(Rectangle rect, f32 rotation, Vector2 origin) {
  const Vector2 pivot = Vector2{ rect.x + origin.x, rect.y + origin.y };
  Vector2 pts[4] = {
    rotate_point(Vector2{ rect.x,               rect.y               }, pivot, rotation),
    rotate_point(Vector2{ rect.x + rect.width,  rect.y               }, pivot, rotation),
    rotate_point(Vector2{ rect.x + rect.width,  rect.y + rect.height }, pivot, rotation),
    rotate_point(Vector2{ rect.x,               rect.y + rect.height }, pivot, rotation)
  };
  f32 min_x = F32_MAX, min_y = F32_MAX, max_x = -F32_MAX, max_y = -F32_MAX;
  for (int i = 0; i < 4; ++i) {
    if (pts[i].x < min_x) min_x = pts[i].x;
    if (pts[i].y < min_y) min_y = pts[i].y;
    if (pts[i].x > max_x) max_x = pts[i].x;
    if (pts[i].y > max_y) max_y = pts[i].y;
  }
  return Rectangle{ min_x, min_y, (max_x - min_x), (max_y - min_y) };
}

