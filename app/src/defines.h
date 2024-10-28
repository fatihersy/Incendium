

#ifndef DEFINES_H
#define DEFINES_H

#include "raylib.h"
#include <stdlib.h>  // Required for: malloc(), free()
#include <string.h>

#define MAX_SPAWN_COUNT 1000
#define MAX_PROJECTILE_COUNT 50
#define MAX_TEXTURE_SLOTS 50

#define MAX_ABILITY_AMOUNT 10

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
#define DIRECT_FIRE_SQUARE_HEIGHT_DIV_2 DIRECT_FIRE_SQUARE_HEIGHT / 2

#define SALVO_PROJECTILE_AT_A_TIME 2
#define SALVO_FIRE_COUNT 3
#define SALVO_PROJECTILE_COUNT SALVO_PROJECTILE_AT_A_TIME * SALVO_FIRE_COUNT
#define SALVO_FIRE_RATE 1  // in sec

#define DEBUG_COLLISIONS 1


// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;

typedef struct resource_system_state {
    i16 texture_amouth;

} resource_system_state;

typedef enum elapse_time_type {
    SALVO_ETA
} elapse_time_type;

typedef struct timer {
    elapse_time_type type;
    f32 total_delay;
    f32 remaining;
} timer;

typedef struct Character2D {
    u16 character_id;
    unsigned int texId;
    bool initialized;

    Rectangle collision_rect;
    Vector2 position;

    u16 rotation;
    i16 health;
    i16 damage;
    f32 speed;
} Character2D;

typedef struct spawn_system_state {
    Character2D* spawns[MAX_SPAWN_COUNT];
    u16 current_spawn_count;
} spawn_system_state;

typedef enum actor_type {
    ENEMY,
    PROJECTILE_ENEMY,
    PLAYER,
    PROJECTILE_PLAYER,
} actor_type;

typedef enum ability_type {
    fireball,
    salvo,
    radiation,
    direct_fire,
} ability_type;

typedef struct collision {
    bool is_active;

    actor_type type;
    Rectangle* rect;
    i16 amount;
} collision;

typedef struct ability {
    bool is_on_fire;

    ability_type type;
    Vector2 position;
    u16 rotation;
    i16 fire_rate;

    Vector2* projectile_target_position[MAX_PROJECTILE_COUNT];
    Character2D* projectiles[MAX_PROJECTILE_COUNT];
    f32* projectile_process[MAX_PROJECTILE_COUNT];

    f32 overall_process;
} ability;

typedef struct ability_system_state {
    actor_type owner_type;
    Vector2 owner_position;

    ability* abilities[MAX_ABILITY_AMOUNT];
    i16 ability_amount;
} ability_system_state;

typedef struct game_data {
} game_data;

typedef struct player_state {
    bool initialized;
    Vector2 position;
    collision collisions;
    Texture2D player_texture;
    const char* texture_path;
} player_state;

typedef struct game_state {
    f32 delta_time;
} game_state;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected float to be 8 bytes.");

/**
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID32 4294967295U
#define INVALID_ID16 65535U

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define KPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define KPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define KPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define KPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define KPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define KPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define KPLATFORM_IOS 1
#define KPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define KPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#ifdef KEXPORT
// Exports
#ifdef _MSC_VER
#define KAPI __declspec(dllexport)
#else
#define KAPI __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define KAPI __declspec(dllimport)
#else
#define KAPI
#endif
#endif

#define KCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max \
                                                                      : value;

// Inlining
#if defined(__clang__) || defined(__gcc__)
#define KINLINE __attribute__((always_inline)) inline
#define KNOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define KINLINE __forceinline
#define KNOINLINE __declspec(noinline)
#else
#define KINLINE static inline
#define KNOINLINE
#endif

#endif
