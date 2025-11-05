
#ifndef EVENT_H
#define EVENT_H

#include "defines.h"

typedef struct event_context {
  data128 data;
  event_context(void) {
    this->data = data128();
  }
  event_context(data128 in_data) : event_context() {
    this->data = in_data;
  }
  event_context(i64 value1, i64 value2 = 0) : event_context() { data = data128(value1, value2); }
  event_context(u64 value1, u64 value2 = 0) : event_context() { data = data128(value1, value2); }
  event_context(f64 value1, f64 value2 = 0) : event_context() { data = data128(value1, value2); }
  event_context(i32 value1, i32 value2 = 0, i32 value3 = 0, i32 value4 = 0) : event_context() { 
    data = data128(value1, value2, value3, value4); 
  }
  event_context(u32 value1, u32 value2 = 0, u32 value3 = 0, u32 value4 = 0) : event_context() {
    data = data128(value1, value2, value3, value4);
  }
  event_context(f32 value1, f32 value2 = 0, f32 value3 = 0, f32 value4 = 0) : event_context() {
    data = data128(value1, value2, value3, value4);
  }
  event_context(u16 value1, u16 value2 = 0, u16 value3 = 0, u16 value4 = 0, u16 value5 = 0, u16 value6 = 0, u16 value7 = 0, u16 value8 = 0) : event_context() {
    data = data128(value1, value2, value3, value4, value5, value6, value7, value8);
  }
  event_context(i16 value1, i16 value2 = 0, i16 value3 = 0, i16 value4 = 0, i16 value5 = 0, i16 value6 = 0, i16 value7 = 0, i16 value8 = 0) : event_context() {
    data = data128(value1, value2, value3, value4, value5, value6, value7, value8);
  }
  event_context(u16* value, u16 len) : event_context() {
    data = data128(value, len);
  }
  event_context(i16* value, u16 len) : event_context() {
    data = data128(value, len);
  }
  event_context(u8* value, u16 len) : event_context() {
    data = data128(value, len);
  }
  event_context(i8* value, u16 len) : event_context() {
    data = data128(value, len);
  }
  event_context(char* value, u16 len) : event_context() {
    data = data128(value, len);
  }
} event_context;

typedef bool (*PFN_on_event)(i32 code, event_context data);

bool event_system_initialize(void) ;
consteval void event_system_shutdown(void);

bool event_register(i32 code, PFN_on_event on_event);

bool event_unregister(i32 code, PFN_on_event on_event);

bool event_fire(i32 code, event_context context);

typedef enum system_event_code {
  // app
  EVENT_CODE_APPLICATION_QUIT,
  EVENT_CODE_TOGGLE_BORDERLESS,
  EVENT_CODE_TOGGLE_FULLSCREEN,
  EVENT_CODE_TOGGLE_WINDOWED,
  EVENT_CODE_SET_POST_PROCESS_FADE_VALUE,
	
  // game_manager
  EVENT_CODE_END_GAME,
  /**
   * @brief WARN: This event fires 'clean_up_spawn_state()' function if player die from damage
  */
  EVENT_CODE_DAMAGE_PLAYER_IF_COLLIDE,
  EVENT_CODE_ADD_CURRENCY_COINS,
  EVENT_CODE_ADD_TO_INVENTORY,
  EVENT_CODE_SPAWN_ITEM,
  /**
   * @brief coll_data data128(i16[0], i16[1], i16[2], i16[3])
   * @brief gm_damage_spawn_if_collide(coll_data, damage i16[4], collision_type i16[5]);
   */
  EVENT_CODE_DAMAGE_ANY_SPAWN_IF_COLLIDE,
  EVENT_CODE_KILL_ALL_SPAWNS,

  // Spawn
  EVENT_CODE_SET_SPAWN_FOLLOW_DISTANCE,

  // scene_manager
  EVENT_CODE_SCENE_IN_GAME,
  EVENT_CODE_SCENE_EDITOR,
  EVENT_CODE_SCENE_MAIN_MENU,

  // scene_in_game
  EVENT_CODE_PAUSE_GAME,
  EVENT_CODE_RESUME_GAME,
  EVENT_CODE_TOGGLE_GAME_PAUSE,
  EVENT_CODE_SPAWN_COMBAT_FEEDBACK_FLOATING_TEXT,
  EVENT_CODE_BEGIN_CHEST_OPENING_SEQUENCE,

  // user_interface
  EVENT_CODE_UI_UPDATE_PROGRESS_BAR,

  // sound
  EVENT_CODE_PLAY_SOUND,
  EVENT_CODE_PLAY_SOUND_GROUP,
  EVENT_CODE_PLAY_MUSIC,
  EVENT_CODE_RESET_SOUND,
  EVENT_CODE_RESET_SOUND_GROUP,
  EVENT_CODE_RESET_MUSIC,

  // camera
  EVENT_CODE_CAMERA_SET_FRUSTUM,
  EVENT_CODE_CAMERA_SET_DRAWING_EXTENT,
  EVENT_CODE_CAMERA_SET_OFFSET,
  EVENT_CODE_CAMERA_SET_TARGET,
  EVENT_CODE_BEGIN_CAMERA_SHAKE,

  /**
   * @brief state->cam_met.handle.target.x = context.data.f32[0];
   * @brief state->cam_met.handle.target.y = context.data.f32[1];
   */
  EVENT_CODE_CAMERA_SET_CAMERA_POSITION,
  EVENT_CODE_CAMERA_SET_ZOOM,
  EVENT_CODE_CAMERA_SET_ZOOM_TARGET,
  EVENT_CODE_CAMERA_ADD_ZOOM,

  // player
  EVENT_CODE_PLAYER_ADD_EXP,
  EVENT_CODE_PLAYER_SET_POSITION,
  EVENT_CODE_PLAYER_TAKE_DAMAGE,
  EVENT_CODE_PLAYER_HEAL,
  
  MAX_EVENT_CODE
} system_event_code;

#endif
