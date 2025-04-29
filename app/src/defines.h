#ifndef DEFINES_H
#define DEFINES_H

#include <raylib.h>
#include <array>
#include <string>

#define GAME_TITLE "Incendium"

#define RESOURCE_PATH "C:\\Users\\fatih\\repositories\\resources\\"
#define MAP_LAYER_PATH "map_layers\\"
#define SHADER_PATH "../app/src/shaders/"

#define TOTAL_ALLOCATED_MEMORY 512 * 1024 * 1024
#define TARGET_FPS 60

#define DEBUG_COLLISIONS 0
#define USE_PAK_FORMAT 0

#define PAK_FILE_LOCATION "./resource.pak"
#define CONFIG_FILE_LOCATION "./config.ini"
#define SAVE_GAME_EXTENSION ".save_slot"

#define SCREEN_OFFSET CLITERAL(Vector2){5, 5}

#define BASE_RENDER_RES     CLITERAL(Vector2){ 1920, 1080}
#define BASE_RENDER_SCALE(FLOAT) CLITERAL(Vector2){ BASE_RENDER_RES.x * FLOAT , BASE_RENDER_RES.y * FLOAT }

#define MAX_IMAGE_SLOTS 10
#define MAX_SPRITESHEET_SLOTS 50

#define ATLAS_TEXTURE_ID TEX_ID_ASSET_ATLAS

#define MAX_UPDATE_ABILITY_PANEL_COUNT 3
#define MAX_UPDATE_PASSIVE_PANEL_COUNT 3

#define WHITE_ROCK CLITERAL(Color) {245, 246, 250,255}
#define CLEAR_BACKGROUND_COLOR BLACK

#define MAX_FILENAME_LENGTH 64
#define MAX_FILENAME_EXT_LENGTH 5

#define INI_FILE_MAX_FILE_SIZE 32000
#define INI_FILE_MAX_SECTION_NAME_LENGTH 32
#define INI_FILE_MAX_SECTION_LENGTH 512
#define INI_FILE_MAX_VARIABLE_NAME_LENGTH 32
#define INI_FILE_MAX_VARIABLE_VALUE_LENGTH 32
#define MAX_PARSED_TEXT_ARR_LEN 10
#define MAX_PARSED_TEXT_TEXT_LEN 10

#define MAX_SHADER_LOCATION_SLOT 16
#define MAX_SHADER_LOCATION_NAME_LENGTH 16

#define UI_FONT_SPACING 1

#define MAX_SLIDER_OPTION_SLOT 10
#define MAX_SLIDER_OPTION_TEXT_SLOT 18

#define MAX_TILESHEET_SLOTS 10
#define MAX_TILESHEET_UNIQUE_TILESLOTS_X 64
#define MAX_TILESHEET_UNIQUE_TILESLOTS_Y 64
#define MAX_TILESHEET_UNIQUE_TILESLOTS MAX_TILESHEET_UNIQUE_TILESLOTS_X * MAX_TILESHEET_UNIQUE_TILESLOTS_Y
#define MAX_TILEMAP_LAYERS 5
#define MAX_TILEMAP_TILESHEETSLOT 10
#define MAX_TILEMAP_TILESLOT_X 128
#define MAX_TILEMAP_TILESLOT_Y MAX_TILEMAP_TILESLOT_X
#define MAX_TILEMAP_TILESLOT MAX_TILEMAP_TILESLOT_X * MAX_TILEMAP_TILESLOT_Y
#define TILEMAP_TILE_START_SYMBOL 0x21 // Refers to ASCII exclamation mark. First visible character on the chart. To debug.
#define TILESHEET_TILE_SYMBOL_STR_LEN 2
#define TILESHEET_PROP_SYMBOL_STR_LEN 60
#define MAX_TILESHEET_PROPS 1024
#define MAX_TILEMAP_PROPS 255
#define MAX_TILEMAP_FILENAME_LEN 20

#define MAX_ITEM_ACTOR_NAME_LENGTH 10

#define MAX_PLAYER_LEVEL 100
#define MAX_SPAWN_COUNT 100
#define MAX_SPAWN_HEALTH 100000
#define MAX_SPAWN_COLLISIONS 1

#define MAX_ABILITY_NAME_LENGTH 10
#define MAX_ABILITY_SLOT 5
#define MAX_ABILITY_PROJECTILE_SLOT 6
#define MAX_ABILITY_LEVEL 7

#define MAX_PASSIVE_UPGRADE_TIER 5
#define MAX_PASSIVE_NAME_LENGTH 15
#define MAX_PASSIVE_DESC_LENGTH 50

#define MAX_WORLDMAP_LOCATION_NAME_LENGTH 10
#define MAX_WORLDMAP_LOCATIONS 22
#define WORLDMAP_LOC_PIN_SIZE 32 // Also check the SHEET_ID_ICON_ATLAS frame sizes.
#define WORLDMAP_LOC_PIN_SIZE_DIV2 WORLDMAP_LOC_PIN_SIZE * .5f // Needed?
#define WORLDMAP_MAINMENU_MAP 0

#define MAX_RAND 6
#define RANDOM_TABLE_NUMBER_COUNT 507
#define RANDOM_STACK_COUNT 10

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

// Ensure all types are of the correct size.
static_assert(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
static_assert(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
static_assert(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
static_assert(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

static_assert(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
static_assert(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
static_assert(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
static_assert(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

static_assert(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
static_assert(sizeof(f64) == 8, "Expected float to be 8 bytes.");


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

typedef struct file_data {
  i8 file_name[MAX_FILENAME_LENGTH];
  i8 file_extension[MAX_FILENAME_EXT_LENGTH];
  u64 size;
  u32 offset;
  u8* data;
  bool is_initialized;
}file_data;

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
  SHADER_ID_FADE_TRANSITION,
  SHADER_ID_MAX,
} shader_id;

typedef enum spawn_type {
  SPAWN_TYPE_UNDEFINED,
  SPAWN_TYPE_BROWN,
  SPAWN_TYPE_ORANGE,
  SPAWN_TYPE_YELLOW,
  SPAWN_TYPE_RED,
  SPAWN_TYPE_MAX
} spawn_type;

typedef enum actor_type {
  ACTOR_TYPE_SPAWN,
  ACTOR_TYPE_PROJECTILE_SPAWN,
  ACTOR_TYPE_PLAYER,
  ACTOR_TYPE_PROJECTILE_PLAYER,
} actor_type;

typedef enum scene_id {
  SCENE_TYPE_UNSPECIFIED,
  SCENE_TYPE_MAIN_MENU,
  SCENE_TYPE_IN_GAME,
  SCENE_TYPE_EDITOR,
  SCENE_TYPE_MAX
} scene_id;

// LABEL: Move Type
typedef enum movement_pattern {
  MOVE_TYPE_UNDEFINED,
  MOVE_TYPE_SATELLITE,
  MOVE_TYPE_BULLET,
  MOVE_TYPE_COMET,
  MOVE_TYPE_MAX,
} movement_pattern;

typedef enum ability_upgradables {
  ABILITY_UPG_UNDEFINED,
  ABILITY_UPG_DAMAGE,
  ABILITY_UPG_HITBOX,
  ABILITY_UPG_SPEED,
  ABILITY_UPG_AMOUNT,
  ABILITY_UPG_MAX,
} ability_upgradables;

// LABEL: Ability types
typedef enum ability_type {
  ABILITY_TYPE_UNDEFINED,
  ABILITY_TYPE_FIREBALL,
  ABILITY_TYPE_BULLET,
  //ABILITY_TYPE_RADIATION,
  ABILITY_TYPE_COMET,
  ABILITY_TYPE_MAX,
} ability_type;

typedef enum character_stats {
  CHARACTER_STATS_UNDEFINED,
  CHARACTER_STATS_HEALTH,
  CHARACTER_STATS_HP_REGEN,
  CHARACTER_STATS_MOVE_SPEED,
  CHARACTER_STATS_AOE,
  CHARACTER_STATS_DAMAGE,
  CHARACTER_STATS_ABILITY_CD,
  CHARACTER_STATS_PROJECTILE_AMOUTH,
  CHARACTER_STATS_EXP_GAIN,
  CHARACTER_STATS_MAX,
} character_stats;

typedef enum sound_id {
  SOUND_ID_UNSPECIFIED,
  SOUND_ID_BUTTON_ON_CLICK,
  SOUND_ID_DENY,
  SOUND_ID_FIRE_ON_HIT,
  SOUND_ID_MAX,
} sound_id;

typedef enum music_id {
  MUSIC_ID_UNSPECIFIED,
  MUSIC_ID_MAIN_MENU_THEME,
  MUSIC_ID_NIGHT_THEME2,
  MUSIC_ID_TRACK5,
  MUSIC_ID_MAX,
} music_id;

typedef enum texture_id {
  TEX_ID_UNSPECIFIED,
  TEX_ID_WORLDMAP_WO_CLOUDS,
  TEX_ID_GAME_START_LOADING_SCREEN,
  TEX_ID_ASSET_ATLAS,
  TEX_ID_MAX,
} texture_id;

typedef enum atlas_texture_id {
  ATLAS_TEX_ID_UNSPECIFIED,
  ATLAS_TEX_ID_PLAYER_TEXTURE,
  ATLAS_TEX_ID_SPAWN_TEXTURE,
  ATLAS_TEX_ID_MAP_TILESET_TEXTURE,
  ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,
  ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL,
  ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,
  ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,
  ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE,
  ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR,
  ATLAS_TEX_ID_MAP_PROPS_ATLAS,
  ATLAS_TEX_ID_BG_BLACK,
  ATLAS_TEX_ID_ICON_ATLAS,
  ATLAS_TEX_ID_ZOMBIES_SPRITESHEET,
  ATLAS_TEX_ID_UI_ASSET_ATLAS,
  ATLAS_TEX_ID_CURRENCY_SOUL_ICON_5000,
  ATLAS_TEX_ID_CURRENCY_SOUL_ICON_15000,
  ATLAS_TEX_ID_CURRENCY_SOUL_ICON_45000,
  ATLAS_TEX_ID_MAX,
} atlas_texture_id;

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

typedef enum spritesheet_id {
  SHEET_ID_SPRITESHEET_UNSPECIFIED,
  SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT,
  SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT,
  SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT,
  SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT,
  SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT,
  SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT,
  SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT,
  SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT,
  SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_LEFT,
  SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_RIGHT,
  SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,
  SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,
  SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_LEFT,
  SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_RIGHT,
  SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,
  SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,
  SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_LEFT,
  SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_RIGHT,
  SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,
  SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,
  SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT,
  SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT,
  SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,
  SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,
  SHEET_ID_BUTTON_REFLECTION_SHEET,
  SHEET_ID_MENU_BUTTON,
  SHEET_ID_FLAT_BUTTON,
  SHEET_ID_BUTTON_CRT_SHEET,
  SHEET_ID_SCREEN_CRT_SHEET,
  SHEET_ID_SLIDER_PERCENT,
  SHEET_ID_SLIDER_OPTION,
  SHEET_ID_SLIDER_LEFT_BUTTON,
  SHEET_ID_SLIDER_RIGHT_BUTTON,
  SHEET_ID_FLAME_ENERGY_ANIMATION,
  SHEET_ID_FIREBALL_ANIMATION,
  SHEET_ID_FIREBALL_EXPLOTION_ANIMATION,
  SHEET_ID_SPRITESHEET_TYPE_MAX
} spritesheet_id;

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
  BTN_ID_MAINMENU_BUTTON_UPGRADE,
  BTN_ID_MAINMENU_BUTTON_EXIT,
  BTN_ID_MAINMENU_SETTINGS_CANCEL,
  BTN_ID_MAINMENU_UPGRADE_BACK,
  BTN_ID_MAINMENU_UPGRADE_BUY_UPGRADE,

  BTN_ID_IN_GAME_BUTTON_RETURN_MENU,

  BTN_ID_PAUSEMENU_BUTTON_INVENTORY,
  BTN_ID_PAUSEMENU_BUTTON_TECHNOLOGIES,
  BTN_ID_PAUSEMENU_BUTTON_CREDITS,
  BTN_ID_PAUSEMENU_BUTTON_SETTINGS,
  BTN_ID_PAUSEMENU_BUTTON_EXIT,

  BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_INC,
  BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_DEC,
  BTN_ID_EDITOR_BTN_STAGE_MAP_CHANGE_LEFT,
  BTN_ID_EDITOR_BTN_STAGE_MAP_CHANGE_RIGHT,

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
  DIALOG_TYPE_ABILITY_UPGRADE,
  DIALOG_TYPE_TILE_SELECTION
} dialog_type;

typedef enum tilesheet_type {
  TILESHEET_TYPE_UNSPECIFIED,
  TILESHEET_TYPE_MAP,

  TILESHEET_TYPE_MAX
} tilesheet_type;

  // 128 bytes
typedef union data128 {
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
  Texture2D* sampler;

  data128(){
    this->u64[0] = 0;
    this->u64[1] = 0;
  }
  
  data128(::i64 value1, ::i64 value2 = 0){
    this->i64[0] = value1;
    this->i64[1] = value2;
  }
  data128(::u64 value1, ::u64 value2 = 0){
    this->u64[0] = value1;
    this->u64[1] = value2;
  }
  data128(::f64 value1, ::f64 value2 = 0){
    this->f64[0] = value1;
    this->f64[1] = value2;
  }
  data128(::i32 value1, ::i32 value2 = 0, ::i32 value3 = 0, ::i32 value4 = 0){
    this->i32[0] = value1;
    this->i32[1] = value2;
    this->i32[2] = value3;
    this->i32[3] = value4;
  }
  data128(::u32 value1, ::u32 value2 = 0, ::u32 value3 = 0, ::u32 value4 = 0){
    this->u32[0] = value1;
    this->u32[1] = value2;
    this->u32[2] = value3;
    this->u32[3] = value4;
  }
  data128(::f32 value1, ::f32 value2 = 0, ::f32 value3 = 0, ::f32 value4 = 0){
    this->f32[0] = value1;
    this->f32[1] = value2;
    this->f32[2] = value3;
    this->f32[3] = value4;
  }
  data128(::u16 value1, ::u16 value2 = 0, ::u16 value3 = 0, ::u16 value4 = 0, ::u16 value5 = 0, ::u16 value6 = 0, ::u16 value7 = 0, ::u16 value8 = 0){
    this->u16[0] = value1;
    this->u16[1] = value2;
    this->u16[2] = value3;
    this->u16[3] = value4;
    this->u16[4] = value5;
    this->u16[5] = value6;
    this->u16[6] = value7;
    this->u16[7] = value8;
  }
  data128(::i16 value1, ::i16 value2 = 0, ::i16 value3 = 0, ::i16 value4 = 0, ::i16 value5 = 0, ::i16 value6 = 0, ::i16 value7 = 0, ::i16 value8 = 0){
    this->i16[0] = value1;
    this->i16[1] = value2;
    this->i16[2] = value3;
    this->i16[3] = value4;
    this->i16[4] = value5;
    this->i16[5] = value6;
    this->i16[6] = value7;
    this->i16[7] = value8;
  }
  data128(::u16* value, ::u16 len){
    for (int i=0; i < len; ++i) {
      this->u16[i] = value[i];
    }
  }
  data128(::i16* value, ::u16 len){
    for (int i=0; i < len; ++i) {
      this->i16[i] = value[i];
    }
  }
  data128(::u8* value, ::u16 len){
    for (int i=0; i < len; ++i) {
      this->u8[i] = value[i];
    }
  }
  data128(::i8* value, ::u16 len){
    for (int i=0; i < len; ++i) {
      this->i8[i] = value[i];
    }
  }
  data128(char* value, ::u16 len){
    for (int i=0; i < len; ++i) {
      c[i] = value[i];
    }
  }
  data128(Texture2D* _sampler){
    sampler = _sampler;
  }
} data128;

typedef struct data_pack {
  data_type type_flag;
  u16 array_lenght;
  Texture2D* sampler;

  data128 data;
  data_pack() {};
  data_pack(data_type type, data128 buffer, u16 len, Texture2D* tex2d = nullptr) : data_pack()
  {
    this->type_flag = type;
    this->array_lenght = len; 
    this->sampler = tex2d; 
    this->data = buffer;
  };
} data_pack;

typedef struct music_data {
  music_id id;
  Music handle;
  file_data file;

  bool play_once;
  bool played;
}music_data;

typedef struct sound_data {
  sound_id id;
  Sound handle;
  Wave wav;
  file_data file;

  bool play_once;
  bool played;
}sound_data;

typedef struct app_settings {
  Vector2 window_size;
  f32 normalized_ratio;
  Vector2 scale_ratio;
  u16 master_sound_volume;
  i32 window_state;
} app_settings;

typedef struct atlas_texture {
  Texture2D* atlas_handle;
  Rectangle source;
} atlas_texture;

typedef struct tile_symbol {
  u8 c[2];
}tile_symbol;

typedef struct tilesheet {
  tilesheet_type sheet_id;
  atlas_texture atlas_source;
  Texture2D *atlas_handle;

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

typedef struct worldmap_stage {
  u16 map_id;
  std::string displayname;
  std::string filename;
  std::array<Rectangle, MAX_SPAWN_COLLISIONS> spawning_areas = {};
  Vector2 screen_location;
  bool is_centered;
  bool is_active;
} worldmap_stage;

typedef struct tilemap_prop {
	atlas_texture_id atlas_id;
  u16 id;
  Rectangle source;
  Rectangle relative_source;
  Rectangle dest;
  bool is_initialized;
} tilemap_prop;

typedef struct tile_position {
  u16 layer;
  u16 x;
  u16 y;
} tile_position;


typedef struct tile {
  tile_position position = {};
  tile_symbol symbol = { .c { '\0', '\0' } };
  bool is_initialized = false;
} tile;

typedef struct tilemap {
  i8 filename[MAX_TILEMAP_LAYERS][MAX_TILEMAP_FILENAME_LEN];
  i8 propfile[MAX_TILEMAP_FILENAME_LEN];
  Vector2 position;
  u16 map_dim_total;
  u16 map_dim;

  tile_symbol tiles[MAX_TILEMAP_LAYERS][MAX_TILEMAP_TILESLOT_X][MAX_TILEMAP_TILESLOT_Y];
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

typedef struct spritesheet {
  spritesheet_id sheet_id;
  texture_id tex_id;
  Texture2D* tex_handle;
  Vector2 offset;
  u16 col_total;
  u16 row_total;
  u16 frame_total;
  u16 current_col;
  u16 current_row;
  u16 current_frame;
  Rectangle current_frame_rect;
  Rectangle coord;

  Color tint;
  f32 rotation;
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

typedef struct Character2D {
  u32 character_id;
  i32 health;
  i32 damage;
  f32 speed;
  actor_type type;
  Rectangle collision;
  world_direction w_direction;
  spritesheet move_right_animation;
  spritesheet move_left_animation;
  spritesheet take_damage_right_animation;
  spritesheet take_damage_left_animation;
  spritesheet* last_played_animation;
  Vector2 position;
  u16 rotation;
  f32 scale;
  f32 damage_break_time;
  bool is_dead;
  bool is_damagable;
  bool initialized;

  data128 buffer;

  Character2D() {}
  Character2D(u16 spawn_type, u16 player_level, u16 rnd_scale, Vector2 position, f32 speed) : Character2D()
  {
    type = ACTOR_TYPE_SPAWN;
    buffer.u16[0] = spawn_type;
    buffer.u16[1] = player_level;
    buffer.u16[2] = rnd_scale;
    this->position = position;
    this->speed = speed;
    is_damagable = true;
  }
  Character2D(Rectangle collision, i32 damage) : Character2D()
  {
    this->collision = collision;
    this->damage = damage;
    this->initialized = true;
  }
} Character2D;

typedef struct projectile {
  u16 id;
  spritesheet default_animation;
  spritesheet explotion_animation; // Explotion animation etc.
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
  bool play_explosion_animation;
} projectile;

typedef struct ability {
  void* p_owner;
  std::string display_name;
  std::array<projectile, MAX_ABILITY_PROJECTILE_SLOT> projectiles;
  std::array<ability_upgradables, ABILITY_UPG_MAX> upgradables;
  movement_pattern move_pattern;
  ability_type type;
  spritesheet_id default_animation_id;
  spritesheet_id explosion_animation_id;
  
  f32 ability_play_time;
  f32 proj_scale;
  u16 proj_count;
  u16 proj_speed;
  f32 proj_duration;
  Vector2 proj_dim;
  Rectangle icon_src;
  Vector2 position;
  u16 level;
  u16 base_damage;
  u16 rotation;
  bool center_proj_anim;
  bool is_active;
  bool is_initialized;

  ability(std::string name, std::array<ability_upgradables, ABILITY_UPG_MAX> upgrs, ability_type type, spritesheet_id def_anim_id, spritesheet_id expl_anim_id, movement_pattern pattern, f32 proj_scale, u16 proj_count, 
    u16 proj_speed, f32 proj_duration, Vector2 proj_dim, Rectangle icon_src, u16 base_damage, bool center_proj_anim, void* in_player = nullptr, bool is_active = false, bool is_initialized = false) 
    : p_owner(in_player), move_pattern(pattern), type(type), default_animation_id(def_anim_id), explosion_animation_id(expl_anim_id),  proj_scale(proj_scale), proj_count(proj_count), 
      proj_speed(proj_speed), proj_duration(proj_duration), proj_dim(proj_dim), icon_src(icon_src), base_damage(base_damage), center_proj_anim(center_proj_anim), is_active(is_active), 
      is_initialized(is_initialized)
  {
      for (int i=0; i < MAX_ABILITY_NAME_LENGTH; ++i) {
        display_name[i] = name[i];
      }
      display_name[MAX_ABILITY_NAME_LENGTH-1] = '\0';
      for (int i=0; i < ABILITY_UPG_MAX; ++i) {
        upgradables[i] = upgrs[i];
      }
  }

  ability() {
    p_owner = nullptr;
    type = ABILITY_TYPE_UNDEFINED;
    default_animation_id = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    explosion_animation_id = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    move_pattern = MOVE_TYPE_UNDEFINED;
    proj_scale = 0;
    proj_count = 0; 
    proj_speed = 0;
    proj_duration = 0;
    proj_dim = {0, 0};
    rotation = 0;
    base_damage = 0;
    center_proj_anim = false;
    is_active = false;
    is_initialized = false;
    level = 0;
    upgradables.fill(ABILITY_UPG_UNDEFINED);
    projectiles.fill({});
    display_name = "";
    icon_src = {};
    position = {};
    ability_play_time = 0;
  }
}ability;

typedef struct ability_play_system {
  ability abilities[MAX_ABILITY_SLOT];
} ability_play_system;

typedef struct character_stat {
  character_stats id;
  u16 level;
  std::string passive_display_name;
  std::string passive_desc;
  Rectangle passive_icon_src;
  i32 upgrade_cost;

  data128 buffer;

  character_stat() {}
  character_stat(character_stats id, std::string display_name, std::string desc, Rectangle icon_src, i32 upgrade_cost, data128 buffer = {}) : character_stat() 
  {
    this->id = id;
    this->passive_display_name = display_name;
    this->passive_desc = desc;
    this->passive_icon_src = icon_src;
    this->upgrade_cost = upgrade_cost;
    this->buffer = buffer;
    this->level = 1;
  }
}character_stat;

// LABEL: Player State
typedef struct player_state {
  ability_play_system ability_system;
  character_stat stats[CHARACTER_STATS_MAX];
  ability_type starter_ability;
  
  Rectangle collision;
  Vector2 dimentions;
  Vector2 dimentions_div2;
  spritesheet move_left_sprite;
  spritesheet move_right_sprite;
  spritesheet idle_left_sprite;
  spritesheet idle_right_sprite;
  spritesheet take_damage_left_sprite;
  spritesheet take_damage_right_sprite;
  spritesheet wreck_left_sprite;
  spritesheet wreck_right_sprite;
  spritesheet* last_played_animation;

  
  world_direction w_direction;
  Vector2 position;
  u32 level;
  u32 health_max;
  u32 health_current;
  u32 health_regen;
  f32 health_perc;
  u32 exp_to_next_level;
  u32 exp_current;
  f32 exp_perc;
  f32 damage_break_time;
  f32 damage_break_current;
  u32 damage;
  u16 projectile_amouth;
  f32 damage_area_multiply;
  f32 move_speed_multiply;
  f32 cooldown_multiply;
  f32 exp_gain_multiply;
  bool is_player_have_ability_upgrade_points;
  bool is_initialized;
  bool is_moving;
  bool is_dead;
  bool is_damagable;
} player_state;

typedef struct camera_metrics {
  Camera2D handle;
  Vector2 screen_offset;
  Rectangle frustum;
} camera_metrics;

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
#if defined(_WIN32) || defined(__WIN32__)
#define PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define PLATFORM_LINUX 1
#if defined(__ANDROID__)
#define PLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define PLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define PLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define PLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define PLATFORM_IOS 1
#define PLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define PLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
#define PLATFORM_MAC 1
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif

#define FCLAMP(value, min, max)                                                \
  (value <= min) ? min : (value >= max) ? max : value
#define FMAX(v1, v2) (v1 >= v2) ? v1 : v2
#define FMIN(v1, v2) (v1 <= v2) ? v1 : v2
#define FABS(v1) (v1 < 0) ? v1*(-1) : v1

#define pVECTOR2(X) (Vector2{(f32)X[0], (f32)X[1]})
#define VECTOR2(X, Y) (Vector2{X, Y})
#define TO_VECTOR2(X) (Vector2{X, X})
#define NORMALIZE_VEC2(X, Y, X_MAX, Y_MAX)  ( Vector2 {X / X_MAX, Y / Y_MAX})
#define CENTER_RECT(RECT) (Rectangle{RECT.x - RECT.width / 2.f, RECT.y - RECT.height, RECT.width, RECT.height});
#define SCREEN_POS(X, Y) (Vector2{  \
  .x = BASE_RENDER_RES.x  * (X / 100), \
  .y = BASE_RENDER_RES.y * (Y / 100)  \
})

#endif
