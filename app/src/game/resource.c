#include "resource.h"

#include "core/fmemory.h"
#include "defines.h"
#include "raylib.h"

static resource_system_state* resource_system;

bool resource_system_initialized = false;

bool resource_system_initialize() {
    if (resource_system_initialized) return false;

    resource_system = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

    resource_system->texture_amouth = -1;

    load_texture("D:\\Workspace\\Resources\\fudesumi.png", true, (Vector2){64, 64}, ENEMY_TEXTURE);
    load_texture("D:\\Workspace\\Resources\\space_bg.png", true, (Vector2){3840 , 2160}, BACKGROUND);
    load_texture("D:\\Workspace\\Resources\\wabbit_alpha.png", true, (Vector2){32, 32}, PLAYER_TEXTURE);
    load_spritesheet("D:\\Workspace\\Resources\\level_up_sheet.png", LEVEL_UP_SHEET, 150, 150, 8, 8);

    resource_system_initialized = true;

    return true;
}

void update_resource_system() {

    for (int i = 0; i < resource_system->sprite_amouth+1; ++i) { 
        if (!resource_system->sprites[i].is_started) continue;
        spritesheet sheet = resource_system->sprites[i];

        sheet.current_col++;
        if (sheet.current_col > sheet.col_total) {
            sheet.current_row++;
            sheet.current_col = 0;
        }
        if (sheet.current_row > sheet.row_total) {
            sheet.current_row = 0;
            sheet.is_started = false;
        }
        sheet.current_frame_rect.x = sheet.current_col * sheet.current_frame_rect.width;
        sheet.current_frame_rect.y = sheet.current_row * sheet.current_frame_rect.height;

        resource_system->sprites[i] = sheet;
    }
}

void render_resource_system() {
    if (!resource_system_initialized) return;

    for (int i = 0; i < resource_system->sprite_amouth+1; ++i) {
        spritesheet sheet = resource_system->sprites[i];
        if (!sheet.is_started) continue;

        DrawTexturePro(sheet.handle, sheet.current_frame_rect, sheet.coord, (Vector2) {0, 25}, 0, WHITE);
    }
}

Texture2D get_texture_by_id(unsigned int id) {
    for (i16 i = 0; i < resource_system->texture_amouth + 1; i++) {
        if (resource_system->textures[i].id == id) return resource_system->textures[i];
    }

    return (Texture2D){.id = INVALID_ID32};
}

Texture2D get_texture_by_enum(texture_type type) {

    switch (type) {
        case TEX_UNSPECIFIED: {
            return (Texture2D) { .id = INVALID_ID32 };
        }
        case PLAYER_TEXTURE: {
            return resource_system->textures[PLAYER_TEXTURE];
        }
        case ENEMY_TEXTURE: {
            return resource_system->textures[ENEMY_TEXTURE];
        }
        case BACKGROUND: {
            return resource_system->textures[BACKGROUND];
        }

        default: break;
    }

    return (Texture2D) { .id = INVALID_ID32 };
}

void play_sprite(spritesheet_type _type, Vector2 coord) {
    resource_system->sprites[_type].coord.x = coord.x - resource_system->sprites[_type].coord.width / 2.f;
    resource_system->sprites[_type].coord.y = coord.y - resource_system->sprites[_type].coord.height / 2.f;

    resource_system->sprites[_type].is_started = true;
}

unsigned int load_texture(const char* _path, bool resize, Vector2 new_size, texture_type type) {

    if (!FileExists(_path)) return INVALID_ID32;

    Texture2D tex;

    if (resize) {
        Image img = LoadImage(_path);
        ImageResize(&img, new_size.x, new_size.y);
        tex = LoadTextureFromImage(img);
    } else {
        tex = LoadTexture(_path);
    }

    switch (type) {
        case TEX_UNSPECIFIED: {
            resource_system->textures[++resource_system->texture_amouth] = tex;

            return resource_system->textures[resource_system->texture_amouth].id;
        }
        case PLAYER_TEXTURE: {
            resource_system->textures[PLAYER_TEXTURE] = tex;

            return resource_system->textures[PLAYER_TEXTURE].id;
        }
        case ENEMY_TEXTURE: {
            resource_system->textures[ENEMY_TEXTURE] = tex;

            return resource_system->textures[ENEMY_TEXTURE].id;
        }
        case BACKGROUND: {
            resource_system->textures[BACKGROUND] = tex;

            return resource_system->textures[BACKGROUND].id;
        }

        default: break;
    }

    return 0;
}
void load_spritesheet(const char* _path, spritesheet_type _type, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col) 
{
    if (!FileExists(_path)) return;

    spritesheet _sheet;
    Texture2D tex = LoadTexture(_path);
    resource_system->sprite_amouth++;

    _sheet.handle = tex;
    _sheet.type = _type;
    _sheet.is_started = false;
    _sheet.row_total = _total_row;
    _sheet.col_total = _total_col;
    _sheet.frame_total = _total_col * _total_row;
    _sheet.current_col = 0;
    _sheet.current_row = 0;
    _sheet.current_frame_rect = (Rectangle) {
        .x = 0,
        .y = 0,
        .width = _frame_width,
        .height = _frame_height
    };
    _sheet.coord = (Rectangle) {
        .x = 0,
        .y = 0,
        .width = _frame_width,
        .height = _frame_height
    };

    switch (_type) {
        case SPRITESHEET_UNSPECIFIED: {
            resource_system->sprites[resource_system->sprite_amouth] = _sheet;
            TraceLog(LOG_WARNING, "resource::load_spritesheet()::WARNING! Spritesheet type was not set");
        }
        case LEVEL_UP_SHEET: resource_system->sprites[LEVEL_UP_SHEET] = _sheet;

    default: break;
    }
}