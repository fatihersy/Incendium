#ifndef DEFINES_H
#define DEFINES_H

#include <raylib.h>

#define RESOURCE_PATH "D:/Workspace/resources/"
#define SHADER_PATH "../app/src/shaders/"

#define TOTAL_ALLOCATED_MEMORY 128 * 1024 * 1024
#define TARGET_FPS 60
#define MAX_SPRITE_RENDERQUEUE 50

#define MAX_IMAGE_SLOTS 10
#define MAX_SPRITESHEET_SLOTS 50

#define MAX_UPDATE_ABILITY_PANEL_COUNT 3

#define CLEAR_BACKGROUND_COLOR BLACK
#define BUTTON_TEXT_UP_COLOR WHITE_ROCK
#define BUTTON_TEXT_HOVER_COLOR WHITE
#define BUTTON_TEXT_PRESSED_COLOR WHITE
#define TEXT_SHADOW_COLOR BLACK
#define TEXT_SHADOW_OFFSET CLITERAL(Vector2){ 0, 1}

#define INI_FILE_MAX_FILE_SIZE 32000
#define INI_FILE_MAX_SECTION_NAME_LENGTH 32
#define INI_FILE_MAX_SECTION_LENGTH 512
#define INI_FILE_MAX_VARIABLE_NAME_LENGTH 32
#define INI_FILE_MAX_VARIABLE_VALUE_LENGTH 32
#define MAX_PARSED_TEXT_ARR_LEN 10
#define MAX_PARSED_TEXT_TEXT_LEN 10

#define MAX_SHADER_LOCATION_SLOT 16
#define MAX_SHADER_LOCATION_NAME_LENGHT 16

#define UI_FONT_SPACING 1

#define MAX_SLIDER_OPTION_SLOT 10
#define MAX_SLIDER_OPTION_TEXT_SLOT 18

#define MAX_TILESHEET_SLOTS 10
#define MAX_TILESHEET_UNIQUE_TILESLOTS_X 64
#define MAX_TILESHEET_UNIQUE_TILESLOTS_Y 64
#define MAX_TILESHEET_UNIQUE_TILESLOTS MAX_TILESHEET_UNIQUE_TILESLOTS_X * MAX_TILESHEET_UNIQUE_TILESLOTS_Y
#define MAX_TILEMAP_LAYERS 5
#define MAX_TILEMAP_TILESHEETSLOT 10
#define MAX_TILEMAP_TILESLOT_X 255
#define MAX_TILEMAP_TILESLOT_Y 255
#define MAX_TILEMAP_TILESLOT MAX_TILEMAP_TILESLOT_X * MAX_TILEMAP_TILESLOT_Y
#define TILEMAP_TILE_START_SYMBOL 0x21 // Refers to ASCII exclamation mark. First visible character on the chart. To debug.
#define TILESHEET_TILE_SYMBOL_STR_LEN 2
#define TILESHEET_PROP_SYMBOL_STR_LEN 60
#define MAX_TILESHEET_PROPS 1024
#define MAX_TILEMAP_PROPS 255
#define MAX_TILEMAP_FILENAME_LEN 15

#define MAX_ITEM_ACTOR_NAME_LENGHT 10
#define MAX_INVENTORY_SLOTS 50

#define MAX_PLAYER_LEVEL 100
#define MAX_SPAWN_COUNT 100

#define MAX_ABILITY_SLOT 5
#define MAX_ABILITY_PROJECTILE_SLOT 6

#define MAX_RAND 6

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


#define I64_MAX 9223372036854775807
#define U64_MAX 18446744073709551615U
#define F64_MAX 1.7976931348623157e+308
#define I32_MAX 2147483647
#define U32_MAX 4294967295U
#define F32_MAX 3.402823466e+38F
#define I16_MAX 32767
#define U16_MAX 65535U
#define  I8_MAX 127
#define  U8_MAX 255
/**
 * @brief Any id set to this should be considered invalid,
 * and not actually pointing to a real object.
 */
#define INVALID_ID32 U32_MAX
#define INVALID_ID16 U16_MAX

typedef enum data_type {
  DATA_TYPE_UNRESERVED,
  
  DATA_TYPE_I64,
  DATA_TYPE_U64,
  DATA_TYPE_F64,
  DATA_TYPE_I32,
  DATA_TYPE_U32,
  DATA_TYPE_F32,
  DATA_TYPE_I16,
  DATA_TYPE_U16,
  DATA_TYPE_I8,
  DATA_TYPE_U8,
  DATA_TYPE_C,
  DATA_TYPE_SAMPLER2D,

  DATA_TYPE_MAX,
}data_type;

typedef enum shader_id {
  SHADER_ID_UNSPECIFIED,
  SHADER_ID_PROGRESS_BAR_MASK,
  SHADER_ID_MAX,
} shader_id;

typedef enum actor_type {
  ENEMY,
  PROJECTILE_ENEMY,
  PLAYER,
  PROJECTILE_PLAYER,
} actor_type;

typedef enum scene_type {
  SCENE_MAIN_MENU,
  SCENE_IN_GAME,
  SCENE_EDITOR
} scene_type;

// LABEL: Move Type
typedef enum movement_pattern {
  MOVE_TYPE_UNDEFINED,
  MOVE_TYPE_SATELLITE,
  MOVE_TYPE_BULLET,
  MOVE_TYPE_COMET,
  MOVE_TYPE_MAX,
} movement_pattern;

// LABEL: Ability types
typedef enum ability_type {
  ABILITY_TYPE_UNDEFINED,
  ABILITY_TYPE_FIREBALL,
  ABILITY_TYPE_BULLET,
  ABILITY_TYPE_RADIATION,
  ABILITY_TYPE_COMET,
  ABILITY_TYPE_MAX,
} ability_type;

typedef enum elapse_time_type { SALVO_ETA } elapse_time_type;

typedef enum texture_id {
  TEX_ID_UNSPECIFIED,
  TEX_ID_PLAYER_TEXTURE,
  TEX_ID_ENEMY_TEXTURE,
  TEX_ID_BACKGROUND,
  TEX_ID_MAP_TILESET_TEXTURE,
  TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,
  TEX_ID_PROGRESS_BAR_INSIDE_FULL,
  TEX_ID_CRIMSON_FANTASY_PANEL,
  TEX_ID_CRIMSON_FANTASY_PANEL_BG,
  TEX_ID_MAP_PROPS_ATLAS,

  TEX_ID_MAX,
} texture_id;

typedef enum font_type {
  FONT_TYPE_UNDEFINED,
  FONT_TYPE_MOOD,
  FONT_TYPE_MOOD_OUTLINE,
  FONT_TYPE_MINI_MOOD,
  FONT_TYPE_MINI_MOOD_OUTLINE,
  FONT_TYPE_MAX
} font_type;

typedef enum image_type {
  IMAGE_TYPE_UNSPECIFIED,
  IMAGE_TYPE_A, // To Avoid warning
  IMAGE_TYPE_MAX
} image_type;

typedef enum spritesheet_playmod {
  SPRITESHEET_PLAYMOD_UNSPECIFIED,
  ON_SITE,
  ON_PLAYER
} spritesheet_playmod;

typedef enum world_direction {
  WORLD_DIRECTION_UNDEFINED,
  WORLD_DIRECTION_LEFT,
  WORLD_DIRECTION_RIGHT,
  WORLD_DIRECTION_UP,
  WORLD_DIRECTION_DOWN,
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
  MENU_BUTTON,
  FLAT_BUTTON,
  BUTTON_CRT_SHEET,
  SCREEN_CRT_SHEET,
  SLIDER_PERCENT,
  SLIDER_OPTION,
  SLIDER_LEFT_BUTTON,
  SLIDER_RIGHT_BUTTON,
  FIREBALL_ANIMATION,

  SPRITESHEET_TYPE_MAX
} spritesheet_type;

typedef enum button_state {
  BTN_STATE_UNDEFINED,
  BTN_STATE_UP,
  BTN_STATE_HOVER,
  BTN_STATE_PRESSED,
  BTN_STATE_RELEASED,
} button_state;

typedef enum button_type_id {
  BTN_TYPE_UNDEFINED,
  BTN_TYPE_MENU_BUTTON,
  BTN_TYPE_MENU_BUTTON_NO_CRT,
  BTN_TYPE_FLAT_BUTTON,
  BTN_TYPE_SLIDER_LEFT_BUTTON,
  BTN_TYPE_SLIDER_RIGHT_BUTTON,
  BTN_TYPE_MAX
} button_type_id;

typedef enum button_id {
  BTN_ID_UNDEFINED,

  BTN_ID_MAINMENU_BUTTON_PLAY,
  BTN_ID_MAINMENU_BUTTON_EDITOR,
  BTN_ID_MAINMENU_BUTTON_SETTINGS,
  BTN_ID_MAINMENU_BUTTON_EXTRAS,
  BTN_ID_MAINMENU_BUTTON_EXIT,
  BTN_ID_MAIN_MENU_SETTINGS_CANCEL_SETTINGS_BUTTON,

  BTN_ID_PAUSEMENU_BUTTON_INVENTORY,
  BTN_ID_PAUSEMENU_BUTTON_TECHNOLOGIES,
  BTN_ID_PAUSEMENU_BUTTON_CREDITS,
  BTN_ID_PAUSEMENU_BUTTON_SETTINGS,
  BTN_ID_PAUSEMENU_BUTTON_EXIT,

  BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_INC,
  BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_DEC,

  BTN_ID_SETTINGS_SLIDER_SOUND_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_SOUND_RIGHT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_RES_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_RES_RIGHT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_WIN_MODE_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_WIN_MODE_RIGHT_BUTTON,
  BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON,

  BTN_ID_MAX
} button_id;

typedef enum progress_bar_id {
  PRG_BAR_ID_UNDEFINED,
  PRG_BAR_ID_PLAYER_HEALTH,
  PRG_BAR_ID_PLAYER_EXPERIANCE,
  PRG_BAR_ID_MAX
} progress_bar_id;

typedef enum progress_bar_type_id {
  PRG_BAR_TYPE_ID_UNDEFINED,
  PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
  PRG_BAR_TYPE_ID_MAX
} progress_bar_type_id;

typedef enum slider_type_id {
  SDR_TYPE_UNDEFINED,
  SDR_TYPE_PERCENT,
  SDR_TYPE_OPTION,
  SDR_TYPE_MAX
} slider_type_id;

typedef enum slider_id {
  SDR_ID_UNDEFINED,
  SDR_ID_SETTINGS_SOUND_SLIDER,
  SDR_ID_SETTINGS_RES_SLIDER,
  SDR_ID_SETTINGS_WIN_MODE_SLIDER,
  SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER,
  SDR_ID_MAX
} slider_id;

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

typedef enum tilesheet_type {
  TILESHEET_TYPE_UNSPECIFIED,
  TILESHEET_TYPE_MAP,

  TILESHEET_TYPE_MAX
} tilesheet_type;

typedef struct string_parse_result {
  char buffer[MAX_PARSED_TEXT_ARR_LEN][MAX_PARSED_TEXT_TEXT_LEN];
} string_parse_result;

typedef struct data_pack {
  data_type type_flag;
  u16 array_lenght;
  Texture2D* sampler;

  // 128 bytes
  union {
    i64 i64[2];
    u64 u64[2];
    f64 f64[2];

    i32 i32[4];
    u32 u32[4];
    f32 f32[4];

    i16 i16[8];
    u16 u16[8];

    i8 i8[16];
    u8 u8[16];

    char c[16];
  } data;
} data_pack;

typedef struct fshader_location {
  char name[MAX_SHADER_LOCATION_NAME_LENGHT];
  u16 index;
  data_pack data;
  ShaderUniformDataType uni_data_type;
} fshader_location;

typedef struct fshader {
  u16 total_locations;
  Shader handle;
  fshader_location locations[MAX_SHADER_LOCATION_SLOT];
} fshader;

typedef struct app_settings {
  u32 resolution[2];
  Vector2 resolution_div4;
  Vector2 resolution_div3;
  Vector2 resolution_div2;
  Vector2 resolution_38div20;
  Vector2 resolution_35div20;
  Vector2 resolution_3div2;
  Vector2 resolution_5div4;
  char title[16];
  u16 master_sound_volume;
  i32 window_state;
} app_settings;

typedef struct tile_symbol {
  u8 c[3];
}tile_symbol;

typedef struct tilesheet {
  tilesheet_type sheet_type;
  texture_id tex_id;
  Texture2D *tex;

  tile_symbol tile_symbols[MAX_TILESHEET_UNIQUE_TILESLOTS_X][MAX_TILESHEET_UNIQUE_TILESLOTS_Y];
  u16 tile_count_x;
  u16 tile_count_y;
  u16 tile_count;
  u16 tile_size;
  u16 dest_tile_size;

  Vector2 position;
  f32 offset;
  bool is_initialized;
} tilesheet;

typedef struct tilemap_tile {
	tilesheet* sheet;

	u16 x;
	u16 y;
  tile_symbol tile_symbol;

  bool is_initialized;
} tilemap_tile;

typedef struct tilemap_prop {
	texture_id atlas_id;
  u16 id;
  Rectangle source;
  Rectangle dest;
  bool is_initialized;
} tilemap_prop;

typedef struct tilemap {
  i8 filename[MAX_TILEMAP_LAYERS][MAX_TILEMAP_FILENAME_LEN];
  Vector2 position;
  u16 map_dim_total;
  u16 map_dim;

  tilemap_tile tiles[MAX_TILEMAP_LAYERS][MAX_TILEMAP_TILESLOT_X][MAX_TILEMAP_TILESLOT_Y];
  tilemap_prop props[MAX_TILEMAP_PROPS];
  u16 tile_size;
  u16 prop_count;

  bool is_initialized;
} tilemap;

typedef struct tilemap_stringtify_package {
  u8 str_tilemap[MAX_TILEMAP_LAYERS][MAX_TILEMAP_TILESLOT * TILESHEET_TILE_SYMBOL_STR_LEN];
  u8 str_props[MAX_TILEMAP_PROPS][TILESHEET_PROP_SYMBOL_STR_LEN];
  i32 size_tilemap_str[MAX_TILEMAP_LAYERS];
  i32 size_props_str;
  bool is_success;
}tilemap_stringtify_package;

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
  Color tint;
  i16 fps;
  i16 counter;
  u16 render_queue_index;
  u16 attached_spawn;
  world_direction w_direction;
  spritesheet_playmod playmod;
  bool should_center;
  bool is_started;
  bool is_played;
  bool play_looped;
  bool play_once;
} spritesheet;

typedef struct spritesheet_play_system {
  spritesheet renderqueue[MAX_SPRITE_RENDERQUEUE];
  u16 renderqueue_count;
} spritesheet_play_system;

typedef struct panel {
  texture_id frame_tex_id;
  texture_id bg_tex_id;
  Color bg_tint;
  Color bg_hover_tint;
  Vector4 offsets;
  f32 zoom;
  f32 scroll;
  Vector2 draggable;
  button_state current_state;
  button_state signal_state;
  Rectangle dest;
  Rectangle scroll_handle;
  data_pack buffer[2];
  bool is_dragging_scroll;
} panel;

typedef struct button_type {
  button_type_id id;
  spritesheet_type ss_type;
  Vector2 source_frame_dim;
  Vector2 dest_frame_dim;
  f32 scale;
  Vector2 text_offset_on_click;
  bool play_reflection;
  bool play_crt;
  bool should_center;
} button_type;

typedef struct button {
  button_id id;
  button_type btn_type;
  button_state state;
  Rectangle dest;

  u16 crt_render_index;
  u16 reflection_render_index;

  bool on_screen;
  bool is_registered;
} button;

typedef struct slider_option {
  char display_text[MAX_SLIDER_OPTION_TEXT_SLOT];
  data_pack content;
} slider_option;

typedef struct slider_type {
  slider_type_id id;
  spritesheet_type ss_sdr_body;
  Vector2 source_frame_dim;
  f32 scale;
  u16 width_multiply;
  u16 char_limit;
  button_id left_btn_id;
  button_id right_btn_id;
  button_type_id left_btn_type_id;
  button_type_id right_btn_type_id;
  u16 left_btn_width;
  u16 right_btn_width;
  u16 origin_body_width;
  u16 body_width;
  u16 body_height;
  Vector2 whole_body_width;
} slider_type;

typedef struct slider {
  slider_id id;
  slider_type sdr_type;
  Vector2 position;

  slider_option options[MAX_SLIDER_OPTION_SLOT];
  u16 current_value;
  u16 max_value;
  u16 min_value;

  bool is_clickable;
  bool on_screen;
  bool is_registered;  
} slider;

typedef struct progress_bar_type {
  //texture_id body_repetitive;
  texture_id body_inside;
  texture_id body_outside;


  shader_id mask_shader_id;
} progress_bar_type;

typedef struct progress_bar {
  progress_bar_id id;
  progress_bar_type type;

  f32 width_multiply;
  Vector2 scale;

  f32 progress;
  bool is_initialized;
} progress_bar;

typedef struct Character2D {
  u16 character_id;
  Texture2D *tex;
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

typedef struct projectile {
  u16 id;
  u16 animation_sprite_queueindex;
  Vector2 position;
  Rectangle collision;
  world_direction direction;

  // 128 byte buffer
  union {
    i64 i64[2];
    u64 u64[2];
    f64 f64[2];

    i32 i32[4];
    u32 u32[4];
    f32 f32[4];

    i16 i16[8];
    u16 u16[8];

    i8 i8[16];
    u8 u8[16];

    char c[16];
  } buffer;

  u16 damage;
  f32 duration;
  bool is_active;
} projectile;

typedef struct ability {
  ability_type type;
  void* p_owner;
  Vector2 position;
  u16 level;
  u16 base_damage;
  u16 rotation;
  projectile projectiles[MAX_ABILITY_PROJECTILE_SLOT];
  spritesheet_type proj_anim_sprite;
  movement_pattern move_pattern;
  u16 proj_count;
  f32 proj_duration;
  Vector2 proj_dim;

  bool center_proj_anim;
  bool is_active;
  bool is_initialized;
}ability;

typedef struct ability_play_system {
  ability abilities[MAX_ABILITY_SLOT];
} ability_play_system;

typedef struct item_actor {
  i8 name[MAX_ITEM_ACTOR_NAME_LENGHT];
  texture_id icon_texture_id;
} item_actor;

typedef struct inventory_state {
  item_actor content[MAX_INVENTORY_SLOTS];
  u16 item_count;
} inventory_state;

// LABEL: Player State
typedef struct player_state {
  Rectangle collision;
  ability_play_system ability_system;
  spritesheet_play_system spritesheet_system;
  inventory_state inventory;
  ability_type starter_ability;

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
  f32 damage_break_time;
  f32 damage_break_current;

  bool is_player_have_skill_points;
  bool is_initialized;
  bool is_moving;
  bool is_dead;
  bool is_damagable;
} player_state;

typedef struct spawn_system_state {
  Character2D spawns[MAX_SPAWN_COUNT];
  u16 current_spawn_count;
} spawn_system_state;

typedef struct memory_system_state {
  u64 linear_memory_total_size;
  u64 linear_memory_allocated;
  void *linear_memory;
} memory_system_state;

typedef struct camera_metrics {
  Camera2D handle;
} camera_metrics;

typedef struct timer {
  elapse_time_type type;
  f32 total_delay;
  f32 remaining;
} timer;

static const u32 level_curve[MAX_PLAYER_LEVEL + 1] = {
    0, //	0
    300,         800,        1500,       2500,       4300,
    7200,        11000,      17000,      24000,
    33000, //	10
    43000,       58000,      76000,      100000,     130000,
    169000,      219000,     283000,     365000,
    472000, //	20
    610000,      705000,     813000,     937000,     1077000,
    1237000,     1418000,    1624000,    1857000,
    2122000, //	30
    2421000,     2761000,    3145000,    3580000,    4073000,
    4632000,     5194000,    5717000,    6264000,
    6837000, //	40
    7600000,     8274000,    8990000,    9753000,    10560000,
    11410000,    12320000,   13270000,   14280000,
    15340000, //	50
    16870000,    18960000,   19980000,   21420000,   22930000,
    24530000,    26200000,   27960000,   29800000,
    32780000, //	60
    36060000,    39670000,   43640000,   48000000,   52800000,
    58080000,    63890000,   70280000,   77310000,
    85040000, //	70
    93540000,    102900000,  113200000,  124500000,  137000000,
    150700000,   165700000,  236990000,  260650000,
    286780000, //	80
    315380000,   346970000,  381680000,  419770000,  461760000,
    508040000,   558740000,  614640000,  676130000,
    743730000, //	90
    1041222000,  1145344200, 1259878620, 1385866482, 1524453130,
    1676898443,  1844588288, 2029047116,
    2050000000,  //
    2150000000u, //	100
};

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

#define FCLAMP(value, min, max)                                                \
  (value <= min) ? min : (value >= max) ? max : value

#define FMAX(v1, v2) (v1 >= v2) ? v1 : v2

#define FMIN(v1, v2) (v1 <= v2) ? v1 : v2

#define FABS(v1) (v1 < 0) ? v1*(-1) : v1

#define pVECTOR2(X) ((Vector2){X[0], X[1]})

#define VECTOR2(X, Y) ((Vector2){X, Y})
#define TO_VECTOR2(X) ((Vector2){X, X})

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
