#include "app.h"
#include "settings.h"
#include "defines.h"

#if USE_PAK_FORMAT
  #include "tools/pak_parser.h"
#endif

#include "core/event.h"
#include "core/ftime.h"
#include "core/fmemory.h"

#include "game/camera.h"
#include "game/resource.h"
#include "game/scenes/scene_manager.h"
#include "game/world.h"

typedef struct app_system_state {
  app_settings settings;
  bool app_runing;
} app_system_state;

static app_system_state* state;

bool application_on_event(u16 code, event_context context);

bool app_initialize(void) {
  // Subsystems
  memory_system_initialize();
  event_system_initialize();
  time_system_initialize();
  state = (app_system_state*)allocate_memory_linear(sizeof(app_system_state), true);
  
  // Platform    
  settings_initialize();
  set_settings_from_ini_file("config.ini");
  state->settings = *get_app_settings();
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(
    state->settings.resolution[0], 
    state->settings.resolution[1], 
    state->settings.title);
  SetTargetFPS(TARGET_FPS); 
  SetExitKey(0);
  if (state->settings.window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
    i32 monitor = GetCurrentMonitor();
    ToggleBorderlessWindowed();
    SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
  }
  else if (state->settings.window_state == FLAG_FULLSCREEN_MODE) {
    ToggleFullscreen();
  }
  SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);

  // Game
  BeginDrawing();
  ClearBackground(CLEAR_BACKGROUND_COLOR); //TODO: Loading screen
  EndDrawing();

  #if USE_PAK_FORMAT
    pak_parser_system_initialize();
    parse_pak(PAK_FILE_LOCATION);
  #endif

  resource_system_initialize();
  create_camera(*get_resolution_div2());

  world_system_initialize(get_active_metrics(), *get_resolution_div2());
  if (!scene_manager_initialize()) {
    TraceLog(LOG_ERROR, "scene_manager() initialization failed");
    return false;
  }

  event_register(EVENT_CODE_APPLICATION_QUIT, application_on_event);
  event_register(EVENT_CODE_TOGGLE_BORDERLESS, application_on_event);
  event_register(EVENT_CODE_TOGGLE_FULLSCREEN, application_on_event);
  event_register(EVENT_CODE_TOGGLE_WINDOWED, application_on_event);

  state->app_runing = true;

  return true;
}

bool window_should_close(void) {
  return state->app_runing;
}

bool app_update(void) {
  if (GetFPS() > TARGET_FPS) return true;
  
  if(IsWindowFocused() && !IsWindowState(FLAG_WINDOW_TOPMOST)) {
    SetWindowState(FLAG_WINDOW_TOPMOST);
  }
  else if(!IsWindowFocused() && IsWindowState(FLAG_WINDOW_TOPMOST)) {
    ClearWindowState(FLAG_WINDOW_TOPMOST);
  }
  update_scene_scene();
  update_time();
  return true;
}

bool app_render(void) {
  if (GetFPS() > TARGET_FPS) return true;
  BeginDrawing();
  ClearBackground(CLEAR_BACKGROUND_COLOR);
  
  BeginMode2D(get_active_metrics()->handle);
  render_scene_world();
  EndMode2D();
  
  render_scene_interface();
  
  EndDrawing();
  return true;
}

bool application_on_event(u16 code, event_context context) {
  switch (code)
  {
  case EVENT_CODE_APPLICATION_QUIT:{
    state->app_runing = context.data.i32[0];
    state->app_runing = false;
    return true;
  }
  case EVENT_CODE_TOGGLE_BORDERLESS: { 
    i32 monitor = GetCurrentMonitor();
    ToggleBorderlessWindowed();
    set_resolution(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
    state->settings.window_state = FLAG_BORDERLESS_WINDOWED_MODE;
    return true;
  }
  case EVENT_CODE_TOGGLE_FULLSCREEN: {
    ToggleFullscreen();
    state->settings.window_state = FLAG_FULLSCREEN_MODE;
    return true;
  }
  case EVENT_CODE_TOGGLE_WINDOWED: {    
    if (state->settings.window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
      ToggleBorderlessWindowed();
      state->settings.window_state = 0;
    }
    else if (state->settings.window_state == FLAG_FULLSCREEN_MODE) {
      ToggleFullscreen();
      state->settings.window_state = 0;
    }
    return true;
  }
  default:
    break;
  }
  return false;
}
