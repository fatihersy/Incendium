#include "resource.h"

#include "core/fmemory.h"
#include "defines.h"

static resource_system_state *resource_system;

bool resource_system_initialized = false;

const char *resource_path = RESOURCE_PATH;

bool resource_system_initialize() {
  if (resource_system_initialized)
    return false;

  resource_system = (resource_system_state *)allocate_memory_linear(
      sizeof(resource_system_state), true);

  resource_system->texture_amouth = -1;

  //NOTE: _path = "%s%s", RESOURCE_PATH, _path
  load_texture("fudesumi.png", true, (Vector2){64, 64}, ENEMY_TEXTURE);
  load_texture("space_bg.png", true, (Vector2){3840, 2160}, BACKGROUND);
  load_texture("menu_button.png", false, (Vector2){0, 0}, BUTTON_TEXTURE);
  load_texture("healthbar_edited.png", false, (Vector2){0, 0}, HEALTHBAR_TEXTURE);
  load_texture("health_perc.png", false, (Vector2){0, 0}, HEALTH_PERC_TEXTURE);
  load_texture("map_tileset.png", false, (Vector2){0, 0}, MAP_TILESET_TEXTURE);
  load_spritesheet("level_up_sheet.png", LEVEL_UP_SHEET, 60, 150, 150, 8, 8);
  load_spritesheet("idle_left.png", PLAYER_ANIMATION_IDLE_LEFT, 15, 86, 86, 1, 4);
  load_spritesheet("idle_right.png", PLAYER_ANIMATION_IDLE_RIGHT, 15, 86, 86, 1, 4);
  load_spritesheet("move_left.png", PLAYER_ANIMATION_MOVE_LEFT, 10, 86, 86, 1, 6);
  load_spritesheet("move_right.png", PLAYER_ANIMATION_MOVE_RIGHT, 10, 86, 86, 1, 6);
  load_spritesheet("take_damage_left.png", PLAYER_ANIMATION_TAKE_DAMAGE_LEFT, 15, 86, 86, 1, 4);
  load_spritesheet("take_damage_right.png", PLAYER_ANIMATION_TAKE_DAMAGE_RIGHT, 15, 86, 86, 1, 4);
  load_spritesheet("wreck_left.png", PLAYER_ANIMATION_WRECK_LEFT, 15, 90, 110, 1, 4);
  load_spritesheet("wreck_right.png", PLAYER_ANIMATION_WRECK_RIGHT, 15, 90, 110, 1, 4);
  load_spritesheet("button_reflection.png", BUTTON_REFLECTION_SHEET, 30, 80, 16, 1, 9);
  load_spritesheet("button_crt.png", BUTTON_CRT_SHEET, 8, 78, 12, 1, 4);

  resource_system_initialized = true;

  return true;
}


Texture2D* get_texture_by_id(unsigned int id) {
  for (i16 i = 0; i <= resource_system->texture_amouth; ++i) {
    if (resource_system->textures[i].id == id)
      return &resource_system->textures[i];
  }

  return (Texture2D*){0};
}

Texture2D* get_texture_by_enum(texture_type type) {

  if (type > MAX_TEXTURE_SLOTS || type == TEX_UNSPECIFIED)
    return (Texture2D*){0};

  return &resource_system->textures[type];
}
Image* get_image_by_enum(image_type type) {

  if (type > MAX_IMAGE_SLOTS || type == IMG_UNSPECIFIED)
    return (Image*){0};

  return &resource_system->images[type];
}

spritesheet get_spritesheet_by_enum(spritesheet_type type) {

  if (type == SPRITESHEET_UNSPECIFIED || type > MAX_SPRITESHEET_SLOTS) {
    return (spritesheet){0};
  }

  return resource_system->sprites[type];
}

const char *rs_path(const char *_path) {
  return TextFormat("%s%s", resource_path, _path);
}

unsigned int load_texture(const char *_path, bool resize, Vector2 new_size, texture_type type) {
  const char *path = rs_path(_path);
  if (!FileExists(path))
    return INVALID_ID32;

  Texture2D tex;
  if (resize) {
    Image img = LoadImage(path);
    ImageResize(&img, new_size.x, new_size.y);
    tex = LoadTextureFromImage(img);
  } else {
    tex = LoadTexture(path);
  }
  resource_system->texture_amouth++;

  if (type == TEX_UNSPECIFIED) {
    resource_system->textures[resource_system->texture_amouth] = tex;
    return resource_system->textures[resource_system->texture_amouth].id;
  }

  resource_system->textures[type] = tex;
  return resource_system->textures[type].id;
}

bool load_image(const char *_path, bool resize, Vector2 new_size, image_type type) {
  const char *path = rs_path(_path);
  if (!FileExists(path) || type == IMG_UNSPECIFIED)
    return false;

  Image img;
  if (resize) {
    img = LoadImage(path);
    ImageResize(&img, new_size.x, new_size.y);
  } else {
    img = LoadImage(path);
  }

  resource_system->image_amouth++;

  resource_system->images[type] = img;
  return true;
}


void load_spritesheet(const char *_path, spritesheet_type _type, u8 _fps,
                      u8 _frame_width, u8 _frame_height, u8 _total_row,
                      u8 _total_col) {
  const char *path = rs_path(_path);
  if (!FileExists(path)) {
    TraceLog(LOG_ERROR,
             "ERROR::resource::load_spritesheet():: Path:'%s' Cannot find",
             path);
    return;
  } else if (_type > MAX_SPRITESHEET_SLOTS) {
    TraceLog(LOG_ERROR,
             "ERROR::resource::load_spritesheet()::Spritesheet slots full");
    return;
  }

  spritesheet _sheet = {0};
  Texture2D tex = LoadTexture(path);
  resource_system->sprite_amouth++;

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

  if (_type == SPRITESHEET_UNSPECIFIED) {
    resource_system->sprites[resource_system->sprite_amouth] = _sheet;
    TraceLog(
        LOG_WARNING,
        "resource::load_spritesheet()::WARNING! Spritesheet type was not set");
  }

  resource_system->sprites[_type] = _sheet;
}

/* 
void flip_texture_spritesheet(spritesheet_type _type,
                              world_direction _w_direction) {
  spritesheet sheet = get_spritesheet_by_enum(_type);
  if (sheet.w_direction == _w_direction)
    return;

  Image img = LoadImageFromTexture(sheet.handle);
  ImageFlipHorizontal(&img);

  const Color *color = LoadImageColors(img);
  UpdateTexture(sheet.handle, color);
  sheet.w_direction = _w_direction;

  resource_system->sprites[_type] = sheet;
  UnloadImage(img);
} */

