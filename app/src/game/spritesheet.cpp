#include "spritesheet.h"

#include "core/logger.h"

#include "game/resource.h"

void update_sprite(spritesheet *const sheet);
constexpr void render_sprite(const spritesheet * sheet,const Color _tint,const Rectangle dest);
constexpr void render_sprite_pro(const spritesheet * sheet, const Rectangle source, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint);
 
void update_sprite(spritesheet *const sheet) {
  if (not sheet or sheet == nullptr) {
    IWARN("spritesheet::update_sprite()::Sheet is invalid");
    return;
  }
  if (sheet->fps <= 0.f) {
    IWARN("spritesheet::update_sprite()::Sheet not meant to be playable");
    return;
  }
  if (not sheet->is_started or (sheet->is_played and (sheet->play_once or not sheet->reset_after_finish)) ) {
    return;
  }
  sheet->time_accumulator += GetFrameTime();
  f32 time_per_frame = 1.0f / sheet->fps;

  while (sheet->time_accumulator >= time_per_frame) {
    sheet->time_accumulator -= time_per_frame;
    sheet->current_col++;
    sheet->current_frame++;
    if (sheet->current_col >= sheet->col_total) {
      sheet->current_row++;
      sheet->current_col = 0;
    }
    if (sheet->current_row >= sheet->row_total) {
      if (sheet->reset_after_finish) {
        if (sheet->play_looped) {
          reset_sprite(sheet, false);
          return;
        }
        else {
          stop_sprite(sheet, true);
          sheet->is_played = true;
          return;
        }
      } else {
        sheet->is_played = true;
        return;
      }
    }
  }

  sheet->current_frame_rect.x = sheet->current_col * std::abs(sheet->current_frame_rect.width);
  sheet->current_frame_rect.y = sheet->current_row * std::abs(sheet->current_frame_rect.height);
}
constexpr void render_sprite(const spritesheet * sheet,const Color _tint,const Rectangle dest) {
  if (not sheet or sheet == nullptr or not sheet->tex_handle) {
    IWARN("spritesheet::render_sprite()::Sheet is not valid");
    return;
  }
  Rectangle source = Rectangle {
    sheet->current_frame_rect.x + sheet->offset.x, sheet->current_frame_rect.y + sheet->offset.y,
    sheet->current_frame_rect.width, sheet->current_frame_rect.height,
  };

  DrawTexturePro(*sheet->tex_handle, source, dest, sheet->origin, sheet->rotation, _tint);

  #if DEBUG_COLLISIONS
    Rectangle coll_dest = Rectangle { dest.x - sheet->origin.x, dest.y - sheet->origin.y, dest.width, dest.height};
    DrawRectangleLines( static_cast<i32>(coll_dest.x),  static_cast<i32>(coll_dest.y),  static_cast<i32>(coll_dest.width),  static_cast<i32>(coll_dest.height), WHITE);
  #endif
}
constexpr void render_sprite_pro(const spritesheet * sheet, const Rectangle source, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint) {
  if (not sheet or sheet == nullptr or not sheet->tex_handle) {
    IWARN("spritesheet::render_sprite_pro()::Sheet is not valid");
    return;
  }

  DrawTexturePro(*sheet->tex_handle, source, dest, origin, rotation, _tint);

  #if DEBUG_COLLISIONS
    Rectangle coll_dest = Rectangle { dest.x - origin.x, dest.y - origin.y, dest.width, dest.height};
    DrawRectangleLines( static_cast<i32>(coll_dest.x),  static_cast<i32>(coll_dest.y),  static_cast<i32>(coll_dest.width),  static_cast<i32>(coll_dest.height), WHITE);
  #endif
}
void set_sprite(spritesheet *const sheet, bool _loop_animation, bool _lock_after_finish) {
  if (not sheet or sheet == nullptr or sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::set_sprite()::Sheet is not valid");
    return;
  }
  const spritesheet * const _sheet_ptr = get_spritesheet_by_enum(sheet->sheet_id);
  if (not _sheet_ptr or _sheet_ptr == nullptr) {
    IERROR("spritesheet::set_sprite()::Sheet resourse pointer is not valid");
    return;
  }
  *sheet = (*_sheet_ptr);
  
  sheet->play_looped = _loop_animation;
  sheet->play_once = _lock_after_finish;
  sheet->is_started = false;
  sheet->reset_after_finish = true;
}
void play_sprite_on_site(spritesheet *const sheet, Color _tint, const Rectangle dest) {
  if (not sheet or sheet == nullptr or sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::play_sprite_on_site()::Sheet is not valid");
    return;
  }
  if (sheet->play_once and sheet->is_played and not sheet->is_started) { return; }

  sheet->is_started = true;
  sheet->tint = _tint;
  render_sprite(sheet, _tint, dest);
}
void play_sprite_on_site_pro(spritesheet *const sheet, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint) {
  if (not sheet or sheet == nullptr or sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::play_sprite_on_site_pro()::Sheet is not valid");
    return;
  }
  if (sheet->play_once and sheet->is_played and not sheet->is_started) { return; }

  sheet->is_started = true;

  Rectangle source = Rectangle {
    sheet->current_frame_rect.x + sheet->offset.x, sheet->current_frame_rect.y + sheet->offset.y,
    sheet->current_frame_rect.width, sheet->current_frame_rect.height,
  };

  render_sprite_pro(sheet, source, dest, origin, rotation, _tint);
}
void play_sprite_on_site_ex(spritesheet *const sheet, const Rectangle source, const Rectangle dest, const Vector2 origin, const f32 rotation, const Color _tint) {
  if (not sheet or sheet == nullptr or sheet->sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::play_sprite_on_site_ex()::Sheet is not valid");
    return;
  }
  if (sheet->play_once and sheet->is_played and not sheet->is_started) { return; }

  sheet->is_started = true;

  render_sprite_pro(sheet, source, dest, origin, rotation, _tint);
}

void draw_sprite_on_site_by_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, i32 frame) {
  if (_id > SHEET_ID_SPRITESHEET_TYPE_MAX or _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::draw_sprite_on_site_by_id()::invalid sprite type");
    return;
  }
  const spritesheet * const _sheet_ptr = get_spritesheet_by_enum(_id);
  if (not _sheet_ptr or _sheet_ptr == nullptr ) {
    IWARN("spritesheet::draw_sprite_on_site_by_id()::Recieved sheet is invalid");
    return;
  }
  spritesheet sheet = (*_sheet_ptr);
  if (sheet.sheet_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or sheet.sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::draw_sprite_on_site_by_id()::Recieved spritesheet doesn't exist or invalid");
    return;
  }
  i32 col = frame % sheet.col_total;
  i32 row = frame / sheet.col_total;

  Rectangle source = Rectangle{
    sheet.offset.x + sheet.current_frame_rect.width * col, 
    sheet.offset.y + sheet.current_frame_rect.height * row,
    sheet.current_frame_rect.width,
    sheet.current_frame_rect.height
  };
  Rectangle dest = Rectangle{ pos.x,  pos.y, sheet.current_frame_rect.width * scale.x,  sheet.current_frame_rect.height * scale.y};
  DrawTexturePro(*sheet.tex_handle,source,dest, ZEROVEC2, 0.f, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines(static_cast<i32>(dest.x), static_cast<i32>(dest.y), static_cast<i32>(dest.width), static_cast<i32>(dest.height), WHITE);
  #endif
}

void draw_sprite_on_site(spritesheet *const sheet, Color _tint, i32 frame) {
  if (not sheet or sheet == nullptr  or sheet->sheet_id > SHEET_ID_SPRITESHEET_TYPE_MAX or sheet->sheet_id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("spritesheet::draw_sprite_on_site()::invalid sprite type");
    return;
  }
  i32 col = frame % sheet->col_total;
  i32 row = frame / sheet->col_total;

  Rectangle source = Rectangle{
    sheet->offset.x + sheet->current_frame_rect.width * col,
    sheet->offset.y + sheet->current_frame_rect.height * row,
    sheet->current_frame_rect.width,
    sheet->current_frame_rect.height
  };
  
  DrawTexturePro(*sheet->tex_handle, source, sheet->coord, sheet->origin, sheet->rotation, _tint);
  
  #if DEBUG_COLLISIONS
    DrawRectangleLines( static_cast<i32>(dest.x),  static_cast<i32>(dest.y),  static_cast<i32>(dest.width),  static_cast<i32>(dest.height), WHITE);
  #endif
}
void stop_sprite(spritesheet *const sheet, bool reset) {
  if (not sheet or sheet == nullptr)  {
    IWARN("resource::stop_sprite()::Sheet is invalid");
    return;
  }
  if (reset) {
    sheet->time_accumulator = 0.f;
    sheet->current_col = 0;
    sheet->current_row = 0;
    sheet->current_frame_rect.x = 0;
    sheet->current_frame_rect.y = 0;
  }

  sheet->is_started = false;
}
void reset_sprite(spritesheet *const sheet, bool _retrospective) {
  if (not sheet or sheet == nullptr)  {
    IWARN("resource::reset_sprite()::sheet is invalid");
    return;
  }
  sheet->time_accumulator = 0.f;
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
