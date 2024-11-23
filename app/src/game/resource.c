#include "resource.h"

#include "core/fmemory.h"

#include "game_manager.h"

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
  load_texture("MenuButton.png", false, (Vector2){0, 0}, BUTTON_TEXTURE);
  load_spritesheet("level_up_sheet.png", LEVEL_UP_SHEET, 60, 150, 150, 8, 8);
  load_spritesheet("IdleLeft.png", PLAYER_ANIMATION_IDLELEFT, 15, 86, 86, 1, 4);
  load_spritesheet("IdleRight.png", PLAYER_ANIMATION_IDLERIGHT, 15, 86, 86, 1, 4);
  load_spritesheet("MoveLeft.png", PLAYER_ANIMATION_MOVELEFT, 10, 86, 86, 1, 6);
  load_spritesheet("MoveRight.png", PLAYER_ANIMATION_MOVERIGHT, 10, 86, 86, 1, 6);
  load_spritesheet("ButtonReflection.png", BUTTON_REFLECTION_SHEET, 30, 80, 16, 1, 9);
  load_spritesheet("ButtonCRT.png", BUTTON_CRT_SHEET, 8, 78, 12, 1, 4);

  resource_system_initialized = true;

  return true;
}

void update_resource_system() {
  resource_system->game_on_scene = get_current_scene_type();

  for (int i = 1; i <= resource_system->render_queue_amouth; ++i) {
    if (!resource_system->render_sprite_queue[i].is_started) {
      continue;
    }
    update_sprite(i);
  }
}

void render_resource_system() {
  if (!resource_system_initialized)
    return;

  for (int i = 1; i <= resource_system->render_queue_amouth; ++i) {
    spritesheet sheet = resource_system->render_sprite_queue[i];
    if (!sheet.is_started ||
        sheet.render_on_scene != resource_system->game_on_scene) {
      sheet.is_started = false;
      continue;
    }
    /*     DrawTexturePro(sheet.handle, sheet.current_frame_rect, sheet.coord,
                       (Vector2){0, 0}, 0, WHITE); */

    DrawTexturePro(sheet.handle,
                   (Rectangle){.x = sheet.current_frame_rect.x,
                               .y = sheet.current_frame_rect.y,
                               .width = sheet.current_frame_rect.width,
                               .height = sheet.current_frame_rect.height},
                   (Rectangle){.x = sheet.coord.x,
                               .y = sheet.coord.y,
                               .width = sheet.coord.width,
                               .height = sheet.coord.height},
                   (Vector2){.x = 0, .y = 0}, 0,
                   WHITE); // Draws the background to main menu

#if DEBUG_COLLISIONS
    DrawRectangleLines(sheet.coord.x, sheet.coord.y, sheet.coord.width,
                       sheet.coord.height, WHITE);
#endif
  }
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

spritesheet get_spritesheet_by_enum(spritesheet_type type) {

  if (type == SPRITESHEET_UNSPECIFIED || type > MAX_SPRITESHEET_SLOTS) {
    return (spritesheet){0};
  }

  return resource_system->sprites[type];
}

const char *rs_path(const char *_path) {
  return TextFormat("%s%s", resource_path, _path);
}

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
}

u16 register_sprite(spritesheet_type _type, scene_type _scene, bool _play_once, bool center_sprite) {
  spritesheet ss = resource_system->sprites[_type];
  resource_system->render_queue_amouth++;

  ss.render_on_scene = _scene;
  ss.play_once = _play_once;
  ss.is_started = false;
  ss.should_center = center_sprite;
  resource_system->render_sprite_queue[resource_system->render_queue_amouth] = ss;

  return resource_system->render_queue_amouth;
}

void play_sprite_on_site(u16 _id, Rectangle dest) {
  spritesheet ss = resource_system->render_sprite_queue[_id];
  ss.playmod = ON_SITE;
  ss.is_started = true;
  ss.coord = dest;
  if (ss.should_center) {
    ss.coord.x -= ss.coord.width / 2.f;
    ss.coord.y -= ss.coord.height / 2.f;
  }

  resource_system->render_sprite_queue[_id] = ss;
}

void play_sprite_on_player(u16 _id) {
  spritesheet ss = resource_system->render_sprite_queue[_id];
  ss.playmod = ON_PLAYER;
  ss.is_started = true;
  ss.coord.x = get_player_position().x;
  ss.coord.y = get_player_position().y;
  ss.coord.width = get_player_dimentions().x;
  ss.coord.height = get_player_dimentions().y;

  if (ss.should_center) {
    ss.coord.x -= get_player_dimentions().x / 2.f;
    ss.coord.y -= get_player_dimentions().y / 2.f;
  }

  resource_system->render_sprite_queue[_id] = ss;
}

void update_sprite(u16 queue_index) {
  spritesheet sheet = resource_system->render_sprite_queue[queue_index];
  sheet.counter++;
  //             1                     12
  if (sheet.counter >= 60 / sheet.fps) {
    sheet.counter = 0;
    sheet.current_col++;
    if (sheet.current_col >= sheet.col_total) {
      sheet.current_row++;
      sheet.current_col = 0;
    }
    if (sheet.current_row >= sheet.row_total) {
      sheet.current_row = 0;
      sheet.current_col = 0;
      if (sheet.play_once) {
        stop_sprite(queue_index, true);
        return;
      }
    }
  }
  sheet.current_frame_rect.x =
    sheet.current_col * sheet.current_frame_rect.width;
  sheet.current_frame_rect.y =
    sheet.current_row * sheet.current_frame_rect.height;

  switch (sheet.playmod) {
    default: {
      TraceLog(LOG_ERROR,
      "ERROR::resources::update_resource_system()::Sprite was corrupted");
      stop_sprite(queue_index, true);
      return;
    }
    case SPRITESHEET_PLAYMOD_UNSPECIFIED:{
      TraceLog(LOG_ERROR,
      "ERROR::resources::update_resource_system()::Sprite was corrupted");
      stop_sprite(queue_index, true);
      return;
    }

    case ON_SITE: break;
    case ON_PLAYER: {
      sheet.coord.width = get_player_dimentions().x;
      sheet.coord.height = get_player_dimentions().y;
      sheet.coord.x = get_player_position().x - sheet.coord.width / 2.f;
      sheet.coord.y = get_player_position().y - sheet.coord.height / 2.f;
    break;
    }
  }

  resource_system->render_sprite_queue[queue_index] = sheet;
}

void queue_sprite_change_location(u16 queue_index, Rectangle _location) {
  resource_system->render_sprite_queue[queue_index].coord = _location;
}

bool is_sprite_playing(u16 i) {
  return (i == 0) ? false
                  : resource_system->render_sprite_queue[i].is_started == true;
}

void stop_sprite(i16 i, bool reset) {
  if (i > resource_system->render_queue_amouth || i < 0) {
    TraceLog(LOG_ERROR, "ERROR::resource::stop_sprite()::Given index value is "
                        "out of bound! Unable to stop sprite");
    return;
  }
  if (i == 0) return;
  if (reset) {
    resource_system->render_sprite_queue[i].counter = 0;
    resource_system->render_sprite_queue[i].current_col = 0;
    resource_system->render_sprite_queue[i].current_row = 0;
  }

  resource_system->render_sprite_queue[i].is_started = false;
}

unsigned int load_texture(const char *_path, bool resize, Vector2 new_size,
                          texture_type type) {
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
