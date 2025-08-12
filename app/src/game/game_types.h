#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "/core/fmemory.h"

#include <string>
#include <array>
#include <raylib.h>

#include "defines.h"

#define pVECTOR2(X) (Vector2{(f32)X[0], (f32)X[1]})
#define VECTOR2(X, Y) (Vector2{X, Y})
#define TO_VECTOR2(X) (Vector2{X, X})
#define NORMALIZE_VEC2(X, Y, X_MAX, Y_MAX)  ( Vector2 {X / X_MAX, Y / Y_MAX})
#define CENTER_RECT(RECT) (Rectangle{RECT.x - RECT.width / 2.f, RECT.y - RECT.height, RECT.width, RECT.height});
#define ZEROVEC2 (Vector2{0.f, 0.f})
#define ZERORECT (Rectangle{0.f, 0.f, 0.f, 0.f})

#define ATLAS_TEXTURE_ID TEX_ID_ASSET_ATLAS

#define WHITE_ROCK CLITERAL(Color) {245, 246, 250,255}
#define CLEAR_BACKGROUND_COLOR BLACK

#define MAX_SHADER_LOCATION_SLOT 16
#define MAX_SHADER_LOCATION_NAME_LENGTH 16

#define UI_FONT_SPACING 1

#define MAX_SLIDER_OPTION_SLOT 16

#define MAX_Z_INDEX_SLOT 10
#define MAX_Y_INDEX_SLOT 10

#define MAX_UPDATE_ABILITY_PANEL_COUNT 3
#define MAX_UPDATE_PASSIVE_PANEL_COUNT 3

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

#define MAX_PLAYER_LEVEL 100
#define MAX_SPAWN_COUNT 100
#define MAX_SPAWN_HEALTH 100000
#define MAX_SPAWN_COLLISIONS 1

#define MAX_ABILITY_LEVEL 7
#define MAX_ABILITY_PROJECTILE_COUNT 32

#define MAX_PASSIVE_UPGRADE_TIER 5
#define MAX_ABILITY_PLAYER_CAN_HAVE_IN_THE_SAME_TIME 6

#define MAX_WORLDMAP_LOCATIONS 22
#define WORLDMAP_LOC_PIN_SIZE 32 // Also check the SHEET_ID_ICON_ATLAS frame sizes.
#define WORLDMAP_LOC_PIN_SIZE_DIV2 WORLDMAP_LOC_PIN_SIZE * .5f // Needed?
#define WORLDMAP_MAINMENU_MAP 0

typedef enum collision_type {
  COLLISION_TYPE_UNDEFINED,
  COLLISION_TYPE_RECTANGLE_RECTANGLE,
  COLLISION_TYPE_CIRCLE_CIRCLE,
  COLLISION_TYPE_CIRCLE_RECTANGLE,
  COLLISION_TYPE_CIRCLE_LINE,
  COLLISION_TYPE_POINT_RECTANGLE,
  COLLISION_TYPE_POINT_CIRCLE,
  COLLISION_TYPE_POINT_TRIANGLE,
  COLLISION_TYPE_POINT_LINE,
  COLLISION_TYPE_POINT_POLY,
  COLLISION_TYPE_MAX,
} collision_type;

typedef enum spawn_type {
  SPAWN_TYPE_UNDEFINED,
  SPAWN_TYPE_BROWN,
  SPAWN_TYPE_ORANGE,
  SPAWN_TYPE_YELLOW,
  SPAWN_TYPE_RED,
  SPAWN_TYPE_BOSS,
  SPAWN_TYPE_MAX
} spawn_type;

typedef enum actor_type {
  ACTOR_TYPE_UNDEFINED,
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
  ABILITY_TYPE_RADIANCE,
  ABILITY_TYPE_COMET,
  ABILITY_TYPE_CODEX,
  ABILITY_TYPE_FIRETRAIL,
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
  CHARACTER_STATS_TOTAL_TRAIT_POINTS,
  CHARACTER_STATS_MAX,
} character_stats;

typedef enum font_type {
  FONT_TYPE_UNDEFINED,
  FONT_TYPE_LIGHT,
  FONT_TYPE_MEDIUM,
  FONT_TYPE_BOLD,
  FONT_TYPE_ITALIC,
  FONT_TYPE_ABRACADABRA,
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

typedef enum tilemap_prop_types {
  TILEMAP_PROP_TYPE_UNDEFINED,
  TILEMAP_PROP_TYPE_TREE,
  TILEMAP_PROP_TYPE_TOMBSTONE,
  TILEMAP_PROP_TYPE_STONE,
  TILEMAP_PROP_TYPE_SPIKE,
  TILEMAP_PROP_TYPE_SKULL,
  TILEMAP_PROP_TYPE_PILLAR,
  TILEMAP_PROP_TYPE_LAMP,
  TILEMAP_PROP_TYPE_FENCE,
  TILEMAP_PROP_TYPE_DETAIL,
  TILEMAP_PROP_TYPE_CANDLE,
  TILEMAP_PROP_TYPE_BUILDING,
  TILEMAP_PROP_TYPE_SPRITE,
  TILEMAP_PROP_TYPE_MAX,
}tilemap_prop_types;

typedef enum ingame_phases {
  INGAME_PHASE_UNDEFINED,
  INGAME_PHASE_CLEAR_ZOMBIES,
  INGAME_PHASE_DEFEAT_BOSS,
  INGAME_PHASE_RESULTS,
  INGAME_PHASE_MAX,
}ingame_phases;

typedef enum text_alignment {
  TEXT_ALIGN_UNDEFINED,
  TEXT_ALIGN_TOP_LEFT,
  TEXT_ALIGN_TOP_CENTER,
  TEXT_ALIGN_TOP_RIGHT,
  TEXT_ALIGN_BOTTOM_LEFT,
  TEXT_ALIGN_BOTTOM_CENTER,
  TEXT_ALIGN_BOTTOM_RIGHT,
  TEXT_ALIGN_LEFT_CENTER,
  TEXT_ALIGN_RIGHT_CENTER,
  TEXT_ALIGN_MAX,
}text_alignment;

typedef enum spawn_movement_animations {
  SPAWN_ZOMBIE_ANIMATION_UNDEFINED,
  SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT,
  SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT,
  SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,
  SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,
  SPAWN_ZOMBIE_ANIMATION_MAX,
} spawn_movement_animations;

typedef enum player_animation_set {
  PLAYER_ANIMATION_UNDEFINED,
  //PLAYER_ANIMATION_MOVE_LEFT,
  PLAYER_ANIMATION_MOVE_RIGHT,
  //PLAYER_ANIMATION_TAKE_DAMAGE_LEFT,
  PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT,
  //PLAYER_ANIMATION_IDLE_LEFT,
  PLAYER_ANIMATION_IDLE_RIGHT,
  //PLAYER_ANIMATION_WRECK_LEFT,
  PLAYER_ANIMATION_WRECK_RIGHT,
  PLAYER_ANIMATION_MAX,
} player_animation_set;

typedef struct spritesheet {
  spritesheet_id sheet_id;
  texture_id tex_id;
  Texture2D* tex_handle;
  Vector2 offset;
  Vector2 origin;
  i32 col_total;
  i32 row_total;
  i32 frame_total;
  i32 current_col;
  i32 current_row;
  i32 current_frame;
  Rectangle current_frame_rect;
  Rectangle coord;

  Color tint;
  f32 rotation;
  i16 fps;
  i16 counter;
  world_direction w_direction;
  spritesheet_playmod playmod;
  bool is_started;
  bool is_played;
  bool play_looped;
  bool play_once;

  spritesheet(void) {
    this->sheet_id = SHEET_ID_SPRITESHEET_UNSPECIFIED;
    this->tex_id = TEX_ID_UNSPECIFIED;
    this->tex_handle = nullptr;
    this->offset = ZEROVEC2;
    this->origin = ZEROVEC2;
    this->col_total = 0;
    this->row_total = 0;
    this->frame_total = 0;
    this->current_col = 0;
    this->current_row = 0;
    this->current_frame = 0;
    this->current_frame_rect = ZERORECT;
    this->coord = ZERORECT;
    this->tint = WHITE;
    this->rotation = 0.f;
    this->fps = 0;
    this->counter = 0;
    this->w_direction = WORLD_DIRECTION_UNDEFINED;
    this->playmod = SPRITESHEET_PLAYMOD_UNSPECIFIED;
    this->is_started = false;
    this->is_played = false;
    this->play_looped = false;
    this->play_once = false;
  }
} spritesheet;

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

typedef struct atlas_texture {
  Texture2D* atlas_handle;
  Rectangle source;
  atlas_texture(void) {
    this->atlas_handle = nullptr;
    this->source = ZERORECT;
  } 
} atlas_texture;

typedef struct tile_symbol {
  u8 c[2];
  tile_symbol(void) {
    zero_memory(this->c, sizeof(u8) * 2);
  }
  tile_symbol(u16 u1, u16 u2) : tile_symbol() {
    this->c[0] = u1;
    this->c[1] = u2;
  }
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

  tilesheet(void) {
    zero_memory(this->tile_symbols, sizeof(tile_symbols) * MAX_TILESHEET_UNIQUE_TILESLOTS_X * MAX_TILESHEET_UNIQUE_TILESLOTS_Y);
    
    this->sheet_id = TILESHEET_TYPE_UNSPECIFIED;
    this->atlas_source = atlas_texture();
    this->atlas_handle = nullptr;
    this->tile_count_x = 0;
    this->tile_count_y = 0;
    this->tile_count = 0;
    this->tile_size = 0;
    this->dest_tile_size = 0;
    this->position = ZEROVEC2;
    this->offset = 0.f;
    this->is_initialized = false;
  }
} tilesheet;

typedef struct worldmap_stage {
  i32 map_id;
  std::string displayname;
  std::string filename;
  std::array<Rectangle, MAX_SPAWN_COLLISIONS> spawning_areas;
  Vector2 screen_location;
  i32 total_spawn_count;
  i32 stage_level;
  f32 spawn_scale;
  f32 boss_scale;
  bool is_centered;
  bool is_active;
  worldmap_stage(void) {
    this->map_id = INVALID_IDI32;
    this->displayname.clear();
    this->filename.clear();
    this->spawning_areas.fill(ZERORECT);
    this->screen_location = ZEROVEC2;
    this->total_spawn_count = 0;
    this->stage_level = 0;
    this->spawn_scale = 0.f;
    this->boss_scale = 0.f;
    this->is_centered = false;
    this->is_active = false;
  }
  worldmap_stage(
    i32 in_id,std::string in_display_name, std::string in_filename, i32 in_stage_level,
    i32 in_spw_count, f32 in_spw_scale, f32 in_boss_scale, 
    Vector2 in_screen_loc, bool in_is_centered, bool in_is_active
  ) : worldmap_stage() {
    this->map_id = in_id;
    this->displayname = in_display_name;
    this->filename = in_filename;
    this->screen_location = NORMALIZE_VEC2(in_screen_loc.x, in_screen_loc.y, 3840.f, 2160.f);
    this->total_spawn_count = in_spw_count;
    this->stage_level = in_stage_level;
    this->spawn_scale = in_spw_scale;
    this->boss_scale = in_boss_scale;
    this->is_centered = in_is_centered;
    this->is_active = in_is_active;
  }
} worldmap_stage;

typedef struct tilemap_prop_static {
  i32 prop_id;
  i32 map_id;
	texture_id tex_id;
  tilemap_prop_types prop_type;
  Rectangle source;
  Rectangle dest;
  i16 zindex;
  f32 rotation;
  f32 scale;
  Color tint;
  Vector2 origin;
  bool is_initialized;
  bool use_y_based_zindex;
  tilemap_prop_static(void) {
    this->map_id = I32_MAX;
    this->prop_id = I32_MAX;
    this->tex_id = TEX_ID_UNSPECIFIED;
    this->prop_type = TILEMAP_PROP_TYPE_UNDEFINED;
    this->source = ZERORECT;
    this->dest = ZERORECT;
    this->zindex = 0;
    this->rotation = 0.f;
    this->scale = 0.f;
    this->origin = ZEROVEC2;
    this->is_initialized = false;
    this->use_y_based_zindex = false;
  }
} tilemap_prop_static;

typedef struct tilemap_prop_sprite {
  i32 prop_id;
  i32 map_id;
  tilemap_prop_types prop_type;
  spritesheet sprite;
  i16 zindex;
  f32 scale;
  bool is_initialized;
  bool use_y_based_zindex;
  tilemap_prop_sprite(void) {
    this->prop_id = I32_MAX;
    this->map_id = I32_MAX;
    this->prop_type = TILEMAP_PROP_TYPE_UNDEFINED;
    this->sprite = spritesheet();
    this->zindex = 0;
    this->scale = 0.f;
    this->is_initialized = false;
    this->use_y_based_zindex = false;
  }
} tilemap_prop_sprite;

typedef struct tilemap_prop_address {
  tilemap_prop_types type;
  union {
    tilemap_prop_static* prop_static;
    tilemap_prop_sprite* prop_sprite;
  } data;
  tilemap_prop_address(void) {
    this->type = TILEMAP_PROP_TYPE_UNDEFINED;
    this->data.prop_sprite = nullptr;
  }
  tilemap_prop_address(tilemap_prop_static* _prop) : tilemap_prop_address() {
    this->data.prop_static = _prop;
    this->type = _prop->prop_type;
  }
  tilemap_prop_address(tilemap_prop_sprite* _prop) : tilemap_prop_address() {
    this->data.prop_sprite = _prop;
    this->type = _prop->prop_type;
  }
}tilemap_prop_address;

typedef struct tile_position {
  u16 layer;
  u16 x;
  u16 y;
  tile_position(void) {
    this->x = 0u;
    this->y = 0u;
    this->layer = 0u;
  }
} tile_position;

typedef struct tile {
  tile_position position;
  tile_symbol symbol;
  bool is_initialized;
  tile(void) {
    this->symbol = tile_symbol();
    this->position = tile_position();
    this->is_initialized = false;
  }
} tile;

typedef struct map_collision {
  i32 coll_id;
  Rectangle dest;
  map_collision(void) {
    this->coll_id = I32_MAX;
    this->dest = ZERORECT;
  }
  map_collision(i32 collision_id, Rectangle dest) : map_collision() {
    this->coll_id = collision_id;
    this->dest = dest;
  }
} map_collision;

typedef struct tilemap {
  std::array<std::string, MAX_TILEMAP_LAYERS> filename;
  std::string propfile;
  std::string collisionfile;
  Vector2 position;
  i32 next_map_id;
  i32 next_collision_id;
  u16 map_dim_total;
  u16 map_dim;
  u16 tile_size;
  tile_symbol tiles[MAX_TILEMAP_LAYERS][MAX_TILEMAP_TILESLOT_X][MAX_TILEMAP_TILESLOT_Y];
  std::vector<tilemap_prop_static> static_props;
  std::vector<tilemap_prop_sprite> sprite_props;
  std::array<std::vector<tilemap_prop_address>, MAX_Z_INDEX_SLOT> render_z_index_queue;
  std::array<std::vector<tilemap_prop_address>, MAX_Y_INDEX_SLOT> render_y_based_queue;
  std::vector<map_collision> collisions;
  bool is_initialized;
} tilemap;

typedef struct tilemap_stringtify_package {
  u8 str_tilemap[MAX_TILEMAP_LAYERS][MAX_TILEMAP_TILESLOT * TILESHEET_TILE_SYMBOL_STR_LEN];
  i32 size_tilemap_str[MAX_TILEMAP_LAYERS];
  std::string str_props;
  std::string str_collisions;
  i32 size_props_str;
  i32 size_collisions_str;
  bool is_success;
  tilemap_stringtify_package(void) {
    zero_memory(this->str_tilemap, sizeof(u8) * MAX_TILEMAP_LAYERS * MAX_TILEMAP_TILESLOT * TILESHEET_TILE_SYMBOL_STR_LEN);
    zero_memory(this->size_tilemap_str, sizeof(i32) * MAX_TILEMAP_LAYERS);
    this->str_props.clear();
    this->size_props_str = 0;
    this->is_success = false;
  }
}tilemap_stringtify_package;

/**
 * @param buffer.i32[0] = SPAWN_TYPE
 * @param buffer.i32[1] = SPAWN_LEVEL
 * @param buffer.i32[2] = SPAWN_RND_SCALE
 */
typedef struct Character2D {
  i32 character_id;
  i32 health_max;
  i32 health_current;
  i32 damage;
  f32 speed;
  actor_type type;
  Rectangle collision;
  world_direction w_direction;
  spritesheet move_right_animation;
  spritesheet move_left_animation;
  spritesheet take_damage_right_animation;
  spritesheet take_damage_left_animation;
  spawn_movement_animations last_played_animation;
  Vector2 position;
  f32 rotation;
  f32 scale;
  f32 damage_break_time;
  bool is_dead;
  bool is_damagable;
  bool initialized;

  data128 buffer;

  Character2D(void) {
    this->character_id = I32_MAX;
    this->health_max = 0;
    this->health_current = 0;
    this->damage = 0;
    this->speed = 0.f;
    this->type = ACTOR_TYPE_UNDEFINED;
    this->collision = ZERORECT;
    this->w_direction = WORLD_DIRECTION_UNDEFINED;
    this->move_right_animation = spritesheet();
    this->move_left_animation = spritesheet();
    this->take_damage_right_animation = spritesheet();
    this->take_damage_left_animation = spritesheet();
    this->last_played_animation = SPAWN_ZOMBIE_ANIMATION_UNDEFINED;
    this->position = ZEROVEC2;
    this->rotation = 0.f;
    this->scale = 0.f;
    this->damage_break_time = 0.f;
    this->is_dead = false;
    this->is_damagable = false;
    this->initialized = false;
    this->buffer = data128();
  }
  Character2D(i32 spawn_type, i32 player_level, i32 rnd_scale, Vector2 position) : Character2D() {
    type = ACTOR_TYPE_SPAWN;
    buffer.i32[0] = spawn_type;
    buffer.i32[1] = player_level;
    buffer.i32[2] = rnd_scale;
    this->position = position;
    is_damagable = true;
  }
  Character2D(Rectangle collision, i32 damage) : Character2D() {
    this->collision = collision;
    this->damage = damage;
    this->initialized = true;
  }
} Character2D;

/**
 * @brief vec_ex buffer summary: {f32[0], f32[1]}, {f32[2], f32[3]} = {target x, target y}, {explosion.x, explosion.y}
 * @brief mm_ex buffer  summary: {u16[0]} = {counter, }
 */
typedef struct projectile {
  u16 id;
  std::vector<spritesheet> animations;
  i16 active_sprite;
  Vector2 position;
  Rectangle collision;
  world_direction direction;

  // 128 byte buffer
  data256 vec_ex;
  data128 mm_ex;

  u16 damage;
  f32 duration;
  bool is_active;

  projectile(void) {
    this->id = 0u;
    this->animations.clear();
    this->active_sprite = -1;
    this->position = ZEROVEC2;
    this->collision = ZERORECT;
    this->direction = WORLD_DIRECTION_UNDEFINED;
    this->vec_ex = data256();
    this->mm_ex = data128();
    this->damage = 0u;
    this->duration = 0.f;
    this->is_active = false;
  }
} projectile;

typedef struct ability {
  ability_type type;
  std::string display_name;
  std::vector<projectile> projectiles;
  std::vector<spritesheet_id> animation_ids;
  void* p_owner;
  std::array<ability_upgradables, ABILITY_UPG_MAX> upgradables;
  
  Vector2 proj_dim;
  Vector2 position;
  f32 ability_cooldown_duration;
  f32 ability_play_time;
  f32 proj_sprite_scale;
  Vector2 proj_collision_scale;
  f32 proj_duration;
  u16 proj_count;
  u16 proj_speed;
  u16 level;
  u16 base_damage;
  u16 rotation;
  Rectangle icon_src;
  bool is_active;
  bool is_initialized;

  data128 mm_ex;
  data256 vec_ex;

  ability(void) {
    this->p_owner = nullptr;
    this->display_name.clear();
    this->projectiles.clear();
    this->upgradables.fill(ABILITY_UPG_UNDEFINED);
    this->animation_ids.clear();
    this->type = ABILITY_TYPE_UNDEFINED;
    this->ability_play_time = 0.f;
    this->proj_sprite_scale = 1.f;
    this->proj_collision_scale = Vector2 { 1.f, 1.f };
    this->proj_count = 0u;
    this->proj_speed = 0u;
    this->proj_duration = 0.f;
    this->proj_dim = ZEROVEC2;
    this->icon_src = ZERORECT;
    this->position = ZEROVEC2;
    this->level = 0u;
    this->base_damage = 0u;
    this->rotation = 0u;
    this->is_active = false;
    this->is_initialized = false;
  };
  ability(
    std::string name, ability_type type, 
    std::array<ability_upgradables, ABILITY_UPG_MAX> upgrs, 
    f32 ability_cooldown, f32 proj_sprite_scale, Vector2 proj_collision_scale, u16 proj_count, u16 proj_speed, f32 proj_duration, u16 base_damage,
    Vector2 proj_dim, Rectangle icon_src) : ability()
  {
    this->display_name = name;
    this->type = type;
    this->upgradables = upgrs;
    this->ability_cooldown_duration = ability_cooldown;
    this->proj_sprite_scale = proj_sprite_scale;
    this->proj_collision_scale = proj_collision_scale;
    this->proj_count = proj_count;
    this->proj_speed = proj_speed;
    this->proj_duration = proj_duration;
    this->base_damage = base_damage;
    this->proj_dim = proj_dim;
    this->icon_src = icon_src;
    this->level = 1;
  }
}ability;

typedef struct ability_play_system {
  std::array<ability, ABILITY_TYPE_MAX> abilities;
  ability_play_system(void) {
    this->abilities.fill(ability());
  }
} ability_play_system;

/**
* @brief buffer[0] : Default value
* @brief buffer[1] : Level value
* @brief buffer[2] : Trait value
* @brief buffer[3] : Total value
*/
typedef struct character_stat {
  character_stats id;
  i32 level;
  i32 passive_display_name_symbol;
  i32 passive_desc_symbol;
  Rectangle passive_icon_src;
  i32 upgrade_cost;

  data128 buffer;

  character_stat(void) {
    this->id = CHARACTER_STATS_UNDEFINED;
    this->level = 0;
    this->passive_display_name_symbol = 0;
    this->passive_desc_symbol = 0;
    this->passive_icon_src = ZERORECT;
    this->upgrade_cost = 0;
    this->buffer = data128();
  }
  character_stat(character_stats id, i32 display_name_symbol, i32 desc_symbol, Rectangle icon_src, i32 upgrade_cost, data128 buffer = data128()) : character_stat() {
    this->id = id;
    this->passive_display_name_symbol = display_name_symbol;
    this->passive_desc_symbol = desc_symbol;
    this->passive_icon_src = icon_src;
    this->upgrade_cost = upgrade_cost;
    this->buffer = buffer;
    this->level = 1;
  }
}character_stat;

typedef struct character_trait {
  i32 id;
  character_stats type;
  std::string title;
  std::string description;
  i32 point;

  data128 ingame_ops;
  data128 ui_ops;

  character_trait(void) {
    this->id = 0;
    this->type = CHARACTER_STATS_UNDEFINED;
    this->title = std::string("");
    this->description = std::string("");
    this->ingame_ops = data128();
    this->ui_ops = data128();
    this->point = 0;
  }
  character_trait(i32 _id, character_stats _type, const char* _title, const char * _description, i32 _point, data128 buffer) : character_trait() {
    this->id = _id;
    this->type = _type;
    this->title = _title;
    this->description = _description;
    this->point = _point;
    this->ingame_ops = buffer;
  }
}character_trait;

// LABEL: Player State
typedef struct player_state {
  ability_play_system ability_system;
  std::array<character_stat, CHARACTER_STATS_MAX> stats;
  ability_type starter_ability;
  
  Rectangle collision;
  Rectangle map_level_collision;

  spritesheet move_right_sprite;
  spritesheet idle_right_sprite;
  spritesheet take_damage_right_sprite;
  spritesheet wreck_right_sprite;
  spritesheet attack_1_sprite;
  spritesheet attack_2_sprite;
  spritesheet attack_3_sprite;
  spritesheet attack_down_sprite;
  spritesheet attack_up_sprite;
  spritesheet roll_sprite;
  spritesheet dash_sprite;
  spritesheet current_anim_to_play;

  world_direction w_direction;
  Vector2 position;
  f32 damage_break_time;
  f32 damage_break_current;
  i32 exp_to_next_level;
  i32 exp_current;
  f32 exp_perc;
  f32 health_perc;
  i32 health_current;

  i32 level;

  bool is_player_have_ability_upgrade_points;
  bool is_initialized;
  bool is_moving;
  bool is_dead;
  bool is_damagable;
  player_state(void) {
    this->ability_system = ability_play_system();
    this->stats.fill(character_stat());
    this->starter_ability = ABILITY_TYPE_UNDEFINED;
    this->collision = ZERORECT;
    this->map_level_collision = ZERORECT;
    this->move_right_sprite = spritesheet();
    this->idle_right_sprite = spritesheet();
    this->take_damage_right_sprite = spritesheet();
    this->wreck_right_sprite = spritesheet();
    this->attack_1_sprite = spritesheet();
    this->attack_2_sprite = spritesheet();
    this->attack_3_sprite = spritesheet();
    this->attack_down_sprite = spritesheet();
    this->attack_up_sprite = spritesheet();
    this->roll_sprite = spritesheet();
    this->dash_sprite = spritesheet();
    this->current_anim_to_play = spritesheet();
    this->w_direction = WORLD_DIRECTION_UNDEFINED;
    this->position = ZEROVEC2;
    this->damage_break_time = 0.f;
    this->damage_break_current = 0.f;
    this->exp_to_next_level = 0.f;
    this->exp_current = 0.f;
    this->exp_perc = 0.f;
    this->health_perc = 0.f;
    this->health_current = 0.f;;
    this->level = 0;
    this->is_player_have_ability_upgrade_points = false;
    this->is_initialized = false;
    this->is_moving = false;
    this->is_dead = false;
    this->is_damagable = false;
  }
} player_state;

typedef struct ingame_info {
  player_state* player_state_dynamic;
  const std::vector<Character2D>* in_spawns;
  const Character2D* nearest_spawn;
  const Vector2* mouse_pos_world;
  const Vector2* mouse_pos_screen;
  const ingame_phases* ingame_phase;
  std::vector<character_trait>* chosen_traits;

  i32 collected_souls;
  f32 play_time;
  bool is_win;
  i32 stage_boss_id;

  ingame_info(void) {
    this->in_spawns = nullptr;
    this->nearest_spawn = nullptr;
    this->player_state_dynamic = nullptr;
    this->mouse_pos_world = nullptr;
    this->mouse_pos_screen = nullptr;
    this->ingame_phase = nullptr;
    this->chosen_traits = nullptr;
    
    this->collected_souls = 0;
    this->play_time = 0.f;
    this->is_win = false;
    this->stage_boss_id = -1;
  }
} ingame_info;

typedef struct camera_metrics {
  Camera2D handle;
  Rectangle frustum;
  Vector2 screen_offset;
  camera_metrics(void) {
    this->handle = Camera2D {ZEROVEC2, ZEROVEC2, 0.f, 0.f}; 
    this->frustum = ZERORECT;
    this->screen_offset = ZEROVEC2;
  }
} camera_metrics;

typedef struct localization_package {
  std::string language_name;
  u32 language_index;
  i32 * codepoints;
  Font light_font;
  Font bold_font;
  Font medium_font;
  Font italic_font;
  Font abracadabra;
  localization_package(void) {
    this->language_name.clear();
    this->language_index = INVALID_IDU32;
    this->codepoints = nullptr;
    this->light_font = {};
    this->bold_font = {};
    this->medium_font = {};
    this->italic_font = {};
    this->abracadabra = {};
  }
} localization_package;

typedef struct text_spec {
  std::string text;
  Vector2 text_pos;
  Font* font;
  i32 font_size;
  Color color;
  u32 language_index;

  text_spec(void) {
    this->text = "NULL";
    this->text_pos = VECTOR2(960,540);
    this->font = nullptr;
    this->font_size = 32;
    this->color = Color{0,0,0,255};
    this->language_index = 0u;
  }
  text_spec(std::string _display_text, Vector2 _text_pos, Font* _font, i32 _font_size, Color _color, u32 _language_index) : text_spec() {
    this->text = _display_text;
    this->text_pos = _text_pos;
    this->font = _font;
    this->font_size = _font_size;
    this->color = _color;
    this->language_index = _language_index;
  }
}text_spec;

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

#endif
