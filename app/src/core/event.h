
#ifndef EVENT_H
#define EVENT_H

#include "defines.h"

typedef struct event_context {
    // 128 bytes
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;
} event_context;

typedef bool (*PFN_on_event)(u16 code, event_context data);

void event_system_initialize(void) ;
void event_system_shutdown(void);

bool event_register(u16 code, PFN_on_event on_event);

bool event_unregister(u16 code, PFN_on_event on_event);

bool event_fire(u16 code, event_context context);

typedef enum system_event_code {
    
    // app
    EVENT_CODE_APPLICATION_QUIT = 0x01,
    EVENT_CODE_TOGGLE_BORDERLESS,
    EVENT_CODE_TOGGLE_FULLSCREEN,
    EVENT_CODE_TOGGLE_WINDOWED,

    // game_manager
    EVENT_CODE_PAUSE_GAME,
    EVENT_CODE_UNPAUSE_GAME,
    EVENT_CODE_RELOCATE_SPAWN_COLLISION,
    EVENT_CODE_RELOCATE_PROJECTILE_COLLISION,
    EVENT_CODE_DELETE_SPAWN_COLLISION,

    // scene_manager
    EVENT_CODE_SCENE_IN_GAME,
    EVENT_CODE_SCENE_EDITOR,
    EVENT_CODE_SCENE_MAIN_MENU,
    EVENT_CODE_SCENE_MANAGER_SET_TARGET,
    EVENT_CODE_SCENE_MANAGER_SET_CAM_POS,
    EVENT_CODE_SCENE_MANAGER_SET_ZOOM,

    // user_interface
    EVENT_CODE_UI_SHOW_PAUSE_MENU,
    EVENT_CODE_UI_SHOW_SETTINGS_MENU,
    EVENT_CODE_UI_UPDATE_PROGRESS_BAR,
    EVENT_CODE_UI_START_FADEIN_EFFECT,
    EVENT_CODE_UI_START_FADEOUT_EFFECT,

    // player
    EVENT_CODE_PLAYER_ADD_EXP,
    EVENT_CODE_PLAYER_SET_POSITION,
    EVENT_CODE_PLAYER_TAKE_DAMAGE,

    MAX_EVENT_CODE = 0xFF
} system_event_code;

#endif
