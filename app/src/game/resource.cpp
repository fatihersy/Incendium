#include "resource.h"
#include <tools/pak_parser.h>
#include <core/fmemory.h>

typedef struct resource_system_state {
  std::array<Texture2D, TEX_ID_MAX> textures;
  std::array<atlas_texture, ATLAS_TEX_ID_MAX> atlas_textures;
  std::array<spritesheet, SHEET_ID_SPRITESHEET_TYPE_MAX> sprites;
  std::array<Image, IMAGE_TYPE_MAX> images;
  std::array<tilesheet, TILESHEET_TYPE_MAX> tilesheets;
} resource_system_state;

static resource_system_state *state;

unsigned int load_texture(const char* filename, bool resize, Vector2 new_size, texture_id _id);
void load_texture_from_atlas(atlas_texture_id _id, Rectangle texture_area);
bool load_image(const char *filename, bool resize, Vector2 new_size, image_type type);
void load_spritesheet(texture_id _source_tex, spritesheet_id handle_id, Vector2 offset, u16 _fps, u16 _frame_width, u16 _frame_height, u16 _total_row, u16 _total_col);
void load_tilesheet(tilesheet_type _sheet_sheet_type, atlas_texture_id _atlas_tex_id, u16 _tile_count_x, u16 _tile_count_y, u16 _tile_size);

bool resource_system_initialize(void) {
  if (state) return false;

  state = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "resource::allocate_memory_linear()::State allocation failed");
    return false;
  }

  // NOTE: resource files inside the pak file
  load_texture("atlas.png", false, VECTOR2(0.f, 0.f), TEX_ID_ASSET_ATLAS);
  load_texture("worldmap_wo_clouds.png",       false, VECTOR2(0.f, 0.f), TEX_ID_WORLDMAP_WO_CLOUDS);

  load_texture_from_atlas(ATLAS_TEX_ID_MAP_TILESET_TEXTURE,        Rectangle{   0,   0,  1568, 2016});
  load_texture_from_atlas(ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,  Rectangle{1568, 112,    48,    4});
  load_texture_from_atlas(ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL,   Rectangle{1504, 112,    58,    3});
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,      Rectangle{1504,  32,    48,   48});
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,   Rectangle{1552,  32,    48,   48});
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_PANEL,         Rectangle{1728,   0,    80,   64});
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_PANEL_SELECTED,Rectangle{1808,   0,    80,   64});
  load_texture_from_atlas(ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG,      Rectangle{1888,   0,    48,   44});
  load_texture_from_atlas(ATLAS_TEX_ID_BG_BLACK,                   Rectangle{1680,  32,    48,   48});
  load_texture_from_atlas(ATLAS_TEX_ID_ZOMBIES_SPRITESHEET,        Rectangle{2224,   0,   256,  480});
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE,   Rectangle{1568,1088,    68,   68});
  load_texture_from_atlas(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR,  Rectangle{1648,1088,    28,   28});
  load_texture_from_atlas(ATLAS_TEX_ID_CURRENCY_SOUL_ICON_5000,    Rectangle{1888, 736,    32,   32});
  load_texture_from_atlas(ATLAS_TEX_ID_CURRENCY_SOUL_ICON_15000,   Rectangle{1920, 736,    32,   32});
  load_texture_from_atlas(ATLAS_TEX_ID_CURRENCY_SOUL_ICON_45000,   Rectangle{1952, 736,    32,   32});
  load_texture_from_atlas(ATLAS_TEX_ID_FOG,                        Rectangle{1152,3296,   400,   64});
  load_texture_from_atlas(ATLAS_TEX_ID_HEADER,                     Rectangle{1824,  64,   192,   32});
  load_texture_from_atlas(ATLAS_TEX_ID_LITTLE_SHOWCASE,            Rectangle{1592,  16,    88,   16});

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT,         VECTOR2(1584, 288), 15,   22,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT,        VECTOR2(1584, 224), 15,   22,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT,         VECTOR2(1584, 320), 10,   22,  32, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT,        VECTOR2(1584, 160), 10,   22,  32, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1584, 256), 13,   22,  32, 1,  3);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1584, 192), 13,   22,  32, 1,  3);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT,        VECTOR2(1632, 128),  9,   90, 110, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT,       VECTOR2(1632,  96),  9,   90, 110, 1,  7);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_LEFT,          VECTOR2(2032,  16),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_MOVE_RIGHT,         VECTOR2(2032, 176),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,   VECTOR2(1776, 176), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_BROWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,  VECTOR2(1936, 176), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_LEFT,         VECTOR2(2032,  56),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_MOVE_RIGHT,        VECTOR2(2032, 216),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1776, 216), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ORANGE_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1936, 216), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_LEFT,         VECTOR2(2032,  96),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_MOVE_RIGHT,        VECTOR2(2032, 256),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1776, 256), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_YELLOW_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1936, 256), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_LEFT,            VECTOR2(2032, 136),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_MOVE_RIGHT,           VECTOR2(2032, 296),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,     VECTOR2(1776, 296), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_RED_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT,    VECTOR2(1936, 296), 15,   32,  40, 1,  4);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_OPTION,                      VECTOR2(1632,  32),  0,  16,  10, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_LEFT_BUTTON,                 VECTOR2(1600,  32),  0,  12,  12, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_RIGHT_BUTTON,                VECTOR2(1600,  44),  0,  12,  12, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_MENU_BUTTON,                        VECTOR2(1504,   0),  0,  88,  13, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_FLAT_BUTTON,                        VECTOR2(1504,  16),  0,  88,  13, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_PERCENT,                     VECTOR2(1600,  64),  0,   9,   9, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_FLAME_ENERGY_ANIMATION,             VECTOR2(1584, 352), 13,  48,  48, 1, 18);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_FIREBALL_ANIMATION,                 VECTOR2(1584, 400), 10,  64,  64, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_FIREBALL_EXPLOTION_ANIMATION,       VECTOR2(1584, 464),  6,  64,  64, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_ABILITY_FIRETRAIL_START_ANIMATION,  VECTOR2(1584, 528), 15,  24,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_ABILITY_FIRETRAIL_LOOP_ANIMATION,   VECTOR2(1584, 560), 10,  24,  32, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_ABILITY_FIRETRAIL_END_ANIMATION,    VECTOR2(1680, 528), 15,  24,  32, 1,  5);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_ENVIRONMENTAL_PARTICLES, VECTOR2( 576, 3456), 5, 160,  96, 1, 12);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_LIGHT_INSECTS,           VECTOR2(1248, 2400), 5,  58,  71, 1, 12);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_CANDLE_FX,               VECTOR2(1088, 3360), 5,  64,  64, 1, 12);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_GENERIC_LIGHT,           VECTOR2(1248, 2272),15, 128, 128, 1,  4);
  
  load_tilesheet(TILESHEET_TYPE_MAP, ATLAS_TEX_ID_MAP_TILESET_TEXTURE, 49, 63, 32);
  return true;
}

const atlas_texture* get_atlas_texture_by_enum(atlas_texture_id _id) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_atlas_texture_by_enum()::Texture type out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->atlas_textures.at(_id));
}
const Texture2D* get_texture_by_enum(texture_id _id) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_texture_by_enum()::Texture type out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->textures.at(_id));
}
const Image* get_image_by_enum(image_type type) {
  if (type >= IMAGE_TYPE_MAX || type <= IMAGE_TYPE_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_image_by_enum()::Image type out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->images.at(type));
}
const spritesheet* get_spritesheet_by_enum(spritesheet_id type) {
  if (type >= SHEET_ID_SPRITESHEET_TYPE_MAX || type <= SHEET_ID_SPRITESHEET_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_spritesheet_by_enum()::Spritesheet type out of bound");
    return nullptr;
  }
  return __builtin_addressof(state->sprites.at(type));
}
const tilesheet* get_tilesheet_by_enum(const tilesheet_type type) {
  if (type >= TILESHEET_TYPE_MAX || type <= TILESHEET_TYPE_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_tilesheet_by_enum()::Tilesheet type out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->tilesheets.at(type));
}
const char *rs_path(const char *filename) {
  return TextFormat("%s%s", RESOURCE_PATH, filename);
}
/**
 * @brief "%s%s%s", RESOURCE_PATH, MAP_PATH, filename
 */
const char *map_layer_path(const char *filename) {
  return TextFormat("%s%s%s", RESOURCE_PATH, MAP_LAYER_PATH, filename);
}
unsigned int load_texture(const char *filename, bool resize, Vector2 new_size, texture_id _id) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) { 
    TraceLog(LOG_ERROR, "resource::load_texture()::texture type out of bound");
    return INVALID_IDU32;
  }
  Texture2D tex;

  #if USE_PAK_FORMAT
    file_data file = get_file_data(filename);
    if (!file.is_initialized) {
      TraceLog(LOG_ERROR, "resource::load_texture()::File:'%s' does not exist", filename);
      return INVALID_ID32;
    }
    Image img = LoadImageFromMemory((const char*)file.file_extension, file.data, file.size);
    if (resize) {
      ImageResize(&img, new_size.x, new_size.y);
    }
    tex = LoadTextureFromImage(img);
  #else
    const char *path = rs_path(filename);
    if (resize) {
      Image img = LoadImage(path);
      ImageResize(&img, new_size.x, new_size.y);
      tex = LoadTextureFromImage(img);
    } else {
      tex = LoadTexture(path);
    }
  #endif

  state->textures.at(_id) = tex;
  return state->textures.at(_id).id;
}
bool load_image(const char *filename, bool resize, Vector2 new_size, image_type type) {
  if (type >= IMAGE_TYPE_MAX || type <= IMAGE_TYPE_UNSPECIFIED) { 
    TraceLog(LOG_ERROR, "resource::load_image()::Image type out of bound");
    return false;
  }
  Image img = {};

  #if USE_PAK_FORMAT
    file_data file = get_file_data(filename);
    if (!file.is_initialized) {
      TraceLog(LOG_ERROR, "resource::load_image()::File:'%s' does not exist", filename);
      return false;
    }
    img = LoadImageFromMemory((const char*)file.file_extension, file.data, file.size);
    if (resize) {
      ImageResize(&img, new_size.x, new_size.y);
    }
  #else
    const char *path = rs_path(filename);
    img = LoadImage(path);
    if (resize) {
      ImageResize(&img, new_size.x, new_size.y);
    }
  #endif

  state->images.at(type) = img;
  return true;
}
void load_spritesheet(texture_id _source_tex, spritesheet_id handle_id, Vector2 offset, u16 _fps, u16 _frame_width, u16 _frame_height, u16 _total_row, u16 _total_col) {
  if ((handle_id >= SHEET_ID_SPRITESHEET_TYPE_MAX  ||  handle_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) ||
      (_source_tex >= TEX_ID_MAX  || _source_tex <= TEX_ID_UNSPECIFIED)) 
  {
    TraceLog(LOG_ERROR, "resource::load_spritesheet()::Out of bound ID recieved");
    return;
  }
  spritesheet _sheet = spritesheet();

  _sheet.sheet_id = handle_id;
  _sheet.tex_id = _source_tex;
  _sheet.tex_handle = __builtin_addressof(state->textures.at(_source_tex));
  _sheet.offset = offset;
  _sheet.is_started = false;
  _sheet.row_total = _total_row;
  _sheet.col_total = _total_col;
  _sheet.frame_total = _total_col * _total_row;
  _sheet.tint = WHITE;
  _sheet.current_col = 0;
  _sheet.current_row = 0;
  _sheet.current_frame = 0;
  _sheet.current_frame_rect = {
      .x = 0.f, .y = 0.f, .width = (f32)  _frame_width, .height = (f32) _frame_height};
  _sheet.coord = {
      .x = 0.f, .y = 0.f, .width = (f32)  _frame_width, .height = (f32) _frame_height};
  _sheet.fps = _fps;

  state->sprites.at(handle_id) = _sheet;
}
void load_tilesheet(tilesheet_type sheet_id, atlas_texture_id _atlas_tex_id, u16 _tile_count_x, u16 _tile_count_y, u16 _tile_size) {
  if ((i32)sheet_id >= TILESHEET_TYPE_MAX || sheet_id <= TILESHEET_TYPE_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "resource::load_tilesheet()::Sheet type out of bound");
    return;
  }
  if (_atlas_tex_id >= ATLAS_TEX_ID_MAX || _atlas_tex_id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR,"resource::load_tilesheet()::texture type out of bound");
  }
  tilesheet* _tilesheet = __builtin_addressof(state->tilesheets.at(sheet_id));

  _tilesheet->atlas_source = state->atlas_textures.at(_atlas_tex_id);
  _tilesheet->sheet_id = sheet_id;
  _tilesheet->atlas_handle = &state->textures.at(ATLAS_TEXTURE_ID);
  _tilesheet->tile_count_x = _tile_count_x;
  _tilesheet->tile_count_y = _tile_count_y;
  _tilesheet->tile_count = _tilesheet->tile_count_x * _tilesheet->tile_count_y;
  _tilesheet->tile_size = _tile_size;

  for (u16 i = 0; i < _tilesheet->tile_count; ++i) {
    u8 x = i % _tilesheet->tile_count_x;
    u8 y = i / _tilesheet->tile_count_x;
    u8 x_symbol = TILEMAP_TILE_START_SYMBOL + x;
    u8 y_symbol = TILEMAP_TILE_START_SYMBOL + y;

    _tilesheet->tile_symbols[x][y] = tile_symbol(x_symbol, y_symbol);
  }
}
void load_texture_from_atlas(atlas_texture_id _id, Rectangle texture_area) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) { 
    TraceLog(LOG_ERROR, "resource::load_texture()::texture type out of bound");
    return;
  }
  atlas_texture tex = atlas_texture();
  tex.source = texture_area;
  tex.atlas_handle = __builtin_addressof(state->textures.at(ATLAS_TEXTURE_ID));

  state->atlas_textures.at(_id) = tex;
  return;
}
