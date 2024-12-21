
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

void render_sprite_renderqueue(spritesheet_play_system *system, u16 queue_index) {
  if (queue_index > system->renderqueue_count || queue_index <= 0) {
    TraceLog(LOG_ERROR, "ERROR::resource::stop_sprite()::invalid index: %d",
             queue_index);
    return;
  }
  spritesheet* sheet = &system->renderqueue[queue_index];

  DrawTexturePro(
  sheet->handle,
  (Rectangle){
    .x = sheet->current_frame_rect.x,
    .y = sheet->current_frame_rect.y,
    .width = sheet->current_frame_rect.width,
    .height = sheet->current_frame_rect.height},
  (Rectangle){
    .x = sheet->coord.x,
    .y = sheet->coord.y,
    .width = sheet->coord.width,
    .height = sheet->coord.height},
  (Vector2){.x = 0, .y = 0}, 0, sheet->tint
  );
  #if DEBUG_COLLISIONS
    DrawRectangleLines(sheet.coord.x, sheet.coord.y, sheet.coord.width,
                       sheet.coord.height, WHITE);
  #endif
}

u16 _register_sprite(spritesheet_play_system *system, spritesheet_type _type, bool _play_looped, bool _play_once, bool _center_sprite) {
  spritesheet ss = get_spritesheet_by_enum(_type);
  system->renderqueue_count++;

  ss.play_looped = _play_looped;
  ss.play_once = _play_once;
  ss.should_center = _center_sprite;
  ss.is_started = false;
  system->renderqueue[system->renderqueue_count] = ss;

  return system->renderqueue_count;
}

void _play_sprite_on_site(spritesheet_play_system *system, u16 _id, Color _tint, Rectangle dest) {
  spritesheet* ss = &system->renderqueue[_id];
  if (ss->play_once && ss->is_played && !ss->is_started) { return; }
  ss->playmod = ON_SITE;
  ss->is_started = true;
  ss->is_played = true;
  ss->coord = dest;
  ss->tint = _tint;
  if (ss->should_center) {
    ss->coord.x -= ss->coord.width / 2.f;
    ss->coord.y -= ss->coord.height / 2.f;
  }
  render_sprite_renderqueue(system, _id);
}

void update_sprite(spritesheet_play_system *system, u16 queue_index) {
  spritesheet sheet = system->renderqueue[queue_index];
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

Texture2D* _get_texture_by_enum(texture_type _type) {
    return get_texture_by_enum(_type);
}
spritesheet _get_spritesheet_by_enum(spritesheet_type _type) {
    return get_spritesheet_by_enum(_type);
}
const char *_rs_path(const char *_path) {
    return rs_path(_path);
}

