#include "app.h"
#include "game/game_types.h"

#include "settings.h"
#include "sound.h"

#include "tools/loc_parser.h"
#if USE_PAK_FORMAT 
  #include "tools/pak_parser.h"
#endif

#include "core/event.h"
#include "core/ftime.h"
#include "core/fmemory.h"
//#include "core/logger.h"

#include "game/camera.h"
#include "game/resource.h"
#include "game/scenes/scene_manager.h"
#include "game/world.h"

typedef struct app_system_state {
  app_settings* settings;
  bool app_runing;
  RenderTexture2D drawing_target;
  Camera2D screen_space_camera;
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
  set_settings_from_ini_file(CONFIG_FILE_LOCATION);
  state->settings = get_app_settings();
  
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(
    state->settings->window_size.at(0), 
    state->settings->window_size.at(1), 
    GAME_TITLE);
  SetTargetFPS(TARGET_FPS); 
  SetExitKey(KEY_END);
  if (state->settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
    ToggleBorderlessWindowed();
    Vector2 res = {(f32)GetMonitorWidth(GetCurrentMonitor()), (f32)GetMonitorHeight(GetCurrentMonitor())};
    SetWindowSize(res.x, res.y);
    set_resolution(res.x, res.y);
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
  }
  else if (state->settings->window_state == FLAG_FULLSCREEN_MODE) {
    ToggleFullscreen();
    Vector2 res = {(f32)GetMonitorWidth(GetCurrentMonitor()), (f32)GetMonitorHeight(GetCurrentMonitor())};
    SetWindowSize(res.x, res.y);
    set_resolution(res.x, res.y);
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST);
  }
  state->drawing_target = LoadRenderTexture(BASE_RENDER_RES.x, BASE_RENDER_RES.y);
  // Game
  #if USE_PAK_FORMAT 
    pak_parser_system_initialize(PAK_FILE_LOCATION);
    file_data loading_image_file = fetch_file_data("aaa_game_start_loading_screen.png");
    Image loading_image = LoadImageFromMemory(".png", loading_image_file.data, loading_image_file.size);
    Texture loading_tex = LoadTextureFromImage(loading_image);

    BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR); //TODO: Loading screen
    DrawTexturePro(loading_tex, 
      {0, 0, loading_image.width, loading_image.height}, 
      {0, 0, GetScreenWidth(), GetScreenHeight()}, Vector2{}, 0, WHITE);
    EndDrawing();

    parse_pak();
  #else
    Texture loading_tex = LoadTexture(RESOURCE_PATH "aaa_game_start_loading_screen.png");

    BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR); //TODO: Loading screen
    DrawTexturePro(loading_tex, 
      {0, 0, (f32)loading_tex.width, (f32)loading_tex.height}, 
      {0, 0, (f32)GetScreenWidth(),  (f32)GetScreenHeight()}, Vector2{}, 0, WHITE);
    EndDrawing();
  #endif

  if (!loc_parser_system_initialize()) {
    TraceLog(LOG_WARNING, "app::app_initialize()::Localization system init failed");
  }
  _loc_parser_parse_localization_data_from_file(state->settings->language.c_str());

  resource_system_initialize();
  sound_system_initialize();
  create_camera(BASE_RENDER_SCALE(.5f));

  world_system_initialize(get_in_game_camera());
  scene_manager_initialize();

  event_register(EVENT_CODE_APPLICATION_QUIT, application_on_event);
  event_register(EVENT_CODE_TOGGLE_BORDERLESS, application_on_event);
  event_register(EVENT_CODE_TOGGLE_FULLSCREEN, application_on_event);
  event_register(EVENT_CODE_TOGGLE_WINDOWED, application_on_event);

  state->app_runing = true;

  save_ini_file();
  return true;
}

bool window_should_close(void) {
  return state->app_runing;
}

bool app_update(void) {
  if (GetFPS() > TARGET_FPS) return true;
  if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_W)) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_F4))) {
    // TODO: handle destruction ops
    state->app_runing = false;
  }
  if (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER)) {
   event_fire(EVENT_CODE_TOGGLE_BORDERLESS, event_context {});
  }
  
  if(IsWindowFocused() && !IsWindowState(FLAG_WINDOW_TOPMOST) && state->settings->window_state != 0) {
    SetWindowState(FLAG_WINDOW_TOPMOST);
  }
  else if(!IsWindowFocused() && IsWindowState(FLAG_WINDOW_TOPMOST)) {
    ClearWindowState(FLAG_WINDOW_TOPMOST);
  }
  update_scene_scene();
  update_sound_system();
  update_time();
  return true;
}

bool app_render(void) {
  if (GetFPS() > TARGET_FPS) return true;

  BeginTextureMode(state->drawing_target);
    ClearBackground(CLEAR_BACKGROUND_COLOR);

    BeginMode2D(get_in_game_camera()->handle);
      render_scene_world();
    EndMode2D();

    render_scene_interface();
  EndTextureMode();
  
  state->screen_space_camera.target = Vector2 {};
  state->screen_space_camera.zoom = 1.f;
  BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR);
    BeginMode2D(state->screen_space_camera);
      DrawTexturePro(state->drawing_target.texture, 
        {0, 0, (f32)state->drawing_target.texture.width, (f32)-state->drawing_target.texture.height},
        {
          -state->settings->normalized_ratio, -state->settings->normalized_ratio, 
          state->settings->window_size.at(0) + (state->settings->normalized_ratio*2), 
          state->settings->window_size.at(1) + (state->settings->normalized_ratio*2)
        }, Vector2 {}, 0, WHITE
      );
    EndMode2D();
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
    Vector2 res = {(f32)GetMonitorWidth(monitor), (f32)GetMonitorHeight(monitor)};
    ToggleBorderlessWindowed();
    SetWindowSize(res.x, res.y);
    set_resolution(res.x, res.y);
    state->settings->window_state = FLAG_BORDERLESS_WINDOWED_MODE;
    return true;
  }
  case EVENT_CODE_TOGGLE_FULLSCREEN: {
    i32 monitor = GetCurrentMonitor();
    Vector2 res = {(f32)GetMonitorWidth(monitor), (f32)GetMonitorHeight(monitor)};
    ToggleFullscreen();
    SetWindowSize(res.x, res.y);
    set_resolution(res.x, res.y);
    state->settings->window_state = FLAG_FULLSCREEN_MODE;
    return true;
  }
  case EVENT_CODE_TOGGLE_WINDOWED: {    
    if (state->settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
      ToggleBorderlessWindowed();
      state->settings->window_state = 0;
    }
    else if (state->settings->window_state == FLAG_FULLSCREEN_MODE) {
      ToggleFullscreen();
      state->settings->window_state = 0;
    }
    SetWindowSize(get_app_settings()->window_size.at(0), get_app_settings()->window_size.at(1));
    SetWindowPosition(
      (GetMonitorWidth(GetCurrentMonitor())  / 2.f) - (GetScreenWidth()  / 2.f), 
      (GetMonitorHeight(GetCurrentMonitor()) / 2.f) - (GetScreenHeight() / 2.f)
    );
    if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
      ClearWindowState(FLAG_WINDOW_UNDECORATED);
    }
    return true;
  }
  default:
    break;
  }
  return false;
}
