
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
void event_system_shutdown();

bool event_register(u16 code, void* listener, PFN_on_event on_event);

bool event_unregister(u16 code, void* listener, PFN_on_event on_event);

bool event_fire(u16 code, void* sender, event_context context);

typedef enum system_event_code {
    
    EVENT_CODE_APPLICATION_QUIT = 0x01,
    EVENT_CODE_SCENE_IN_GAME = 0x02,
    EVENT_CODE_SCENE_MAIN_MENU = 0x03,

    MAX_EVENT_CODE = 0xFF
} system_event_code;

#endif
