
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

typedef bool (*PFN_on_event)(u16 code, void* sender, void* listener_inst, event_context data);

void event_system_initialize() ;
//void event_system_shutdown(); TODO: Essantial

bool event_register(u16 code, void* listener, PFN_on_event on_event);

bool event_unregister(u16 code, void* listener, PFN_on_event on_event);

bool event_fire(u16 code, void* sender, event_context context);

typedef enum system_event_code {
    
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // game_manager
    EVENT_CODE_PAUSE_GAME,
    EVENT_CODE_UNPAUSE_GAME,
    EVENT_CODE_RELOCATE_SPAWN_COLLISION,

    // scene_manager
    EVENT_CODE_SCENE_IN_GAME,
    EVENT_CODE_SCENE_EDITOR,
    EVENT_CODE_SCENE_MAIN_MENU,
    EVENT_CODE_SCENE_MANAGER_SET_TARGET,

    // user_interface
    EVENT_CODE_UI_SHOW_PAUSE_SCREEN,
    EVENT_CODE_UI_SHOW_UNPAUSE_SCREEN,

    // player
    EVENT_CODE_PLAYER_ADD_EXP,
    EVENT_CODE_PLAYER_SET_POSITION,
    EVENT_CODE_PLAYER_DEAL_DAMAGE,

    MAX_EVENT_CODE = 0xFF
} system_event_code;

#endif
