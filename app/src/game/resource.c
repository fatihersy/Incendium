#include "resource.h"
#include <tools/pak_parser.h>
#include <core/fmemory.h>

typedef struct resource_system_state {
  u16 texture_amouth;
  u16 sprite_amouth;
  u16 image_amouth;
  u16 tilesheet_amouth;

  Texture2D textures[TEX_ID_MAX];
  atlas_texture atlas_textures[ATLAS_TEX_ID_MAX];
  spritesheet sprites[SHEET_ID_SPRITESHEET_TYPE_MAX];
  Image images[IMAGE_TYPE_MAX];
  tilesheet tilesheets[TILESHEET_TYPE_MAX];

  scene_id game_on_scene;
} resource_system_state;

static resource_system_state *state;

unsigned int load_texture(const char* filename, bool resize, Vector2 new_size, texture_id _id);
unsigned int load_texture_from_atlas(atlas_texture_id _id, Rectangle texture_area);
bool load_image(const char *filename, bool resize, Vector2 new_size, image_type type);
void load_spritesheet(texture_id _source_tex, spritesheet_id handle_id, Vector2 offset, u16 _fps, u16 _frame_width, u16 _frame_height, u16 _total_row, u16 _total_col);
void load_tilesheet(tilesheet_type _sheet_sheet_type, atlas_texture_id _atlas_tex_id, u16 _tile_count_x, u16 _tile_count_y, u16 _tile_size);

bool resource_system_initialize(void) {
  if (state) return false;

  state = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

  // NOTE: resource files inside the pak file
  load_texture("atlas.png", false, (Vector2){ 0, 0}, TEX_ID_ASSET_ATLAS);
  load_texture("worldmap_wo_clouds.png",       false, (Vector2){ 0,  0}, TEX_ID_WORLDMAP_WO_CLOUDS);

  load_texture_from_atlas(ATLAS_TEX_ID_MAP_TILESET_TEXTURE,       (Rectangle){   0,   0,  1568, 2016});
  load_texture_from_atlas(ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL, (Rectangle){1568, 112,    64,    9});
  load_texture_from_atlas(ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL,  (Rectangle){1504, 112,    64,    9});
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL,     (Rectangle){1504,  32,    48,   48});
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_PANEL_BG,  (Rectangle){1552,  32,    48,   48});
  load_texture_from_atlas(ATLAS_TEX_ID_MAP_PROPS_ATLAS,           (Rectangle){   0,2016,  1248, 1632});
  load_texture_from_atlas(ATLAS_TEX_ID_ICON_ATLAS,                (Rectangle){1568, 608,  1024, 1024});
  load_texture_from_atlas(ATLAS_TEX_ID_BG_BLACK,                  (Rectangle){1680,  32,    48,   48});
  load_texture_from_atlas(ATLAS_TEX_ID_ZOMBIES_SPRITESHEET,       (Rectangle){2224,   0,   256,  480});
  load_texture_from_atlas(ATLAS_TEX_ID_CRIMSON_FANTASY_SHOWCASE,  (Rectangle){1568,1088,    68,   68});
  load_texture_from_atlas(ATLAS_TEX_ID_PASSIVE_UPGRADE_TIER_STAR, (Rectangle){1648,1088,    28,   28});

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_IDLE_LEFT,         VECTOR2(1584, 288), 15,   22,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_IDLE_RIGHT,        VECTOR2(1584, 224), 15,   22,  32, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_MOVE_LEFT,         VECTOR2(1584, 320), 10,   22,  32, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_MOVE_RIGHT,        VECTOR2(1584, 160), 10,   22,  32, 1,  6);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1584, 256), 13,   22,  32, 1,  3);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1584, 192), 13,   22,  32, 1,  3);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_WRECK_LEFT,        VECTOR2(1632, 128),  9,   90, 110, 1,  7);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_PLAYER_ANIMATION_WRECK_RIGHT,       VECTOR2(1632,  96),  9,   90, 110, 1,  7);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_LEFT,         VECTOR2(2032,  16),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ZOMBIE_ANIMATION_MOVE_RIGHT,        VECTOR2(2032, 176),  8,   32,  40, 1,  8);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_LEFT,  VECTOR2(1776, 176), 15,   32,  40, 1,  4);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SPAWN_ZOMBIE_ANIMATION_TAKE_DAMAGE_RIGHT, VECTOR2(1936, 176), 15,   32,  40, 1,  4);

  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_BUTTON_REFLECTION_SHEET,            VECTOR2(1504,   0), 30,   80,  16, 1,  9);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_PERCENT,                     VECTOR2(1664,  16),  0,   20,  10, 1, 11);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_OPTION,                      VECTOR2(1632,  32),  0,   21,  10, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_LEFT_BUTTON,                 VECTOR2(1600,  32),  0,   10,  10, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_SLIDER_RIGHT_BUTTON,                VECTOR2(1610,  42),  0,   10,  10, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_MENU_BUTTON,                        VECTOR2(1504,  16),  0,   80,  16, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_FLAT_BUTTON,                        VECTOR2(1504,  96),  0,   44,  14, 1,  2);
  load_spritesheet(TEX_ID_ASSET_ATLAS, SHEET_ID_FIREBALL_ANIMATION,                 VECTOR2(1584, 352),  10,   48,  48, 1, 18);
  
  load_tilesheet(TILESHEET_TYPE_MAP, ATLAS_TEX_ID_MAP_TILESET_TEXTURE, 49, 63, 32);
  return true;
}

atlas_texture* get_atlas_texture_by_enum(atlas_texture_id _id) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_atlas_texture_by_enum()::Texture type out of bound");
    return 0;
  }

  return &state->atlas_textures[_id];
}
Texture2D* get_texture_by_enum(texture_id _id) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_texture_by_enum()::Texture type out of bound");
    return 0;
  }

  return &state->textures[_id];
}
Image* get_image_by_enum(image_type type) {
  if (type >= IMAGE_TYPE_MAX || type <= IMAGE_TYPE_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_image_by_enum()::Image type out of bound");
    return 0;
  }

  return &state->images[type];
}
spritesheet* get_spritesheet_by_enum(spritesheet_id type) {
  if (type >= SHEET_ID_SPRITESHEET_TYPE_MAX || type <= SHEET_ID_SPRITESHEET_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_spritesheet_by_enum()::Spritesheet type out of bound");
    return 0;
  }
  return &state->sprites[type];
}
tilesheet* get_tilesheet_by_enum(tilesheet_type type) {
  if (type >= TILESHEET_TYPE_MAX || type <= TILESHEET_TYPE_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_tilesheet_by_enum()::Tilesheet type out of bound");
    return 0;
  }

  return &state->tilesheets[type];
}
const char *rs_path(const char *filename) {
  return TextFormat("%s%s", RESOURCE_PATH, filename);
}
unsigned int load_texture(const char *filename, bool resize, Vector2 new_size, texture_id _id) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) { 
    TraceLog(LOG_ERROR, "resource::load_texture()::texture type out of bound");
    return INVALID_ID32;
  }
  Texture2D tex = {0};

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

  state->texture_amouth++;
  state->textures[_id] = tex;
  return state->textures[_id].id;
}
bool load_image(const char *filename, bool resize, Vector2 new_size, image_type type) {
  if (type >= IMAGE_TYPE_MAX || type <= IMAGE_TYPE_UNSPECIFIED) { 
    TraceLog(LOG_ERROR, "resource::load_image()::Image type out of bound");
    return false;
  }
  Image img = {0};

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

  state->image_amouth++;
  state->images[type] = img;
  return true;
}
void load_spritesheet(texture_id _source_tex, spritesheet_id handle_id, Vector2 offset, u16 _fps, u16 _frame_width, u16 _frame_height, u16 _total_row, u16 _total_col) {
  if ((handle_id >= MAX_SPRITESHEET_SLOTS  ||  handle_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) ||
      (_source_tex >= TEX_ID_MAX  || _source_tex <= TEX_ID_UNSPECIFIED)) 
  {
    TraceLog(LOG_ERROR, "resource::load_spritesheet()::Out of bound ID recieved");
    return;
  }
  spritesheet _sheet = {0};

  _sheet.sheet_id = handle_id;
  _sheet.tex_id = _source_tex;
  _sheet.tex_handle = &state->textures[_source_tex];
  _sheet.offset = offset;
  _sheet.is_started = false;
  _sheet.row_total = _total_row;
  _sheet.col_total = _total_col;
  _sheet.frame_total = _total_col * _total_row;
  _sheet.current_col = 0;
  _sheet.current_row = 0;
  _sheet.current_frame = 0;
  _sheet.attached_spawn = 0;
  _sheet.current_frame_rect = (Rectangle){
      .x = 0, .y = 0, .width = _frame_width, .height = _frame_height};
  _sheet.coord = (Rectangle){
      .x = 0, .y = 0, .width = _frame_width, .height = _frame_height};
  _sheet.fps = _fps;

  state->sprite_amouth++;
  state->sprites[handle_id] = _sheet;
}
void load_tilesheet(tilesheet_type sheet_id, atlas_texture_id _atlas_tex_id, u16 _tile_count_x, u16 _tile_count_y, u16 _tile_size) {
  if (sheet_id >= MAX_TILESHEET_SLOTS || sheet_id <= 0) {
    TraceLog(LOG_ERROR, "resource::load_tilesheet()::Sheet type out of bound");
    return;
  }
  if (_atlas_tex_id >= ATLAS_TEX_ID_MAX || _atlas_tex_id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR,"resource::load_tilesheet()::texture type out of bound");
  }
  tilesheet* _tilesheet = &state->tilesheets[sheet_id];

  _tilesheet->atlas_source = state->atlas_textures[_atlas_tex_id];
  _tilesheet->sheet_id = sheet_id;
  _tilesheet->atlas_handle = &state->textures[ATLAS_TEXTURE_ID];
  _tilesheet->tile_count_x = _tile_count_x;
  _tilesheet->tile_count_y = _tile_count_y;
  _tilesheet->tile_count = _tilesheet->tile_count_x * _tilesheet->tile_count_y;
  _tilesheet->tile_size = _tile_size;

  for (u16 i = 0; i < _tilesheet->tile_count; ++i) {
    u8 x = i % _tilesheet->tile_count_x;
    u8 y = i / _tilesheet->tile_count_x;
    u8 x_symbol = TILEMAP_TILE_START_SYMBOL + x;
    u8 y_symbol = TILEMAP_TILE_START_SYMBOL + y;

    _tilesheet->tile_symbols[x][y] = (tile_symbol) {
      .c = { x_symbol, y_symbol, _tilesheet->sheet_id}
    };
  }

  state->tilesheet_amouth++;
}
unsigned int load_texture_from_atlas(atlas_texture_id _id, Rectangle texture_area) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) { 
    TraceLog(LOG_ERROR, "resource::load_texture()::texture type out of bound");
    return INVALID_ID32;
  }
  atlas_texture tex = {0};
  tex.source = texture_area;
  tex.atlas_handle = &state->textures[ATLAS_TEXTURE_ID];

  state->atlas_textures[_id] = tex;

  return state->textures[_id].id;
}
