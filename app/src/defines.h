

#ifndef DEFINES_H
#define DEFINES_H

#include "raylib.h"
#include <stdlib.h>  // Required for: malloc(), free()

#define MAX_SPAWN_COUNT 1000

#define MAX_PROJECTILE_COUNT 500

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

typedef struct resource_system_state 
{
    i16 texture_amouth;

} resource_system_state;

typedef enum actor_type 
{
    ENEMY,
    PROJECTILE_ENEMY,
    PLAYER,
    PROJECTILE_PLAYER,
} actor_type;

typedef enum ability_type {
    fireball,
} ability_type;

typedef struct collision 
{
    bool is_active;

    actor_type type;
    Rectangle* rect;
    i16 amount;
} collision;

typedef struct Character2D {
    unsigned int texId;
    i16 character_id;
    bool initialized;

    Rectangle collision_rect;
    Vector2 position;
    i16 health;
    i16 speed;
} Character2D;

typedef struct ability
{
    ability_type type;
    Vector2 location;
    i16 rotation;
    Character2D* projectiles;
} ability;

typedef struct ability_system_state 
{
    actor_type owner_type;
    Vector2 owner_position;

    ability* abilities;
    i16 ability_amount;
} ability_system_state;

typedef struct game_data {
    
} game_data;


typedef struct player_state
{
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
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

/**
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID 4294967295U

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