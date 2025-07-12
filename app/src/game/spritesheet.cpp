#include "spritesheet.h"
#include "game/resource.h"

void update_sprite(spritesheet *const sheet);
constexpr void render_sprite(const spritesheet * sheet,const Color _tint,const Rectangle dest);
constexpr void render_sprite_pro(const spritesheet * sheet, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint);
 
void update_sprite(spritesheet *const sheet) {
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
    sheet->current_frame++;
    if (sheet->current_col >= sheet->col_total) {
      sheet->current_row++;
      sheet->current_col = 0;
    }
    if (sheet->current_row >= sheet->row_total) {
      sheet->current_row = 0;
      sheet->current_col = 0;
      sheet->current_frame = 0;

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
constexpr void render_sprite(const spritesheet * sheet,const Color _tint,const Rectangle dest) {
  if (!sheet || !sheet->tex_handle) {
    TraceLog(LOG_ERROR, "spritesheet::render_sprite()::Sheet is not valid");
    return;
  }
  Rectangle source = Rectangle {
    sheet->current_frame_rect.x + sheet->offset.x,
    sheet->current_frame_rect.y + sheet->offset.y,
    sheet->current_frame_rect.width,
    sheet->current_frame_rect.height,
  };

  DrawTexturePro(*sheet->tex_handle, source, dest, sheet->origin, sheet->rotation, _tint);

  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      sheet->coord.x, sheet->coord.y, 
      sheet->coord.width,sheet->coord.height, 
      WHITE);
  #endif
}
constexpr void render_sprite_pro(const spritesheet * sheet, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint) {
  if (!sheet || !sheet->tex_handle) {
    TraceLog(LOG_ERROR, "spritesheet::render_sprite()::Sheet is not valid");
    return;
  }
  Rectangle source = Rectangle {
    sheet->current_frame_rect.x + sheet->offset.x,
    sheet->current_frame_rect.y + sheet->offset.y,
    sheet->current_frame_rect.width,
    sheet->current_frame_rect.height,
  };

  DrawTexturePro(*sheet->tex_handle, source, dest, origin, rotation, _tint);

  #if DEBUG_COLLISIONS 
    DrawRectangleLines(sheet->coord.x, sheet->coord.y, sheet->coord.width,sheet->coord.height, WHITE);
  #endif
}
void set_sprite(spritesheet *const sheet, bool _play_looped, bool _play_once) {
  if (!sheet || sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::register_sprite()::Sheet is not valid");
    return;
  }
  *sheet = *get_spritesheet_by_enum(sheet->sheet_id);
  
  sheet->play_looped = _play_looped;
  sheet->play_once = _play_once;
  sheet->is_started = false;
}
void play_sprite_on_site(spritesheet *const sheet, Color _tint, const Rectangle dest) {
  if (!sheet || sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::play_sprite_on_site()::Sheet is not valid");
    return;
  }
  if (sheet->play_once && sheet->is_played && !sheet->is_started) { return; }

  sheet->playmod = ON_SITE;
  sheet->is_started = true;
  sheet->is_played = true;
  sheet->tint = _tint;

  render_sprite(sheet, _tint, dest);
}
void play_sprite_on_site_pro(spritesheet *const sheet, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint) {
  if (!sheet || sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::play_sprite_on_site()::Sheet is not valid");
    return;
  }
  if (sheet->play_once && sheet->is_played && !sheet->is_started) { return; }

  sheet->playmod = ON_SITE;
  sheet->is_started = true;
  sheet->is_played = true;

  render_sprite_pro(sheet, dest, origin, rotation, _tint);
}

void draw_sprite_on_site_by_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame) {
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

  Rectangle source = Rectangle{
    sheet.offset.x + sheet.current_frame_rect.width * col, 
    sheet.offset.y + sheet.current_frame_rect.height * row,
    sheet.current_frame_rect.width, 
    sheet.current_frame_rect.height
  };
  Rectangle dest = Rectangle{
    pos.x, 
    pos.y,
    sheet.current_frame_rect.width * scale.x, 
    sheet.current_frame_rect.height * scale.y
  };
  DrawTexturePro(*sheet.tex_handle,source,dest, ZEROVEC2, 0, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      pos.x, pos.y, 
      sheet.current_frame_rect.width * scale.x,sheet.current_frame_rect.height * scale.y, 
      WHITE);
  #endif
}

void draw_sprite_on_site(spritesheet *const sheet, Color _tint, Vector2 pos, Vector2 scale, u16 frame) {
  if (!sheet || sheet->sheet_id > SHEET_ID_SPRITESHEET_TYPE_MAX || sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "spritesheet::draw_sprite_on_site()::invalid sprite type");
    return;
  }
  u16 col = frame % sheet->col_total;
  u16 row = frame / sheet->row_total;

  Rectangle source = Rectangle{
    sheet->offset.x + sheet->current_frame_rect.width * col, 
    sheet->offset.y + sheet->current_frame_rect.height * row,
    sheet->current_frame_rect.width, 
    sheet->current_frame_rect.height
  };
  Rectangle dest = Rectangle{
    pos.x, 
    pos.y,
    sheet->current_frame_rect.width * scale.x, 
    sheet->current_frame_rect.height * scale.y
  };
  DrawTexturePro(*sheet->tex_handle,source,dest, ZEROVEC2, 0, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines(
      pos.x, pos.y, 
      sheet->current_frame_rect.width * scale.x,sheet->current_frame_rect.height * scale.y, 
      WHITE);
  #endif
}
void stop_sprite(spritesheet *const sheet, bool reset) {
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
void reset_sprite(spritesheet *const sheet, bool _retrospective) {
  if (!sheet)  {
    TraceLog(LOG_ERROR, "resource::reset_sprite()::sheet is invalid");
    return;
  }
  sheet->counter = 0;
  sheet->current_col = 0;
  sheet->current_row = 0;
  sheet->current_frame = 0;
  sheet->current_frame_rect.x = 0;
  sheet->current_frame_rect.y = 0;

  if (_retrospective) {
    sheet->is_played = false;
    sheet->is_started = false;
  }
}
const Texture2D* ss_get_texture_by_enum(texture_id _id) {
  return get_texture_by_enum(_id);
}
const atlas_texture* ss_get_atlas_texture_by_enum(atlas_texture_id _id) {
    return get_atlas_texture_by_enum(_id);
}
const spritesheet* ss_get_spritesheet_by_enum(spritesheet_id _id) {
    return get_spritesheet_by_enum(_id);
}
const char *ss_rs_path(const char *_path) {
    return rs_path(_path);
}

