#include "resource.h"

#include "core/fmemory.h"

#include "defines.h"
#include "game_manager.h"
#include "raylib.h"

static resource_system_state* resource_system;

bool resource_system_initialized = false;

const char* resource_path = "D:\\Workspace\\Resources\\";
const char* rs_path(const char* _path);

bool resource_system_initialize() {
    if (resource_system_initialized) return false;

    resource_system = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

    resource_system->texture_amouth = -1;

    load_texture("fudesumi", true, (Vector2){64, 64}, ENEMY_TEXTURE);
    load_texture("space_bg", true, (Vector2){3840 , 2160}, BACKGROUND);
    load_texture("MenuButton", false, (Vector2){0 , 0}, BUTTON_TEXTURE);
    load_spritesheet("level_up_sheet", LEVEL_UP_SHEET, 60, 150, 150, 8, 8);
    load_spritesheet("IdleLeft", PLAYER_ANIMATION_IDLELEFT, 15, 86, 86, 1, 4);
    load_spritesheet("IdleRight", PLAYER_ANIMATION_IDLERIGHT, 15, 86, 86, 1, 4);
    load_spritesheet("MoveLeft", PLAYER_ANIMATION_MOVELEFT, 10, 86, 86, 1, 6);
    load_spritesheet("MoveRight", PLAYER_ANIMATION_MOVERIGHT, 10, 86, 86, 1, 6);
    load_spritesheet("ButtonReflection", BUTTON_REFLECTION_SHEET, 10, 80, 16, 1, 9);

    resource_system_initialized = true;

    return true;
}

void update_resource_system() {
    for (int i = 0; i < resource_system->sprite_amouth+1; ++i) { 
        if (!resource_system->sprites[i].is_started) continue;
        else if (resource_system->sprites[i].playmod == SPRITESHEET_PLAYMOD_UNSPECIFIED) {
            TraceLog(LOG_ERROR, "resources::update_resource_system()::Sprite %d's playmod was set 0 skipping.", i);
            resource_system->sprites[i].is_started = false;
            continue;
        }
        spritesheet sheet = resource_system->sprites[i];
        sheet.counter++;
        //             1                     12
        if(sheet.counter >= 60/sheet.fps) {
            sheet.counter = 0;

            sheet.current_col++;
            if (sheet.current_col >= sheet.col_total) {
                sheet.current_row++;
                sheet.current_col = 0;
            }
            if (sheet.current_row >= sheet.row_total) {
                sheet.current_row = 0;
                sheet.current_col = 0;
                if(sheet.play_once) sheet.is_started = false;
            }
        }

        sheet.current_frame_rect.x = sheet.current_col * sheet.current_frame_rect.width;
        sheet.current_frame_rect.y = sheet.current_row * sheet.current_frame_rect.height;

        switch (sheet.playmod) {
            case SPRITESHEET_PLAYMOD_UNSPECIFIED: break;  
            case ON_SITE: break;

            case ON_PLAYER: {
                sheet.coord.x = get_player_position().x - sheet.coord.width / 2.f;
                sheet.coord.y = get_player_position().y - sheet.coord.height / 2.f;
                break;
            }
            case ON_SPAWN: {
                sheet.coord.x = get_actor_by_id(sheet.attached_spawn)->position.x - sheet.coord.width / 2.f;
                sheet.coord.y = get_actor_by_id(sheet.attached_spawn)->position.y - sheet.coord.height / 2.f;
                break;
            }

            default: {
                TraceLog(LOG_ERROR, "resources::update_resource_system()::Sprite '%d' was corrupted! Please update and try again.", i);
                resource_system->sprites[i].is_started = false;
                continue;
            }
        }
        resource_system->sprites[i] = sheet;
    }
}

void render_resource_system() {
    if (!resource_system_initialized) return;

    for (int i = 0; i < resource_system->sprite_amouth+1; ++i) {
        spritesheet sheet = resource_system->sprites[i];
        if (!sheet.is_started) continue;

        DrawTexturePro(sheet.handle, sheet.current_frame_rect, sheet.coord, (Vector2) {0, 0}, 0, WHITE);

#if DEBUG_COLLISIONS 
        DrawRectangleLines(sheet.coord.x, sheet.coord.y, sheet.coord.width, sheet.coord.height, WHITE);
#endif

    }
}

Texture2D get_texture_by_id(unsigned int id) {
    for (i16 i = 0; i <= resource_system->texture_amouth; ++i) {
        if (resource_system->textures[i].id == id) return resource_system->textures[i];
    }

    return (Texture2D){.id = INVALID_ID32};
}

Texture2D get_texture_by_enum(texture_type type) {

    if(type > MAX_TEXTURE_SLOTS || type == TEX_UNSPECIFIED) return (Texture2D) { .id = INVALID_ID32 };

    return resource_system->textures[type];
}

spritesheet get_spritesheet_by_enum(spritesheet_type type) {

    if (type == SPRITESHEET_UNSPECIFIED || type > MAX_SPRITESHEET_SLOTS) {
        return (spritesheet) {0};
    }

    return resource_system->sprites[type];
}

const char* rs_path(const char* _path) {
    return TextFormat("%s%s%s",resource_path, _path, ".png");
}

void flip_texture_spritesheet(spritesheet_type _type, world_direction _w_direction) {
    spritesheet sheet = get_spritesheet_by_enum(_type);
    if (sheet.w_direction == _w_direction) return;

    Image img = LoadImageFromTexture(sheet.handle);
    ImageFlipHorizontal(&img);

    const Color *color = LoadImageColors(img);
    UpdateTexture(sheet.handle, color);
    sheet.w_direction = _w_direction;

    resource_system->sprites[_type] = sheet;
    UnloadImage(img);
}

void play_sprite(spritesheet_type _type, spritesheet_playmod _mod, bool _play_once, Rectangle dest, bool center_sprite, u16 id){
    switch (_mod) {
        case SPRITESHEET_PLAYMOD_UNSPECIFIED: {
            TraceLog(LOG_WARNING, "resources::play_sprite()::Playmod not specified! Aborting playing sprite.");
            return;
        }
        case ON_SITE: {

            resource_system->sprites[_type].coord = dest;
            if(center_sprite) 
            {
                resource_system->sprites[_type].coord.x -= resource_system->sprites[_type].coord.width / 2.f;
                resource_system->sprites[_type].coord.y -= resource_system->sprites[_type].coord.height / 2.f;
            }

            resource_system->sprites[_type].playmod = ON_SITE;
            break;
        }
        case ON_PLAYER: {
            resource_system->sprites[_type].coord.x = get_player_position().x;
            resource_system->sprites[_type].coord.y = get_player_position().y;
            if (dest.width != 0 && dest.height != 0) {
                resource_system->sprites[_type].coord.width = dest.width;
                resource_system->sprites[_type].coord.height = dest.height;
            }
            if(center_sprite) 
            {
                resource_system->sprites[_type].coord.x -= get_player_dimentions().x / 2.f;
                resource_system->sprites[_type].coord.y -= get_player_dimentions().y / 2.f;
            }

            resource_system->sprites[_type].playmod = ON_PLAYER;
            break;
        }
        case ON_SPAWN: {
            resource_system->sprites[_type].coord.x = get_actor_by_id(id)->position.x;
            resource_system->sprites[_type].coord.y = get_actor_by_id(id)->position.y;
            if (dest.width != 0 && dest.height != 0) {
                resource_system->sprites[_type].coord.width = dest.width;
                resource_system->sprites[_type].coord.height = dest.height;
            }
            if(center_sprite) 
            {
                resource_system->sprites[_type].coord.x -= resource_system->sprites[_type].coord.width / 2.f;
                resource_system->sprites[_type].coord.y -= resource_system->sprites[_type].coord.height / 2.f;
            }

            resource_system->sprites[_type].playmod = ON_SPAWN;
            break;
        }

        default: {
            TraceLog(LOG_WARNING, "resources::play_sprite()::Playmod setted out of bound! Please set a mod and try again.");
            return;
        }
    }

    
    resource_system->sprites[_type].is_started = true;
    resource_system->sprites[_type].play_once = _play_once;
}

void stop_sprite(spritesheet_type _type, bool should_reset) {
    resource_system->sprites[_type].is_started = false;
    if (should_reset) {
        resource_system->sprites[_type].attached_spawn = 0;
        resource_system->sprites[_type].current_col = 0;
        resource_system->sprites[_type].current_row = 0;
        resource_system->sprites[_type].current_frame_rect.x = 0;
        resource_system->sprites[_type].current_frame_rect.y = 0;
        resource_system->sprites[_type].current_frame = 0;
        resource_system->sprites[_type].play_once = 0;
        resource_system->sprites[_type].playmod = 0;
        resource_system->sprites[_type].coord = (Rectangle){0};
    }
}

unsigned int load_texture(const char* _path, bool resize, Vector2 new_size, texture_type type) {
    const char* path = rs_path(_path);
    if (!FileExists(path)) return INVALID_ID32;

    Texture2D tex;
    if (resize) {
        Image img = LoadImage(path);
        ImageResize(&img, new_size.x, new_size.y);
        tex = LoadTextureFromImage(img);
    } else {
        tex = LoadTexture(path);
    }
    resource_system->texture_amouth++;

    if( type ==  TEX_UNSPECIFIED){
        resource_system->textures[resource_system->texture_amouth] = tex;
        return resource_system->textures[resource_system->texture_amouth].id;
    }
    
    resource_system->textures[type] = tex;
    return resource_system->textures[type].id;
}

void load_spritesheet(const char* _path, spritesheet_type _type, u8 _fps, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col) {
    const char* path = rs_path(_path);
    if (!FileExists(path)) return;
    else if(_type > MAX_SPRITESHEET_SLOTS ) return;

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
    _sheet.fps = _fps;

    if (_type == SPRITESHEET_UNSPECIFIED) {
            resource_system->sprites[resource_system->sprite_amouth] = _sheet;
            TraceLog(LOG_WARNING, "resource::load_spritesheet()::WARNING! Spritesheet type was not set");
        }

    resource_system->sprites[_type] = _sheet;
}
