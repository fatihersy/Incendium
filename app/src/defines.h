#ifndef DEFINES_H
#define DEFINES_H

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

int alert(const char* caption, const char* message);

#define STEAM_APP_ID 3680620
#define GAME_TITLE "Incendium"

#define DEFAULT_SETTINGS_WINDOW_WIDTH "1280"
#define DEFAULT_SETTINGS_WINDOW_HEIGHT "720"
#define DEFAULT_SETTINGS_RATIO "16:9"
#define DEFAULT_SETTINGS_WINDOW_MODE "windowed"
#define DEFAULT_SETTINGS_MASTER_VOLUME_C "50"
#define DEFAULT_SETTINGS_MASTER_VOLUME 5
#define DEFAULT_SETTINGS_LANGUAGE "English"

#define RESOURCE_PATH ".\\resources\\"
#define MAP_LAYER_PATH "map_layers\\" // will look resources
#define SHADER_PATH ".\\resources\\"

#define TOTAL_ALLOCATED_MEMORY 512 * 1024 * 1024
#define TARGET_FPS 60

#define DEBUG_COLLISIONS 0

#ifdef _DEBUG
  #define USE_PAK_FORMAT 0
#else
  #define USE_PAK_FORMAT 1
#endif

#define PAK_FILE_LOCATION "./resource.pak"
#define CONFIG_FILE_LOCATION "./config.ini"
#define SAVE_GAME_EXTENSION ".save_slot"

#define SCREEN_OFFSET CLITERAL(Vector2){5.f, 5.f}

#define MAX_FILENAME_LENGTH 64
#define MAX_FILENAME_EXT_LENGTH 5

#define MAX_TILESHEET_UNIQUE_TILESLOTS_X 64
#define MAX_TILESHEET_UNIQUE_TILESLOTS_Y 64
#define MAX_TILESHEET_UNIQUE_TILESLOTS MAX_TILESHEET_UNIQUE_TILESLOTS_X * MAX_TILESHEET_UNIQUE_TILESLOTS_Y
#define MAX_TILEMAP_LAYERS 5
#define MAX_TILEMAP_TILESLOT_X 128
#define MAX_TILEMAP_TILESLOT_Y MAX_TILEMAP_TILESLOT_X
#define MAX_TILEMAP_TILESLOT MAX_TILEMAP_TILESLOT_X * MAX_TILEMAP_TILESLOT_Y
#define TILEMAP_TILE_START_SYMBOL 0x21 // Refers to ASCII exclamation mark. First visible character on the chart. To debug.
#define TILESHEET_TILE_SYMBOL_STR_LEN 2
#define MAX_TILEMAP_SPRITES 50
#define MAX_TILEMAP_FILENAME_LEN 20

#define MAX_WORLDMAP_LOCATIONS 22
#define WORLDMAP_LOC_PIN_SIZE 32 // Also check the SHEET_ID_ICON_ATLAS frame sizes.
#define WORLDMAP_LOC_PIN_SIZE_DIV2 WORLDMAP_LOC_PIN_SIZE * .5f // Needed?
#define WORLDMAP_MAINMENU_MAP 0

#define FCLAMP(value, min, max) ((value <= min) ? min : (value >= max) ? max : value)
#define FMAX(v1, v2) (v1 >= v2) ? v1 : v2
#define FMIN(v1, v2) (v1 <= v2) ? v1 : v2
#define FABS(v1) (v1 < 0) ? v1*(-1) : v1

#include <vector>

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
#define INVALID_IDI32 I32_MAX
#define INVALID_IDU32 U32_MAX
#define INVALID_IDU16 U16_MAX

typedef enum aspect_ratio {
  ASPECT_RATIO_UNDEFINED,
  ASPECT_RATIO_3_2,
  ASPECT_RATIO_4_3,
  ASPECT_RATIO_5_3,
  ASPECT_RATIO_5_4,
  ASPECT_RATIO_8_5,
  ASPECT_RATIO_16_9,
  ASPECT_RATIO_16_10,
  ASPECT_RATIO_21_9,
  ASPECT_RATIO_32_9,
  ASPECT_RATIO_37_27,
  ASPECT_RATIO_43_18,
  ASPECT_RATIO_64_27,
  ASPECT_RATIO_CUSTOM,
  ASPECT_RATIO_MAX,
} aspect_ratio;

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

typedef union data64 {
  i64 i64;
  u64 u64;
  f64 f64;
 
  i32 i32[2];
  u32 u32[2];
  f32 f32[2];
 
  i16 i16[4];
  u16 u16[4];

  i8 i8[8];
  u8 u8[8];
  char c[8];

  void* address;

  data64(void) {
    this->u64 = 0u;
  }
  
  data64(::i64 value1) {
    this->i64 = value1;
  }
  data64(::u64 value1){
    this->u64 = value1;
  }
  data64(::f64 value1){
    this->f64 = value1;
  }
  data64(::i32 value1, ::i32 value2 = 0){
    this->i32[0] = value1;
    this->i32[1] = value2;
  }
  data64(::u32 value1, ::u32 value2 = 0){
    this->u32[0] = value1;
    this->u32[1] = value2;
  }
  data64(::f32 value1, ::f32 value2 = 0){
    this->f32[0] = value1;
    this->f32[1] = value2;
  }
  data64(::u16 value1, ::u16 value2 = 0, ::u16 value3 = 0, ::u16 value4 = 0){
    this->u16[0] = value1;
    this->u16[1] = value2;
    this->u16[2] = value3;
    this->u16[3] = value4;
  }
  data64(::i16 value1, ::i16 value2 = 0, ::i16 value3 = 0, ::i16 value4 = 0){
    this->i16[0] = value1;
    this->i16[1] = value2;
    this->i16[2] = value3;
    this->i16[3] = value4;
  }
  data64(::u16* value, ::u16 len){
    if (len < 4) {
      for (int i=0; i<len; ++i) {
        this->u16[i] = value[i];
      }
    }
  }
  data64(::i16* value, ::u16 len){
    if (len < 4) {
      for (int i=0; i<len; ++i) {
        this->i16[i] = value[i];
      }
    }
  }
  data64(::u8* value, ::u16 len){
    if (len < 8) {
      for (int i=0; i<len; ++i) {
        this->u8[i] = value[i];
      }
    }
  }
  data64(::i8* value, ::u16 len){
    if (len < 8) {
      for (int i=0; i<len; ++i) {
        this->i8[i] = value[i];
      }
    }
  }
  data64(char* value, ::u16 len){
    if (len < 8) {
      for (int i=0; i<len; ++i) {
        this->c[i] = value[i];
      }
      this->c[len-1] = '\0';
    }
  }
  data64(const char* value, ::u16 len){
    if (len < 8) {
      for (int i=0; i<len; ++i) {
        this->c[i] = value[i];
      }
      this->c[len-1] = '\0';
    }
  }
  data64(void* address1){
    this->address = address1;
  }
} data64;

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

  void* address[2];

  data128(void) {
    this->u64[0] = 0u;
    this->u64[1] = 0u;
  }
  
  data128(::i64 value1, ::i64 value2 = 0) {
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
    if (len < 8) {
      for (int i=0; i<len; ++i) {
        this->u16[i] = value[i];
      }
    }
  }
  data128(::i16* value, ::u16 len){
    if (len < 8) {
      for (int i=0; i<len; ++i) {
        this->i16[i] = value[i];
      }
    }
  }
  data128(::u8* value, ::u16 len){
    if (len < 16) {
      for (int i=0; i<len; ++i) {
        this->u8[i] = value[i];
      }
    }
  }
  data128(::i8* value, ::u16 len){
    if (len < 16) {
      for (int i=0; i<len; ++i) {
        this->i8[i] = value[i];
      }
    }
  }
  data128(char* value, ::u16 len){
    if (len < 16) {
      for (int i=0; i<len; ++i) {
        this->c[i] = value[i];
      }
      this->c[len-1] = '\0';
    }
  }
  data128(const char* value, ::u16 len){
    if (len < 16) {
      for (int i=0; i<len; ++i) {
        this->c[i] = value[i];
      }
      this->c[len-1] = '\0';
    }
  }
  data128(void* address1, void* address2 = nullptr){
    this->address[0] = address1;
    this->address[1] = address2;
  }
} data128;

  // 256 bytes
typedef union data256 {
  i64 i64[4];
  u64 u64[4];
  f64 f64[4];
 
  i32 i32[8];
  u32 u32[8];
  f32 f32[8];
 
  i16 i16[16];
  u16 u16[16];

  i8 i8[32];
  u8 u8[32];

  char c[32];
  void* address[4];

  data256(void){
    this->u64[0] = 0u;
    this->u64[1] = 0u;
    this->u64[2] = 0u;
    this->u64[3] = 0u;
  }
  
  data256(::i64 value1, ::i64 value2 = 0, ::i64 value3 = 0, ::i64 value4 = 0){
    this->i64[0] = value1;
    this->i64[1] = value2;
    this->i64[2] = value3;
    this->i64[3] = value4;
  }
  data256(::u64 value1, ::u64 value2 = 0, ::u64 value3 = 0, ::u64 value4 = 0){
    this->u64[0] = value1;
    this->u64[1] = value2;
    this->u64[2] = value3;
    this->u64[3] = value4;
  }
  data256(::f64 value1, ::f64 value2 = 0, ::f64 value3 = 0, ::f64 value4 = 0){
    this->f64[0] = value1;
    this->f64[1] = value2;
    this->f64[2] = value3;
    this->f64[3] = value4;
  }
  data256(::i32 value1, ::i32 value2 = 0, ::i32 value3 = 0, ::i32 value4 = 0, ::i32 value5 = 0, ::i32 value6 = 0, ::i32 value7 = 0, ::i32 value8 = 0){
    this->i32[0] = value1;
    this->i32[1] = value2;
    this->i32[2] = value3;
    this->i32[3] = value4;
    this->i32[4] = value5;
    this->i32[5] = value6;
    this->i32[6] = value7;
    this->i32[7] = value8;
  }
  data256(::u32 value1, ::u32 value2 = 0, ::u32 value3 = 0, ::u32 value4 = 0, ::u32 value5 = 0, ::u32 value6 = 0, ::u32 value7 = 0, ::u32 value8 = 0){
    this->u32[0] = value1;
    this->u32[1] = value2;
    this->u32[2] = value3;
    this->u32[3] = value4;
    this->u32[4] = value5;
    this->u32[5] = value6;
    this->u32[6] = value7;
    this->u32[7] = value8;
  }
  data256(::f32 value1, ::f32 value2 = 0, ::f32 value3 = 0, ::f32 value4 = 0, ::f32 value5 = 0, ::f32 value6 = 0, ::f32 value7 = 0, ::f32 value8 = 0){
    this->f32[0] = value1;
    this->f32[1] = value2;
    this->f32[2] = value3;
    this->f32[3] = value4;
    this->f32[4] = value5;
    this->f32[5] = value6;
    this->f32[6] = value7;
    this->f32[7] = value8;
  }
  data256(::u16* value, ::u16 len){
    if (len < 16) {
      for (int i=0; i<len; ++i) {
        this->u16[i] = value[i];
      }
    }
  }
  data256(::i16* value, ::u16 len){
    if (len < 16) {
      for (int i=0; i<len; ++i) {
        this->i16[i] = value[i];
      }
    }
  }
  data256(::u8* value, ::u16 len){
    if (len < 32) {
      for (int i=0; i<len; ++i) {
        this->u8[i] = value[i];
      }
    }
  }
  data256(::i8* value, ::u16 len){
    if (len < 32) {
      for (int i=0; i<len; ++i) {
        this->i8[i] = value[i];
      }
    }
  }
  data256(char* value, ::u16 len){
    if (len < 32) {
      for (int i=0; i<len; ++i) {
        this->c[i] = value[i];
      }
      this->c[len-1] = '\0';
    }
  }
  data256(const char* value, ::u16 len){
    if (len < 32) {
      for (int i=0; i<len; ++i) {
        this->c[i] = value[i];
      }
      this->c[len-1] = '\0';
    }
  }
  data256(void* address1, void* address2 = nullptr, void* address3 = nullptr, void* address4 = nullptr){
    this->address[0] = address1;
    this->address[1] = address2;
    this->address[2] = address3;
    this->address[3] = address4;
  }
} data256;

typedef struct data_pack {
  data_type type_flag;
  i32 array_lenght;
  data128 data;
  data_pack() {
    this->type_flag = DATA_TYPE_UNRESERVED;
    this->array_lenght = 0;
    this->data = data128();
  };
  data_pack(data_type type, data128 buffer, i32 len) : data_pack() {
    this->type_flag = type;
    this->array_lenght = len; 
    this->data = buffer;
  };
} data_pack;

typedef enum language_index {
  LANGUAGE_INDEX_UNDEFINED,
  LANGUAGE_INDEX_ENGLISH,
  LANGUAGE_INDEX_TURKISH,
  LANGUAGE_INDEX_BUILTIN,
  LANGUAGE_INDEX_MAX,
} language_index;

typedef enum pak_id {
  PAK_FILE_UNDEFINED,
  PAK_FILE_ASSET1,
  PAK_FILE_ASSET2,
  PAK_FILE_MAP,
  PAK_FILE_MAX,
}pak_file_id;

typedef enum asset1_file_id {
  PAK_FILE_ASSET1_UNDEFINED,
  PAK_FILE_ASSET1_THUMBNAIL,
  PAK_FILE_ASSET1_FONT_ABRACADABRA,
  PAK_FILE_ASSET1_BLACK_BACKGROUND_IMAGE1,
  PAK_FILE_ASSET1_BLACK_BACKGROUND_IMAGE2,
  PAK_FILE_ASSET1_SOUND_BTN_CLICK_1,
  PAK_FILE_ASSET1_SOUND_BTN_CLICK_2,
  PAK_FILE_ASSET1_SOUND_BTN_CLICK_3,
  PAK_FILE_ASSET1_COIN_PICKUP,
  PAK_FILE_ASSET1_SOUND_DENY1,
  PAK_FILE_ASSET1_SOUND_DENY2,
  PAK_FILE_ASSET1_EXP_PICKUP,
  PAK_FILE_ASSET1_SOUND_GAME_OVER,
  PAK_FILE_ASSET1_HEALTH_PICKUP,
  PAK_FILE_ASSET1_THEME_INGAME_PLAY_1,
  PAK_FILE_ASSET1_THEME_INGAME_PLAY_2,
  PAK_FILE_ASSET1_THEME_INGAME_PLAY_3,
  PAK_FILE_ASSET1_THEME_INGAME_PLAY_4,
  PAK_FILE_ASSET1_SOUND_LEVEL_UP,
  PAK_FILE_ASSET1_THEME_MAINMENU_1,
  PAK_FILE_ASSET1_THEME_MAINMENU_2,
  PAK_FILE_ASSET1_THEME_MAINMENU_3,
  PAK_FILE_ASSET1_THEME_MAP_SELECTION_1,
  PAK_FILE_ASSET1_THEME_MAP_SELECTION_2,
  PAK_FILE_ASSET1_FONT_MIOSEVKA_LIGHT,
  PAK_FILE_ASSET1_FONT_MIOSEVKA_LIGHT_ITALIC,
  PAK_FILE_ASSET1_WORLDMAP_IMAGE,
  PAK_FILE_ASSET1_SPRITESHEET_ZAP,
  PAK_FILE_ASSET1_SOUND_ZAP_1,
  PAK_FILE_ASSET1_SOUND_ZAP_2,
  PAK_FILE_ASSET1_SOUND_ZAP_3,
  PAK_FILE_ASSET1_SOUND_ZAP_4,
  PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_1,
  PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_2,
  PAK_FILE_ASSET1_SOUND_ZOMBIE_DIE_3,
  PAK_FILE_ASSET1_MAX,
} asset1_file_id;

typedef enum asset2_file_id {
  PAK_FILE_ASSET2_UNDEFINED,
  PAK_FILE_ASSET2_LOC_FILE_ENGLISH,
  PAK_FILE_ASSET2_LOC_FILE_TURKISH,
  PAK_FILE_ASSET2_ATLAS,
  PAK_FILE_ASSET2_FADE_TRANSITION,
  PAK_FILE_ASSET2_FONT_OUTLINE,
  PAK_FILE_ASSET2_MAP_CHOICE_IMAGE,
  PAK_FILE_ASSET2_POST_PROCESS,
  PAK_FILE_ASSET2_PRG_BAR_MASK,
  PAK_FILE_ASSET2_SDR_SPAWN,
  PAK_FILE_ASSET2_SDF_TEXT,
  PAK_FILE_ASSET2_SDR_SPIN,
  PAK_FILE_ASSET2_MAX,
} asset2_file_id;

typedef enum shader_id {
  SHADER_ID_UNSPECIFIED,
  SHADER_ID_PROGRESS_BAR_MASK,
  SHADER_ID_FADE_TRANSITION,
  SHADER_ID_FONT_OUTLINE,
  SHADER_ID_POST_PROCESS,
  SHADER_ID_MAP_CHOICE_IMAGE,
  SHADER_ID_SPAWN,
  SHADER_ID_SDF_TEXT,
  SHADER_ID_CHEST_OPENING_SPIN_TEXT,
  SHADER_ID_MAX,
} shader_id;


typedef enum sound_id {
  SOUND_ID_UNSPECIFIED,
  SOUND_ID_BUTTON_ON_CLICK1,
  SOUND_ID_BUTTON_ON_CLICK2,
  SOUND_ID_BUTTON_ON_CLICK3,
  SOUND_ID_COIN_PICKUP,
  SOUND_ID_DENY1,
  SOUND_ID_DENY2,
  SOUND_ID_EXP_PICKUP,
  SOUND_ID_HEALTH_PICKUP,
  SOUND_ID_LEVEL_UP,
  SOUND_ID_ZAP1,
  SOUND_ID_ZAP2,
  SOUND_ID_ZAP3,
  SOUND_ID_ZAP4,
  SOUND_ID_ZOMBIE_DIE1,
  SOUND_ID_ZOMBIE_DIE2,
  SOUND_ID_ZOMBIE_DIE3,
  SOUND_ID_MAX,
} sound_id;

typedef enum music_id {
  MUSIC_ID_UNSPECIFIED,
  MUSIC_ID_MAINMENU_THEME1,
  MUSIC_ID_MAINMENU_THEME2,
  MUSIC_ID_MAINMENU_THEME3,
  MUSIC_ID_MAP_SELECTION_THEME1,
  MUSIC_ID_MAP_SELECTION_THEME2,
  MUSIC_ID_INGAME_PLAY_THEME1,
  MUSIC_ID_INGAME_PLAY_THEME2,
  MUSIC_ID_INGAME_PLAY_THEME3,
  MUSIC_ID_INGAME_PLAY_THEME4,
  MUSIC_ID_MAX,
} music_id;

typedef enum soundgroup_id {
  SOUNDGROUP_ID_UNDEFINED,
  SOUNDGROUP_ID_BUTTON_ON_CLICK,
  SOUNDGROUP_ID_DENY,
  SOUNDGROUP_ID_ZAP,
  SOUNDGROUP_ID_ZOMBIE_DIE,
  SOUNDGROUP_ID_MAX,
} soundgroup_id;

typedef enum texture_id {
  TEX_ID_UNSPECIFIED,
  TEX_ID_WORLDMAP_WO_CLOUDS,
  TEX_ID_GAME_START_LOADING_SCREEN,
  TEX_ID_ASSET_ATLAS,
  TEX_ID_BLACK_BACKGROUND_IMG1,
  TEX_ID_BLACK_BACKGROUND_IMG2,
  TEX_ID_ZAP,
  TEX_ID_MAX,
} texture_id;

typedef enum atlas_texture_id {
  ATLAS_TEX_ID_UNSPECIFIED,
  ATLAS_TEX_ID_PLAYER_TEXTURE,
  ATLAS_TEX_ID_SPAWN_TEXTURE,
  ATLAS_TEX_ID_MAP_TILESET_TEXTURE,
  ATLAS_TEX_ID_INQUISITOR_PORTRAIT,
  ATLAS_TEX_ID_PORTRAIT_FRAME,
  ATLAS_TEX_ID_ABILITY_SLOT_FRAME,
  ATLAS_TEX_ID_ARROW,
  ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,
  ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL,
  ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_OUTSIDE,
  ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_INSIDE_YELLOW,
  ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_INSIDE_WHITE,
  ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_MIDDLE,
  ATLAS_TEX_ID_DARK_FANTASY_PANEL,
  ATLAS_TEX_ID_DARK_FANTASY_PANEL_SELECTED,
  ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG,
  ATLAS_TEX_ID_HEADER,
  ATLAS_TEX_ID_LITTLE_SHOWCASE,
  ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,
  ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,
  ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE,
  ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR,
  ATLAS_TEX_ID_BG_BLACK,
  ATLAS_TEX_ID_ZOMBIES_SPRITESHEET,
  ATLAS_TEX_ID_UI_ASSET_ATLAS,
  ATLAS_TEX_ID_CURRENCY_COIN_ICON_5000,
  ATLAS_TEX_ID_CURRENCY_COIN_ICON_15000,
  ATLAS_TEX_ID_CURRENCY_COIN_ICON_45000,
  ATLAS_TEX_ID_FOG,
  ATLAS_TEX_ID_PANEL_SCROLL,
  ATLAS_TEX_ID_PANEL_SCROLL_HANDLE,
  ATLAS_TEX_ID_CHEST_BASE,
  ATLAS_TEX_ID_CHEST_LID,
  ATLAS_TEX_ID_MAX,
} atlas_texture_id;

typedef enum spritesheet_id {
  SHEET_ID_SPRITESHEET_UNSPECIFIED,

  //SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT,
  SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT,
  //SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT,
  SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT,
  //SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT,
  SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT,
  //SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT,
  SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT,
  SHEET_ID_PLAYER_ANIMATION_ATTACK_1,
  SHEET_ID_PLAYER_ANIMATION_ATTACK_2,
  SHEET_ID_PLAYER_ANIMATION_ATTACK_3,
  SHEET_ID_PLAYER_ANIMATION_ATTACK_DOWN,
  SHEET_ID_PLAYER_ANIMATION_ATTACK_UP,
  SHEET_ID_PLAYER_ANIMATION_ROLL,
  SHEET_ID_PLAYER_ANIMATION_DASH,

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

  SHEET_ID_SPAWN_EXPLOSION,

  SHEET_ID_MENU_BUTTON,
  SHEET_ID_FLAT_BUTTON,
  SHEET_ID_SLIDER_PERCENT,
  SHEET_ID_SLIDER_OPTION,
  SHEET_ID_SLIDER_LEFT_BUTTON,
  SHEET_ID_SLIDER_RIGHT_BUTTON,
  
  SHEET_ID_FLAME_ENERGY_ANIMATION,
  SHEET_ID_FIREBALL_ANIMATION,
  SHEET_ID_FIREBALL_EXPLOTION_ANIMATION,
  SHEET_ID_ABILITY_FIRETRAIL_START_ANIMATION,
  SHEET_ID_ABILITY_FIRETRAIL_LOOP_ANIMATION,
  SHEET_ID_ABILITY_FIRETRAIL_END_ANIMATION,
  SHEET_ID_ABILITY_CODEX,

  SHEET_ID_LOOT_ITEM_HEALTH,
  SHEET_ID_LOOT_ITEM_COIN,
  SHEET_ID_LOOT_ITEM_EXPERIENCE,
  SHEET_ID_LOOT_ITEM_CHEST,

  // WARN: REST OF THEM MAP SPRITES
  SHEET_ENUM_MAP_SPRITE_START,
  SHEET_ID_ENVIRONMENTAL_PARTICLES,
  SHEET_ID_LIGHT_INSECTS,
  SHEET_ID_CANDLE_FX,
  SHEET_ID_GENERIC_LIGHT,
  // WARN: DO NOT ADD SPRITE HERE

  SHEET_ID_SPRITESHEET_TYPE_MAX
} spritesheet_id;

typedef enum button_type_id {
  BTN_TYPE_UNDEFINED,
  BTN_TYPE_MENU_BUTTON,
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
  BTN_ID_MAINMENU_BUTTON_ENTER_STATE_CHARACTER,
  BTN_ID_MAINMENU_BUTTON_EXIT,
  BTN_ID_MAINMENU_SETTINGS_CANCEL,
  BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_INVENTORY,
  BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_STATS,
  BTN_ID_MAINMENU_STATE_CHARACTER_BACK,
  BTN_ID_MAINMENU_STATE_CHARACTER_BUY_STAT_UPGRADE,
  BTN_ID_MAINMENU_MAP_CHOICE_BACK,
  BTN_ID_MAINMENU_TRAIT_CHOICE_BACK,
  BTN_ID_MAINMENU_TRAIT_CHOICE_ACCEPT,

  BTN_ID_IN_GAME_BUTTON_RETURN_MENU,
  BTN_ID_IN_GAME_BUTTON_CHEST_OPENING_ACCEPT,

  BTN_ID_PAUSEMENU_BUTTON_RESUME,
  BTN_ID_PAUSEMENU_BUTTON_SETTINGS,
  BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_MAIN_MENU,
  BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_DESKTOP,

  BTN_ID_EDITOR_SDR_ACTIVE_STAGE_INDEX_DEC,
  BTN_ID_EDITOR_SDR_ACTIVE_STAGE_INDEX_INC,
  BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_DEC,
  BTN_ID_EDITOR_ACTIVE_TILEMAP_EDIT_LAYER_INC,
  BTN_ID_EDITOR_PROP_TYPE_SLC_SLIDER_LEFT,
  BTN_ID_EDITOR_PROP_TYPE_SLC_SLIDER_RIGHT,
  BTN_ID_EDITOR_PROP_SCALE_SLIDER_LEFT,
  BTN_ID_EDITOR_PROP_SCALE_SLIDER_RIGHT,
  BTN_ID_EDITOR_PROP_ROTATION_SLIDER_LEFT,
  BTN_ID_EDITOR_PROP_ROTATION_SLIDER_RIGHT,
  BTN_ID_EDITOR_PROP_ZINDEX_SLIDER_LEFT,
  BTN_ID_EDITOR_PROP_ZINDEX_SLIDER_RIGHT,

  BTN_ID_SETTINGS_SLIDER_SOUND_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_SOUND_RIGHT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_RES_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_RES_RIGHT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_WIN_MODE_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_WIN_MODE_RIGHT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_LANGUAGE_LEFT_BUTTON,
  BTN_ID_SETTINGS_SLIDER_LANGUAGE_RIGHT_BUTTON,
  BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON,

  BTN_ID_EDITOR_ADD_MAP_COLLISION,

  BTN_ID_MAX
} button_id;

typedef enum checkbox_type_id {
  CHECKBOX_TYPE_UNDEFINED,
  CHECKBOX_TYPE_BUTTON,
  CHECKBOX_TYPE_MAX
} checkbox_type_id;

typedef enum checkbox_id {
  CHECKBOX_ID_UNDEFINED,
  CHECKBOX_ID_IS_PROP_YBASED,
  CHECKBOX_ID_MAX
} checkbox_id;

typedef enum progress_bar_id {
  PRG_BAR_ID_UNDEFINED,
  PRG_BAR_ID_PLAYER_HEALTH,
  PRG_BAR_ID_PLAYER_EXPERIANCE,
  PRG_BAR_ID_BOSS_HEALTH,
  PRG_BAR_ID_MAX
} progress_bar_id;

typedef enum progress_bar_type_id {
  PRG_BAR_TYPE_ID_UNDEFINED,
  PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
  PRG_BAR_TYPE_ID_DARK_FANTASY_BOSSBAR_6_YELLOW_INNER,
  PRG_BAR_TYPE_ID_DARK_FANTASY_BOSSBAR_6_WHITE_INNER,
  PRG_BAR_TYPE_ID_MAX
} progress_bar_type_id;

typedef enum slider_type_id {
  SDR_TYPE_UNDEFINED,
  SDR_TYPE_PERCENT,
  SDR_TYPE_OPTION,
  SDR_TYPE_NUMBER,
  SDR_TYPE_MAX
} slider_type_id;

typedef enum slider_id {
  SDR_ID_UNDEFINED,
  SDR_ID_SETTINGS_SOUND_SLIDER,
  SDR_ID_SETTINGS_RES_SLIDER,
  SDR_ID_SETTINGS_WIN_MODE_SLIDER,
  SDR_ID_SETTINGS_LANGUAGE,
  SDR_ID_EDITOR_MAP_STAGE_SLC_SLIDER,
  SDR_ID_EDITOR_MAP_LAYER_SLC_SLIDER,
  SDR_ID_EDITOR_PROP_TYPE_SLC_SLIDER,
  SDR_ID_EDITOR_PROP_SCALE_SLIDER,
  SDR_ID_EDITOR_PROP_ROTATION_SLIDER,
  SDR_ID_EDITOR_PROP_ZINDEX_SLIDER,
  SDR_ID_MAX
} slider_id;

typedef struct app_settings {
  std::vector<f32> scale_ratio;
  aspect_ratio display_ratio;
  i32 master_sound_volume;
  i32 window_state;
  i32 window_width;
  i32 window_height;
  i32 render_width;
  i32 render_height;
  i32 render_width_div2;
  i32 render_height_div2;
  std::string language;
  app_settings(void) {
    this->scale_ratio = std::vector<f32>();
    this->language = std::string();
    this->master_sound_volume = 0;
    this->window_state = 0;
    this->window_width = 0;
    this->window_height = 0;
    this->render_width = 0;
    this->render_height = 0;
    this->render_width_div2 = 0;
    this->render_height_div2 = 0;
  }
  app_settings(i32 window_state, i32 width, i32 height, i32 volume, std::string language) : app_settings() {
    this->window_width       = width;
    this->window_height      = height;
    this->render_width       = width;
    this->render_height      = height;
    this->render_width_div2  = width * .5f;
    this->render_height_div2 = height * .5f;
    this->scale_ratio.push_back(static_cast<f32>(this->render_width)  / static_cast<f32>(this->window_width));
    this->scale_ratio.push_back(static_cast<f32>(this->render_height) / static_cast<f32>(this->window_height));
    this->master_sound_volume = volume;
    this->language = language;
    this->window_state = window_state;
  }
} app_settings;

typedef struct file_buffer {
  pak_file_id pak_id;
  i32 file_id;
  std::string content;
  std::string file_extension;
  size_t offset;
  bool is_success;

  file_buffer(void) {
    this->pak_id = PAK_FILE_UNDEFINED;
    this->file_id = PAK_FILE_UNDEFINED;
    this->content = std::string();
    this->file_extension = std::string();
    this->offset = 0u;
    this->is_success = false;
  }
  file_buffer(pak_file_id pak_id, i32 file_id, std::string ext) : file_buffer() {
    this->pak_id = pak_id;
    this->file_id = file_id;
    this->file_extension = ext;
  }
} file_buffer;

typedef struct worldmap_stage_file {
  i32 stage_index;
  std::array<std::string, MAX_TILEMAP_LAYERS> layer_data;
  std::string file_collision;
  std::string file_prop;
  size_t pak_offset;
  bool is_success;
  worldmap_stage_file(void) {
    this->stage_index = 0;
    this->layer_data.fill(std::string(""));
    this->file_collision = std::string("");
    this->file_prop = std::string("");
    this->pak_offset = 0u;
    this->is_success = false;
  }
} worldmap_stage_file;

typedef struct asset_pak_file {
  std::string path_to_resource;
  std::string pak_file_name;
  std::string pak_data;
  std::vector<file_buffer> file_buffers;
  bool is_initialized;
  
  asset_pak_file(void) {
    this->file_buffers = std::vector<file_buffer>();
    this->path_to_resource = std::string();
    this->pak_file_name = std::string();
    this->pak_data = std::string();
    this->is_initialized = false;
  }
  asset_pak_file(std::string pak_name, std::string path_to_res, std::vector<file_buffer> file_infos) : asset_pak_file() {
    this->pak_file_name = pak_name;
    this->path_to_resource = path_to_res;
    this->file_buffers = file_infos;
  }
}asset_pak_file;

#ifdef STEAM_CEG
// Steam DRM header file
#include "cegclient.h"
#else
#define Steamworks_InitCEGLibrary() (true)
#define Steamworks_TermCEGLibrary()
#define Steamworks_TestSecret()
#define Steamworks_SelfCheck()
#endif

#endif
