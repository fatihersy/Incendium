
#ifndef EVENT_H
#define EVENT_H

#include "defines.h"

typedef struct event_context {
  data128 data;
  event_context() {}
  event_context(data128 in_data) :data(in_data) {}
    
  event_context(i64 value1, i64 value2 = 0){
    data = data128(value1, value2);
  }
  event_context(u64 value1, u64 value2 = 0) {
  	data = data128(value1, value2);
  }
  event_context(f64 value1, f64 value2 = 0){
    data = data128(value1, value2);
  }
  event_context(i32 value1, i32 value2 = 0, i32 value3 = 0, i32 value4 = 0){
    data = data128(value1, value2, value3, value4);
  }
  event_context(u32 value1, u32 value2 = 0, u32 value3 = 0, u32 value4 = 0){
    data = data128(value1, value2, value3, value4);
  }
  event_context(f32 value1, f32 value2 = 0, f32 value3 = 0, f32 value4 = 0){
    data = data128(value1, value2, value3, value4);
  }
  event_context(u16 value1, u16 value2 = 0, u16 value3 = 0, u16 value4 = 0, u16 value5 = 0, u16 value6 = 0, u16 value7 = 0, u16 value8 = 0){
    data = data128(value1, value2, value3, value4, value5, value6, value7, value8);
  }
  event_context(i16 value1, i16 value2 = 0, i16 value3 = 0, i16 value4 = 0, i16 value5 = 0, i16 value6 = 0, i16 value7 = 0, i16 value8 = 0){
    data = data128(value1, value2, value3, value4, value5, value6, value7, value8);
  }
  event_context(u16* value, u16 len){
    data = data128(value, len);
  }
  event_context(i16* value, u16 len){
    data = data128(value, len);
  }
  event_context(u8* value, u16 len){
    data = data128(value, len);
  }
  event_context(i8* value, u16 len){
    data = data128(value, len);
  }
  event_context(char* value, u16 len){
    data = data128(value, len);
  }
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
  EVENT_CODE_END_GAME,
  EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE,
  EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE,

  // scene_manager
  EVENT_CODE_SCENE_IN_GAME,
  EVENT_CODE_SCENE_EDITOR,
  EVENT_CODE_SCENE_MAIN_MENU,
  EVENT_CODE_SCENE_MANAGER_SET_TARGET,
  EVENT_CODE_SCENE_MANAGER_SET_CAM_POS,
  EVENT_CODE_SCENE_MANAGER_SET_ZOOM,

  // scene_in_game
  EVENT_CODE_ADD_CURRENCY_SOULS,
  EVENT_CODE_RESUME_GAME,

  // user_interface
  EVENT_CODE_UI_UPDATE_PROGRESS_BAR,
  EVENT_CODE_UI_START_FADEIN_EFFECT,
  EVENT_CODE_UI_START_FADEOUT_EFFECT,

  // sound
  EVENT_CODE_PLAY_BUTTON_ON_CLICK,
  EVENT_CODE_PLAY_FIRE_HIT,
  EVENT_CODE_PLAY_MAIN_MENU_THEME,
  EVENT_CODE_PLAY_NIGHT_THEME2,
  EVENT_CODE_PLAY_TRACK5,
  EVENT_CODE_PLAY_SOUND,
  EVENT_CODE_PLAY_MUSIC,
  EVENT_CODE_RESET_SOUND,
  EVENT_CODE_RESET_MUSIC,

  // player
  EVENT_CODE_PLAYER_ADD_EXP,
  EVENT_CODE_PLAYER_SET_POSITION,
  EVENT_CODE_PLAYER_TAKE_DAMAGE,
  MAX_EVENT_CODE = 0xFF
} system_event_code;

#endif
