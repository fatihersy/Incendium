#include "resource.h"
#include <defines.h>

#include "core/fmemory.h"

typedef struct resource_system_state {
  u16 texture_amouth;
  u16 sprite_amouth;
  u16 image_amouth;
  u16 tilesheet_amouth;

  Texture2D textures[TEX_ID_MAX];
  spritesheet sprites[SPRITESHEET_TYPE_MAX];
  Image images[IMAGE_TYPE_MAX];
  tilesheet tilesheets[TILESHEET_TYPE_MAX];

  scene_type game_on_scene;
} resource_system_state;

static resource_system_state *state;

unsigned int load_texture(const char* _path, bool resize, Vector2 new_size, texture_id _id);
bool load_image(const char *_path, bool resize, Vector2 new_size, image_type type);
void load_spritesheet(const char* _path, spritesheet_type _type, u16 _fps, u16 _frame_width, u16 _frame_height, u16 _total_row, u16 _total_col);
void load_tilesheet(tilesheet_type _sheet_sheet_type, texture_id _sheet_tex_id, u16 _tile_count_x, u16 _tile_count_y, u16 _tile_size);

bool resource_system_initialize(void) {
  if (state) return false;

  state = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

  //NOTE: _path = "%s%s", RESOURCE_PATH, _path
  load_texture("space_bg.png",                 false, (Vector2){0,  0 }, TEX_ID_BACKGROUND);
  load_texture("fudesumi.png",                 true,  (Vector2){64, 64}, TEX_ID_ENEMY_TEXTURE);
  load_texture("map_tileset.png",              false, (Vector2){0,  0 }, TEX_ID_MAP_TILESET_TEXTURE);
  load_texture("progress_bar_inside_full.png", false, (Vector2){0,  0 }, TEX_ID_PROGRESS_BAR_INSIDE_FULL);
  load_texture("progress_bar_outside_full.png",false, (Vector2){0,  0 }, TEX_ID_PROGRESS_BAR_OUTSIDE_FULL);
  load_texture("crimson_fantasy_panel.png",    false, (Vector2){0,  0 }, TEX_ID_CRIMSON_FANTASY_PANEL);
  load_texture("crimson_fantasy_panel_bg.png", false, (Vector2){0,  0 }, TEX_ID_CRIMSON_FANTASY_PANEL_BG);
  load_texture("map_props_atlas.png",          false, (Vector2){0,  0 }, TEX_ID_MAP_PROPS_ATLAS);
  load_texture("skill_icons.png",              false, (Vector2){0,  0 }, TEX_ID_SKILL_ICON_ATLAS);
  load_spritesheet("idle_left.png",         PLAYER_ANIMATION_IDLE_LEFT, 15, 86, 86, 1, 4);
  load_spritesheet("idle_right.png",        PLAYER_ANIMATION_IDLE_RIGHT, 15, 86, 86, 1, 4);
  load_spritesheet("move_left.png",         PLAYER_ANIMATION_MOVE_LEFT, 10, 86, 86, 1, 6);
  load_spritesheet("move_right.png",        PLAYER_ANIMATION_MOVE_RIGHT, 10, 86, 86, 1, 6);
  load_spritesheet("take_damage_left.png",  PLAYER_ANIMATION_TAKE_DAMAGE_LEFT, 15, 86, 86, 1, 4);
  load_spritesheet("take_damage_right.png", PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, 15, 86, 86, 1, 4);
  load_spritesheet("wreck_left.png",        PLAYER_ANIMATION_WRECK_LEFT, 15, 90, 110, 1, 4);
  load_spritesheet("wreck_right.png",       PLAYER_ANIMATION_WRECK_RIGHT, 15, 90, 110, 1, 4);
  load_spritesheet("button_reflection.png", BUTTON_REFLECTION_SHEET, 30, 80, 16, 1, 9);
  load_spritesheet("button_crt.png",        BUTTON_CRT_SHEET, 8, 78, 12, 1, 4);
  load_spritesheet("screen_crt_sheet.png",  SCREEN_CRT_SHEET, 8, 1280, 720, 1, 4);
  load_spritesheet("slider_percent_edited.png", SLIDER_PERCENT, 0, 20, 10, 1, 11);
  load_spritesheet("slider_option.png", SLIDER_OPTION, 0, 21, 10, 1, 2);
  load_spritesheet("slider_left_button.png", SLIDER_LEFT_BUTTON, 0, 10, 10, 1, 2);
  load_spritesheet("slider_right_button_edited.png", SLIDER_RIGHT_BUTTON, 0, 11, 10, 1, 2);
  load_spritesheet("menu_button.png", MENU_BUTTON, 0, 80, 16, 1, 2);
  load_spritesheet("flat_button.png", FLAT_BUTTON, 0, 44, 14, 1, 2);
  load_spritesheet("fireball.png", FIREBALL_ANIMATION, 15, 32, 32, 1, 4);
  load_tilesheet(TILESHEET_TYPE_MAP, TEX_ID_MAP_TILESET_TEXTURE, 49, 63, 32);

  return true;
}

Texture2D* get_texture_by_enum(texture_id _id) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_texture_by_enum()::Texture type out of bound");
    return (Texture2D*){0};
  }

  return &state->textures[_id];
}
Image* get_image_by_enum(image_type type) {

  if (type >= IMAGE_TYPE_MAX || type <= IMAGE_TYPE_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_image_by_enum()::Image type out of bound");
    return (Image*){0};
  }

  return &state->images[type];
}

spritesheet get_spritesheet_by_enum(spritesheet_type type) {

  if (type >= SPRITESHEET_TYPE_MAX || type <= SPRITESHEET_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_spritesheet_by_enum()::Spritesheet type out of bound");
    return (spritesheet){0};
  }
  return state->sprites[type];
}

tilesheet* get_tilesheet_by_enum(tilesheet_type type) {
  if (type >= TILESHEET_TYPE_MAX || type <= TILESHEET_TYPE_UNSPECIFIED){
    TraceLog(LOG_WARNING, "resource::get_tilesheet_by_enum()::Tilesheet type out of bound");
    return (tilesheet*){0};
  }

  return &state->tilesheets[type];
}

const char *rs_path(const char *_path) {
  return TextFormat("%s%s", RESOURCE_PATH, _path);
}

unsigned int load_texture(const char *_path, bool resize, Vector2 new_size, texture_id _id) {
  const char *path = rs_path(_path);
  if (!FileExists(path)) { TraceLog(
  LOG_ERROR,"ERROR::resource::load_texture():: Path:'%s' Cannot find", path);
    return INVALID_ID32;
  } 
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) { TraceLog(
  LOG_ERROR,
  "ERROR::resource::load_texture()::texture type out of bound");
    return INVALID_ID32;
  }

  Texture2D tex;
  if (resize) {
    Image img = LoadImage(path);
    ImageResize(&img, new_size.x, new_size.y);
    tex = LoadTextureFromImage(img);
  } else {
    tex = LoadTexture(path);
  }

  state->texture_amouth++;
  state->textures[_id] = tex;
  return state->textures[_id].id;
}

bool load_image(const char *_path, bool resize, Vector2 new_size, image_type type) {
  const char *path = rs_path(_path);
  if (!FileExists(path)) { TraceLog(
    LOG_ERROR, "resource::load_image()::Path does not exist");
    return false;
  }
  if (type >= IMAGE_TYPE_MAX || type <= IMAGE_TYPE_UNSPECIFIED) { TraceLog(
  LOG_ERROR,
  "ERROR::resource::load_image()::Image type out of bound");
    return false;
  }

  Image img;
  if (resize) {
    img = LoadImage(path);
    ImageResize(&img, new_size.x, new_size.y);
  } else {
    img = LoadImage(path);
  }

  state->image_amouth++;
  state->images[type] = img;
  return true;
}


void load_spritesheet(const char *_path, spritesheet_type _type, u16 _fps, u16 _frame_width, u16 _frame_height, u16 _total_row, u16 _total_col) {
  const char *path = rs_path(_path);
  if (!FileExists(path)) { TraceLog(
  LOG_ERROR,"ERROR::resource::load_spritesheet():: Path:'%s' Cannot find", path);
    return;
  } 
  if (_type >= MAX_SPRITESHEET_SLOTS || _type <= SPRITESHEET_UNSPECIFIED) { TraceLog(
    LOG_ERROR, "resource::load_spritesheet()::Sheet type out of bound");
    return;
  }

  spritesheet _sheet = {0};
  Texture2D tex = LoadTexture(path);

  _sheet.handle = tex;
  _sheet.type = _type;
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
  state->sprites[_type] = _sheet;
}

void load_tilesheet(tilesheet_type _sheet_sheet_type, texture_id _sheet_tex_id, u16 _tile_count_x, u16 _tile_count_y, u16 _tile_size) {
  if (_sheet_sheet_type >= MAX_TILESHEET_SLOTS || _sheet_sheet_type <= 0) {
    TraceLog(LOG_ERROR,
             "ERROR::resource::load_tilesheet()::Sheet type out of bound");
    return;
  }
  if (_sheet_tex_id >= TEX_ID_MAX || _sheet_tex_id <= 0) {
    TraceLog(
        LOG_ERROR,
        "ERROR::resource::load_tilesheet()::texture type out of bound");
  }
  tilesheet* _tilesheet = &state->tilesheets[_sheet_sheet_type];

  _tilesheet->tex = get_texture_by_enum(_sheet_tex_id);
  _tilesheet->sheet_type = _sheet_sheet_type;
  _tilesheet->tex_id = _sheet_tex_id;
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
      .c = { x_symbol, y_symbol, _tilesheet->sheet_type}
    };
  }

  state->tilesheet_amouth++;
}


