
#include "spritesheet.h"
#include "defines.h"
#include "game/resource.h"
#include "raylib.h"

void update_sprite(spritesheet_play_system *system, u16 queue_index);
void render_sprite_renderqueue(spritesheet_play_system *system, u16 queue_index);

void _update_sprite_renderqueue(spritesheet_play_system *system) {
  for (int i = 1; i <= system->renderqueue_count; ++i) {
    if (!system->renderqueue[i].is_started) {
      continue;
    }

    update_sprite(system, i);
  }
}
void update_sprite(spritesheet_play_system *system, u16 queue_index) {
  spritesheet sheet = system->renderqueue[queue_index];
  if (sheet.fps == 0) {
    TraceLog(LOG_ERROR, "ERROR::spritesheet::update_sprite()::Sheet not meant to be playable");
    return;
  }
  sheet.counter++;

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

      if (sheet.play_looped) {
        _reset_sprite(system, queue_index, false);
        return;
      }
      else {
        _stop_sprite(system, queue_index, true);
        return;
      }
    }
  }
  sheet.current_frame_rect.x =
      sheet.current_col * sheet.current_frame_rect.width;
  sheet.current_frame_rect.y =
      sheet.current_row * sheet.current_frame_rect.height;

  system->renderqueue[queue_index] = sheet;
}
void render_sprite_renderqueue(spritesheet_play_system *system, u16 queue_index) {
  if (queue_index > system->renderqueue_count || queue_index <= 0) {
    TraceLog(LOG_ERROR, "ERROR::spritesheet::render_sprite_renderqueue()::invalid queue index: %d", queue_index);
    return;
  }
  spritesheet* sheet = &system->renderqueue[queue_index];

  DrawTexturePro(
  sheet->handle,
  (Rectangle){
    .x = sheet->current_frame_rect.x, .y = sheet->current_frame_rect.y,
    .width = sheet->current_frame_rect.width, .height = sheet->current_frame_rect.height},
  (Rectangle){
    .x = sheet->coord.x, .y = sheet->coord.y,
    .width = sheet->coord.width, .height = sheet->coord.height},
  (Vector2){.x = 0, .y = 0}, 0, sheet->tint
  );
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      sheet->coord.x, sheet->coord.y, 
      sheet->coord.width,sheet->coord.height, 
      WHITE);
  #endif
}

u16 _register_sprite(spritesheet_play_system *system, spritesheet_id _id, bool _play_looped, bool _play_once, bool _center_sprite) {
  if (_id > SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "ERROR::spritesheet::play_sprite_on_site()::sprite type out of bound");
    return INVALID_ID16;
  }
  spritesheet sheet = get_spritesheet_by_enum(_id);

  system->renderqueue_count++;

  sheet.play_looped = _play_looped;
  sheet.play_once = _play_once;
  sheet.should_center = _center_sprite;
  sheet.is_started = false;
  system->renderqueue[system->renderqueue_count] = sheet;

  return system->renderqueue_count;
}

void _play_sprite_on_site(spritesheet_play_system *system, u16 _id, Color _tint, Rectangle dest) {
  if (_id > system->renderqueue_count || _id <= 0) {
    TraceLog(LOG_ERROR, "ERROR::spritesheet::play_sprite_on_site()::invalid queue id: %d", _id);
    return;
  }
  spritesheet* sheet = &system->renderqueue[_id];

  if (sheet->play_once && sheet->is_played && !sheet->is_started) { return; }
  sheet->playmod = ON_SITE;
  sheet->is_started = true;
  sheet->is_played = true;
  sheet->coord = dest;
  sheet->tint = _tint;
  if (sheet->should_center) {
    sheet->coord.x -= sheet->coord.width / 2.f;
    sheet->coord.y -= sheet->coord.height / 2.f;
  }

  render_sprite_renderqueue(system, _id);
}

void _draw_sprite_on_site(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center) {
  if (_id > SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "ERROR::spritesheet::draw_sprite_on_site()::invalid sprite type");
    return;
  }
  spritesheet sheet = get_spritesheet_by_enum(_id);

  u16 col = frame % sheet.col_total;
  u16 row = frame / sheet.row_total;

  if (_should_center) {
    pos.x -= sheet.current_frame_rect.width/2.f;
    pos.y -= sheet.current_frame_rect.height/2.f;
  }

  DrawTexturePro(
  sheet.handle,
  (Rectangle){
    .x = sheet.current_frame_rect.width * col, .y = sheet.current_frame_rect.height * row,
    .width = sheet.current_frame_rect.width, .height = sheet.current_frame_rect.height},
  (Rectangle){
    .x = pos.x, .y = pos.y,
    .width = sheet.current_frame_rect.width * scale.x, .height = sheet.current_frame_rect.height * scale.y},
  (Vector2){.x = 0, .y = 0}, 0, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      pos.x, pos.y, 
      sheet.current_frame_rect.width * scale.x,sheet.current_frame_rect.height * scale.y, 
      WHITE);
  #endif
}

void _queue_sprite_change_location(spritesheet_play_system *system, u16 queue_index, Rectangle _location) {
  if (queue_index > system->renderqueue_count || queue_index <= 0) {
    TraceLog(LOG_ERROR, "ERROR::resource::stop_sprite()::invalid index: %d",
             queue_index);
    return;
  }
  system->renderqueue[queue_index].coord = _location;
}

bool _is_sprite_playing(spritesheet_play_system *system, u16 queue_index) {
  if (queue_index > system->renderqueue_count || queue_index <= 0)  {
    TraceLog(LOG_ERROR, "ERROR::resource::stop_sprite()::invalid index: %d",
    queue_index);
    return false;
  }
  return system->renderqueue[queue_index].is_started == true;
}

void _stop_sprite(spritesheet_play_system *system, u16 queue_index, bool reset) {
  if (queue_index > system->renderqueue_count || queue_index <= 0)  {
    TraceLog(LOG_ERROR, "ERROR::resource::stop_sprite()::invalid index: %d",
    queue_index);
    return;
  }
  if (reset) {
    system->renderqueue[queue_index].counter = 0;
    system->renderqueue[queue_index].current_col = 0;
    system->renderqueue[queue_index].current_row = 0;
    system->renderqueue[queue_index].current_frame_rect.x = 0;
    system->renderqueue[queue_index].current_frame_rect.y = 0;
  }

  system->renderqueue[queue_index].is_started = false;
}

void _reset_sprite(spritesheet_play_system *system, u16 _queue_index, bool _retrospective) {
  if (_queue_index > system->renderqueue_count || _queue_index <= 0)  {
    TraceLog(LOG_ERROR, "ERROR::resource::reset_sprite()::invalid index: %d",
    _queue_index);
    return;
  }

  system->renderqueue[_queue_index].counter = 0;
  system->renderqueue[_queue_index].current_col = 0;
  system->renderqueue[_queue_index].current_row = 0;
  system->renderqueue[_queue_index].current_frame_rect.x = 0;
  system->renderqueue[_queue_index].current_frame_rect.y = 0;

  if (_retrospective) {
    system->renderqueue[_queue_index].is_played = false;
  }
}

Texture2D* _get_texture_by_enum(texture_id _id) {
    return get_texture_by_enum(_id);
}
spritesheet _get_spritesheet_by_enum(spritesheet_id _id) {
    return get_spritesheet_by_enum(_id);
}
const char *_rs_path(const char *_path) {
    return rs_path(_path);
}

