

#ifndef DEFINES_H
#define DEFINES_H

#include "raylib.h"
#include <stdbool.h>

#define RESOURCE_PATH "D:/Workspace/resources/"
#define TOTAL_ALLOCATED_MEMORY 64 * 1024 * 1024
#define TARGET_FPS 60

#define UI_FONT_SPACING 1
#define SCREEN_WIDTH 1280    
#define SCREEN_HEIGHT 720
#define SCREEN_OFFSET 5
#define SCREEN_WIDTH_DIV2 SCREEN_WIDTH / 2.f
#define SCREEN_HEIGHT_DIV2 SCREEN_HEIGHT / 2.f

#define MAX_PLAYER_LEVEL 100
#define MAX_SPAWN_COUNT 100
#define MAX_PROJECTILE_COUNT 50
#define MAX_TEXTURE_SLOTS 10
#define MAX_IMAGE_SLOTS 10
#define MAX_ABILITY_AMOUNT 10
#define MAX_SPRITESHEET_SLOTS 50
#define MAX_SPRITE_RENDERQUEUE 50
#define MAX_TILEMAP_TILESLOT_X 255
#define MAX_TILEMAP_TILESLOT_Y 255
#define MAX_TILEMAP_TILESLOT MAX_TILEMAP_TILESLOT_X * MAX_TILEMAP_TILESLOT_Y

#define DEBUG_COLLISIONS 0

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

typedef enum actor_type {
    ENEMY,
    PROJECTILE_ENEMY,
    PLAYER,
    PROJECTILE_PLAYER,
} actor_type;

typedef enum scene_type {
    SCENE_MAIN_MENU,
    SCENE_IN_GAME,
    SCENE_IN_GAME_EDIT
} scene_type;

typedef enum ability_type {
    FIREBALL,
    SALVO,
    RADIATION,
    DIRECT_FIRE,
} ability_type;

typedef enum elapse_time_type {
    SALVO_ETA
} elapse_time_type;

typedef enum texture_type {
    TEX_UNSPECIFIED,
    PLAYER_TEXTURE,
    ENEMY_TEXTURE,
    BACKGROUND,
	BUTTON_TEXTURE,
	HEALTHBAR_TEXTURE,
	HEALTH_PERC_TEXTURE,
	MAP_TILESET_TEXTURE,
} texture_type;

typedef enum image_type {
    IMG_UNSPECIFIED,
} image_type;

typedef enum spritesheet_playmod {
	SPRITESHEET_PLAYMOD_UNSPECIFIED,
	ON_SITE,
	ON_PLAYER
} spritesheet_playmod;

typedef enum world_direction {
	LEFT,
	RIGHT,
} world_direction;

typedef enum spritesheet_type {
	SPRITESHEET_UNSPECIFIED,
	PLAYER_ANIMATION_IDLE_LEFT,
	PLAYER_ANIMATION_IDLE_RIGHT,
	PLAYER_ANIMATION_MOVE_LEFT,
	PLAYER_ANIMATION_MOVE_RIGHT,
	PLAYER_ANIMATION_TAKE_DAMAGE_LEFT,
	PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT,
	PLAYER_ANIMATION_WRECK_LEFT,
	PLAYER_ANIMATION_WRECK_RIGHT,
	BUTTON_REFLECTION_SHEET,
	BUTTON_CRT_SHEET,
	LEVEL_UP_SHEET,

	SPRITESHEET_TYPE_MAX
} spritesheet_type;

typedef enum button_state {
	BTN_STATE_UP,
	BTN_STATE_HOVER,
	BTN_STATE_PRESSED
} button_state;

typedef enum button_type { 
	BTN_TYPE_UNDEFINED, 
	BTN_TYPE_MAINMENU_BUTTON_PLAY, 
	BTN_TYPE_MAINMENU_BUTTON_EDIT, 
	BTN_TYPE_MAINMENU_BUTTON_SETTINGS, 
	BTN_TYPE_MAINMENU_BUTTON_EXTRAS, 
	BTN_TYPE_MAINMENU_BUTTON_EXIT, 
	BTN_TYPE_INGAME_PAUSEMENU_BUTTON_RESUME, 
	BTN_TYPE_INGAME_PAUSEMENU_BUTTON_SETTINGS, 
	BTN_TYPE_INGAME_PAUSEMENU_BUTTON_MAINMENU, 
	BTN_TYPE_INGAME_PAUSEMENU_BUTTON_EXIT, 
	BTN_TYPE_SQUARE,

	BTN_TYPE_MAX
} button_type;
typedef enum resource_type {
  RESOURCE_TYPE_SPRITESHEET,
  RESOURCE_TYPE_TEXTURE
} resource_type;

typedef enum dialog_type {
	DIALOG_TYPE_IN_GAME_UI,
	DIALOG_TYPE_MAIN_MENU_UI,
	DIALOG_TYPE_PAUSE_MENU,
	DIALOG_TYPE_SKILL_UP,
	DIALOG_TYPE_TILE_SELECTION
} dialog_type;

typedef struct tilemap {
	Texture2D* tex;
	u16 origin_tilesize;
	
	Vector2 position;
	u16 grid_size;
	u16 cell_size;
	Color grid_color;
	Vector2 tiles[MAX_TILEMAP_TILESLOT_X][MAX_TILEMAP_TILESLOT_Y];
} tilemap;

typedef struct rectangle_collision {
	u16 owner_id;
	actor_type owner_type;
	Rectangle rect;
	bool is_active;
} rectangle_collision;

typedef struct spritesheet {
	spritesheet_type type;
	u16 col_total;
	u16 row_total;
	u16 frame_total;
	u16 current_col;
	u16 current_row;
	u16 current_frame;
	Rectangle current_frame_rect;
	Rectangle coord;

	Texture2D handle;
	i16 fps;
	i16 counter;
	u16 render_queue_index;
	u16 attached_spawn;
	world_direction w_direction;
	spritesheet_playmod playmod;
	bool should_center;
	bool is_started;
	bool play_once;
} spritesheet;

typedef struct spritesheet_play_system {
	spritesheet renderqueue[MAX_SPRITE_RENDERQUEUE];
	u16 renderqueue_count;
} spritesheet_play_system;

typedef struct button {
	u16 id;
	const char* text;
  	bool show;
	Vector2 text_pos;
	u16 text_spacing;
    button_type btn_type;
	texture_type tex_type;
    button_state state;
	u16 crt_render_index;
	u16 reflection_render_index;
	bool is_reflection_played;
    
    Rectangle source;
    Rectangle dest;
} button;

typedef struct Character2D {
    u16 character_id;
    Texture2D* tex;
    bool initialized;

    Rectangle collision;
    Vector2 position;
	world_direction w_direction;
    actor_type type;

    u16 rotation;
    u16 health;
    i16 damage;
    f32 speed;
} Character2D;

typedef struct ability {
    bool is_on_fire;

    ability_type type;
    Vector2 position;
    u16 rotation;
    i16 fire_rate;
    u8 level;

    Vector2 projectile_target_position[MAX_PROJECTILE_COUNT];
    Character2D projectiles[MAX_PROJECTILE_COUNT];
    f32 projectile_process[MAX_PROJECTILE_COUNT];

    f32 overall_process;
} ability;

typedef struct ability_system_state {
    actor_type owner_type;
    Vector2 owner_position;
	u16 player_width;
	u16 player_height;
    bool is_dirty_ability_system;

    ability abilities[MAX_ABILITY_AMOUNT];
    i16 ability_amount;

	u16 fire_ball_ball_count;
	u16 fire_ball_ball_radius;
	u16 fire_ball_ball_diameter;
	u16 fire_ball_circle_radius;
	u16 fire_ball_circle_radius_div_2;
	u16 fire_ball_circle_radius_div_4;

	u16 radiation_circle_radius;
	u16 radiation_circle_diameter;
	u16 radiation_circle_radius_div_2;
	u16 radiation_circle_radius_div_4;

	u16 direct_fire_square_width;
	u16 direct_fire_square_height;
	u16 direct_fire_square_height_div_2;

	u16 salvo_projectile_at_a_time;
	u16 salvo_fire_count;
	u16 salvo_projectile_count;
	u16 salvo_fire_rate;
} ability_system_state;

typedef struct player_state {
    bool initialized;
	bool player_have_skill_points;
	bool is_moving;
	bool is_dead;
	bool is_damagable;

    Vector2 position;
	Vector2 dimentions;
    Vector2 dimentions_div2;
	world_direction w_direction;

	u16 move_left_sprite_queue_index;
	u16 move_right_sprite_queue_index;
	u16 idle_left_sprite_queue_index;
	u16 idle_right_sprite_queue_index;
	u16 take_damage_left_sprite_queue_index;
	u16 take_damage_right_sprite_queue_index;
	u16 wreck_left_sprite_queue_index;
	u16 wreck_right_sprite_queue_index;
	u16 last_played_sprite_id;

    u16 level;
    u16 health_max;
    u16 health_current;
    f32 health_perc;
    u32 exp_to_next_level;
    u32 exp_current; 
    f32 exp_perc;
	float damage_break_time;
	float damage_break_current;

    Rectangle collision;
    ability_system_state ability_system;
	spritesheet_play_system spritesheet_system;
} player_state;

typedef struct resource_system_state {
    i16 texture_amouth;
	i16 sprite_amouth;
	i16 image_amouth;
    Texture2D textures[MAX_TEXTURE_SLOTS];
	spritesheet sprites[MAX_SPRITESHEET_SLOTS];
	Image images[MAX_IMAGE_SLOTS];

    scene_type game_on_scene;
} resource_system_state;

typedef struct spawn_system_state {
    Character2D spawns[MAX_SPAWN_COUNT];
    u16 current_spawn_count;
} spawn_system_state;

typedef struct user_interface_system_state {
	spritesheet_play_system spritesheet_system;
	player_state* p_player;
	Vector2 offset;
	Vector2 mouse_pos;
	button buttons[BTN_TYPE_MAX];
	Font ui_font;

	scene_type scene_data;
	bool b_show_pause_screen;
	bool b_show_tilemap_screen;
	bool b_user_interface_system_initialized;
} user_interface_system_state;

typedef struct game_manager_system_state {
	rectangle_collision spawn_collisions[MAX_SPAWN_COUNT];
	u16 spawn_collision_count;
	scene_type scene_data;

	bool is_game_paused;
	bool game_manager_initialized;
} game_manager_system_state;

typedef struct scene_manager_system_state {
	spritesheet_play_system spritesheet_system;
	game_manager_system_state* p_game_manager;
	
	scene_type scene_data;
	Vector2 screen_size;
	Vector2 screen_half_size;
	Vector2 target;
	u16 gridsize;
	u16 map_size;

	bool is_scene_manager_initialized;
} scene_manager_system_state;

typedef struct memory_system_state {
    u64 linear_memory_total_size;
    u64 linear_memory_allocated;
    void* linear_memory;
} memory_system_state;

typedef struct game_state {
    f32 delta_time;
} game_state;

typedef struct timer {
    elapse_time_type type;
    f32 total_delay;
    f32 remaining;
} timer;

static const u32 level_curve[MAX_PLAYER_LEVEL+1] =
{
	0,	//	0
	300,
	800,
	1500,
	2500,
	4300,
	7200,
	11000,
	17000,
	24000,
	33000,	//	10
	43000,
	58000,
	76000,
	100000,
	130000,
	169000,
	219000,
	283000,
	365000,
	472000,	//	20
	610000,
	705000,
	813000,
	937000,
	1077000,
	1237000,
	1418000,
	1624000,
	1857000,
	2122000,	//	30
	2421000,
	2761000,
	3145000,
	3580000,
	4073000,
	4632000,
	5194000,
	5717000,
	6264000,
	6837000,	//	40
	7600000,
	8274000,
	8990000,
	9753000,
	10560000,
	11410000,
	12320000,
	13270000,
	14280000,
	15340000,	//	50
	16870000,
	18960000,
	19980000,
	21420000,
	22930000,
	24530000,
	26200000,
	27960000,
	29800000,
	32780000,	//	60
	36060000,
	39670000,
	43640000,
	48000000,
	52800000,
	58080000,
	63890000,
	70280000,
	77310000,
	85040000,	//	70
	93540000,
	102900000,
	113200000,
	124500000,
	137000000,
	150700000,
	165700000,
	236990000,
	260650000,
	286780000,	//	80
	315380000,
	346970000,
	381680000,
	419770000,
	461760000,
	508040000,
	558740000,
	614640000,
	676130000,
	743730000,	//	90
	1041222000,
	1145344200,
	1259878620,
	1385866482,
	1524453130,
	1676898443,
	1844588288,
	2029047116,
	2050000000,	//	
	2150000000u,	//	100
};

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

#define FCLAMP(value, min, max) (value <= min) ? min : (value >= max) ? max \
                                                                      : value;
                                                       
#define FMAX(v1, v2) (v1 >= v2) ? v1 : v2 
                                                           
#define FMIN(v1, v2) (v1 <= v2) ? v1 : v2
                                                                      
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
