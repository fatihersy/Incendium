#include "spritesheet.h"
#include "game/resource.h"

void update_sprite(spritesheet *sheet);
void render_sprite(spritesheet *sheet);

void update_sprite(spritesheet *sheet) {
  if (!sheet) {
    TraceLog(LOG_ERROR, "spritesheet::update_sprite()::Sheet is null");
    return;
  }
  if (sheet->fps == 0) {
    TraceLog(LOG_ERROR, "spritesheet::update_sprite()::Sheet not meant to be playable");
    return;
  }
  sheet->counter++;

  if (sheet->counter >= 60 / sheet->fps) {
    sheet->counter = 0;
    sheet->current_col++;
    if (sheet->current_col >= sheet->col_total) {
      sheet->current_row++;
      sheet->current_col = 0;
    }
    if (sheet->current_row >= sheet->row_total) {
      sheet->current_row = 0;
      sheet->current_col = 0;

      if (sheet->play_looped) {
        reset_sprite(sheet, false);
        return;
      }
      else {
        stop_sprite(sheet, true);
        return;
      }
    }
  }

  sheet->current_frame_rect.x = sheet->current_col * sheet->current_frame_rect.width;
  sheet->current_frame_rect.y = sheet->current_row * sheet->current_frame_rect.height;
}
void render_sprite(spritesheet *sheet) {
  if (!sheet || !sheet->tex_handle) {
    TraceLog(LOG_ERROR, "spritesheet::render_sprite()::Sheet is not valid");
    return;
  }
  Rectangle source = (Rectangle){
    sheet->current_frame_rect.x + sheet->offset.x,
    sheet->current_frame_rect.y + sheet->offset.y,
    sheet->current_frame_rect.width,
    sheet->current_frame_rect.height,
  };
  Rectangle dest = sheet->coord;

  DrawTexturePro(*sheet->tex_handle, source, dest, (Vector2){0}, 0, sheet->tint);

  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      sheet->coord.x, sheet->coord.y, 
      sheet->coord.width,sheet->coord.height, 
      WHITE);
  #endif
}
void set_sprite(spritesheet *sheet, bool _play_looped, bool _play_once, bool _center_sprite) {
  if (!sheet || sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::register_sprite()::Sheet is not valid");
    return;
  }
  *sheet = *get_spritesheet_by_enum(sheet->sheet_id);
  
  sheet->play_looped = _play_looped;
  sheet->play_once = _play_once;
  sheet->should_center = _center_sprite;
  sheet->is_started = false;
}
void play_sprite_on_site(spritesheet *sheet, Color _tint, Rectangle dest) {
  if (!sheet || sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::play_sprite_on_site()::Sheet is not valid");
    return;
  }
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

  render_sprite(sheet);
}
void draw_sprite_on_site_by_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center) {
  if (_id > SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::draw_sprite_on_site()::invalid sprite type");
    return;
  }
  spritesheet sheet = *get_spritesheet_by_enum(_id);
  if (sheet.sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sheet.sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::play_sprite_on_site()::Recieved spritesheet doesn't exist or invalid");
    return;
  }
  u16 col = frame % sheet.col_total;
  u16 row = frame / sheet.col_total;

  if (_should_center) {
    pos.x -= sheet.current_frame_rect.width/2.f;
    pos.y -= sheet.current_frame_rect.height/2.f;
  }
  Rectangle source = (Rectangle){
    sheet.offset.x + sheet.current_frame_rect.width * col, 
    sheet.offset.y + sheet.current_frame_rect.height * row,
    sheet.current_frame_rect.width, 
    sheet.current_frame_rect.height
  };
  Rectangle dest = (Rectangle){
    pos.x, 
    pos.y,
    sheet.current_frame_rect.width * scale.x, 
    sheet.current_frame_rect.height * scale.y
  };
  DrawTexturePro(*sheet.tex_handle,source,dest,(Vector2){0}, 0, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      pos.x, pos.y, 
      sheet->current_frame_rect.width * scale.x,sheet->current_frame_rect.height * scale.y, 
      WHITE);
  #endif
}

void draw_sprite_on_site(spritesheet *sheet, Color _tint, Vector2 pos, Vector2 scale, u16 frame, bool _should_center) {
  if (!sheet || sheet->sheet_id > SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::draw_sprite_on_site()::invalid sprite type");
    return;
  }
  u16 col = frame % sheet->col_total;
  u16 row = frame / sheet->row_total;

  if (_should_center) {
    pos.x -= sheet->current_frame_rect.width/2.f;
    pos.y -= sheet->current_frame_rect.height/2.f;
  }
  Rectangle source = (Rectangle){
    sheet->offset.x + sheet->current_frame_rect.width * col, 
    sheet->offset.y + sheet->current_frame_rect.height * row,
    sheet->current_frame_rect.width, 
    sheet->current_frame_rect.height
  };
  Rectangle dest = (Rectangle){
    pos.x, 
    pos.y,
    sheet->current_frame_rect.width * scale.x, 
    sheet->current_frame_rect.height * scale.y
  };
  DrawTexturePro(*sheet->tex_handle,source,dest,(Vector2){0}, 0, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      pos.x, pos.y, 
      sheet->current_frame_rect.width * scale.x,sheet->current_frame_rect.height * scale.y, 
      WHITE);
  #endif
}
void stop_sprite(spritesheet *sheet, bool reset) {
  if (!sheet)  {
    TraceLog(LOG_ERROR, "resource::stop_sprite()::Sheet is invalid");
    return;
  }
  if (reset) {
    sheet->counter = 0;
    sheet->current_col = 0;
    sheet->current_row = 0;
    sheet->current_frame_rect.x = 0;
    sheet->current_frame_rect.y = 0;
  }

  sheet->is_started = false;
}
void reset_sprite(spritesheet *sheet, bool _retrospective) {
  if (!sheet)  {
    TraceLog(LOG_ERROR, "resource::reset_sprite()::sheet is invalid");
    return;
  }
  sheet->counter = 0;
  sheet->current_col = 0;
  sheet->current_row = 0;
  sheet->current_frame_rect.x = 0;
  sheet->current_frame_rect.y = 0;

  if (_retrospective) {
    sheet->is_played = false;
  }
}
Texture2D* _get_texture_by_enum(texture_id _id) {
  return get_texture_by_enum(_id);
}
atlas_texture* _get_atlas_texture_by_enum(atlas_texture_id _id) {
    return get_atlas_texture_by_enum(_id);
}
spritesheet* _get_spritesheet_by_enum(spritesheet_id _id) {
    return get_spritesheet_by_enum(_id);
}
const char *_rs_path(const char *_path) {
    return rs_path(_path);
}

