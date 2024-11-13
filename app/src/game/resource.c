#include "resource.h"

#include "core/fmemory.h"

#include "defines.h"
#include "game_manager.h"
#include "raylib.h"
#include <stdbool.h>

static resource_system_state* resource_system;

bool resource_system_initialized = false;

bool resource_system_initialize() {
    if (resource_system_initialized) return false;

    resource_system = (resource_system_state*)allocate_memory_linear(sizeof(resource_system_state), true);

    resource_system->texture_amouth = -1;

    load_texture("D:\\Workspace\\Resources\\fudesumi.png", true, (Vector2){64, 64}, ENEMY_TEXTURE);
    load_texture("D:\\Workspace\\Resources\\space_bg.png", true, (Vector2){3840 , 2160}, BACKGROUND);
    load_texture("D:\\Workspace\\Resources\\wabbit_alpha.png", true, (Vector2){32, 32}, PLAYER_TEXTURE);
    load_spritesheet("D:\\Workspace\\Resources\\level_up_sheet.png", LEVEL_UP_SHEET, 60, 150, 150, 8, 8);
    load_spritesheet("D:\\Workspace\\Resources\\Idle.png", PLAYER_ANIMATION_IDLE, 15, 144, 144, 1, 4);
    load_spritesheet("D:\\Workspace\\Resources\\Move.png", PLAYER_ANIMATION_RUN, 10, 144, 144, 1, 6);

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
        //             1                     60
        if(sheet.counter >= GetFPS()/sheet.fps) {
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
        // TODO: Hardcoded offset, make it configurable
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

void play_sprite(spritesheet_type _type, spritesheet_playmod _mod, bool _play_once, Vector2 _coord, u16 id){
    switch (_mod) {
        case SPRITESHEET_PLAYMOD_UNSPECIFIED: {
            TraceLog(LOG_WARNING, "resources::play_sprite()::Playmod not specified! Aborting playing sprite.");
            return;
        }
        case ON_SITE: {
            resource_system->sprites[_type].coord.x = _coord.x - resource_system->sprites[_type].coord.width / 2.f;
            resource_system->sprites[_type].coord.y = _coord.y - resource_system->sprites[_type].coord.height / 2.f;

            resource_system->sprites[_type].playmod = ON_SITE;
            resource_system->sprites[_type].is_started = true;
            resource_system->sprites[_type].play_once = _play_once;
            break;
        }
        case ON_PLAYER: {
            resource_system->sprites[_type].coord.x = get_player_position().x - resource_system->sprites[_type].coord.width / 2.f;
            resource_system->sprites[_type].coord.y = get_player_position().y - resource_system->sprites[_type].coord.height / 2.f;

            resource_system->sprites[_type].playmod = ON_PLAYER;
            resource_system->sprites[_type].is_started = true;
            resource_system->sprites[_type].play_once = _play_once;
            break;
        }
        case ON_SPAWN: {
            resource_system->sprites[_type].coord.x = get_actor_by_id(id)->position.x - resource_system->sprites[_type].coord.width / 2.f;
            resource_system->sprites[_type].coord.y = get_actor_by_id(id)->position.y - resource_system->sprites[_type].coord.height / 2.f;

            resource_system->sprites[_type].playmod = ON_SPAWN;
            resource_system->sprites[_type].is_started = true;
            resource_system->sprites[_type].play_once = _play_once;
            break;
        }

        default: {
            TraceLog(LOG_WARNING, "resources::play_sprite()::Playmod setted out of bound! Please set a mod and try again.");
            return;
        }
    }
}

void stop_sprite(spritesheet_type _type) {
    resource_system->sprites[_type].is_started = false;
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
void load_spritesheet(const char* _path, spritesheet_type _type, u8 _fps, u8 _frame_width, u8 _frame_height, u8 _total_row, u8 _total_col) 
{
    if (!FileExists(_path)) return;

    spritesheet _sheet = {0};
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

    switch (_type) {
        case SPRITESHEET_UNSPECIFIED: {
            resource_system->sprites[resource_system->sprite_amouth] = _sheet;
            TraceLog(LOG_WARNING, "resource::load_spritesheet()::WARNING! Spritesheet type was not set");
        }
        case LEVEL_UP_SHEET: resource_system->sprites[LEVEL_UP_SHEET] = _sheet;
        case PLAYER_ANIMATION_IDLE: resource_system->sprites[PLAYER_ANIMATION_IDLE] = _sheet;
        case PLAYER_ANIMATION_RUN: resource_system->sprites[PLAYER_ANIMATION_RUN] = _sheet;

    default: break;
    }
}