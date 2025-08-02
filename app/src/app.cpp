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

#include "game/resource.h"
#include "game/scenes/scene_manager.h"
#include "game/world.h"
#include "game/fshader.h"

typedef struct app_system_state {
  app_settings* settings;
  bool app_running;
  RenderTexture2D drawing_target;
  Camera2D screen_space_camera;
  fshader* post_process_shader;
} app_system_state;

static app_system_state* state;

bool application_on_event(i32 code, event_context context);

constexpr void toggle_borderless(void);
constexpr void toggle_fullscreen(void);
constexpr void toggle_windowed(i32 width, i32 height);

bool app_initialize(void) {
  // Subsystems
  memory_system_initialize();
  event_system_initialize();
  time_system_initialize();
  state = (app_system_state*)allocate_memory_linear(sizeof(app_system_state), true);
  if (!state) {
    TraceLog(LOG_FATAL, "app::app_initialize()::App state allocation has failed");
    return false;
  }
  
  // Platform    
  if(!settings_initialize()) {
    TraceLog(LOG_ERROR, "app::app_initialize()::Settings state allocation has failed");
    return false; // TODO: Set default settings instead
  }
  app_settings * initializer = get_initializer_settings();
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_UNDECORATED);
  InitWindow(initializer->window_width ,initializer->window_height, GAME_TITLE);

  set_settings_from_ini_file(CONFIG_FILE_LOCATION);
  state->settings = get_app_settings();

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
      {0, 0, GetScreenWidth(), GetScreenHeight()}, Vector2{0.f, 0.f}, 0, WHITE);
    EndDrawing();

    parse_pak();
  #else
    Texture loading_tex = LoadTexture(RESOURCE_PATH "aaa_game_start_loading_screen.png");

    BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR); //TODO: Loading screen
    DrawTexturePro(loading_tex, 
      {0, 0, (f32)loading_tex.width, (f32)loading_tex.height}, 
      {0, 0, (f32)GetScreenWidth(),  (f32)GetScreenHeight()}, Vector2{0.f, 0.f}, 0, WHITE);
    EndDrawing();
  #endif

  if (!loc_parser_system_initialize()) {
    TraceLog(LOG_WARNING, "app::app_initialize()::Localization system init failed");
    return false; // TODO: Set default language instead
  }
  _loc_parser_parse_localization_data();

  if(!resource_system_initialize()) {
    TraceLog(LOG_WARNING, "app::app_initialize()::Resource system init failed");
    return false;
  }
  if(!sound_system_initialize()) {
    TraceLog(LOG_WARNING, "app::app_initialize()::Sound system init failed");
    return false;
  }

  if(!world_system_initialize(state->settings)) {
    TraceLog(LOG_WARNING, "app::app_initialize()::World initialize failed");
    return false;
  }
  if(!scene_manager_initialize(state->settings)) {
    TraceLog(LOG_WARNING, "app::app_initialize()::Scene manager initialize failed");
    return false;
  }
  if (!initialize_shader_system()) {
    TraceLog(LOG_WARNING, "app::app_initialize()::Shader system initialization failed");
    return false;
  }
  state->post_process_shader = get_shader_by_enum(SHADER_ID_POST_PROCESS);

  event_register(EVENT_CODE_APPLICATION_QUIT, application_on_event);
  event_register(EVENT_CODE_TOGGLE_BORDERLESS, application_on_event);
  event_register(EVENT_CODE_TOGGLE_FULLSCREEN, application_on_event);
  event_register(EVENT_CODE_TOGGLE_WINDOWED, application_on_event);
  event_register(EVENT_CODE_SET_POST_PROCESS_FADE_VALUE, application_on_event);

  state->drawing_target = LoadRenderTexture(state->settings->render_width, state->settings->render_height);

  SetTargetFPS(TARGET_FPS); // TODO: SetTargetFPS() Doesn't work
  SetExitKey(KEY_END);
  if (state->settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
    toggle_borderless();
  }
  else if (state->settings->window_state == FLAG_FULLSCREEN_MODE) {
    toggle_fullscreen();
  }
  else {
    toggle_windowed(state->settings->window_width, state->settings->window_height);
  }

  event_fire(EVENT_CODE_SET_POST_PROCESS_FADE_VALUE, event_context(1.f));

  state->app_running = true;
  save_ini_file();
  return true;
}

bool window_should_close(void) {
  return state->app_running;
}

bool app_update(void) {
  //if (GetFPS() > TARGET_FPS) return true;

  if (!state) {
    TraceLog(LOG_FATAL, "app::app_update()::App state is not valid!");
    return false;
  }
  
  update_app_settings_state();

  if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_W)) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_F4))) {
    // TODO: handle destruction ops
    state->app_running = false;
  }
  if (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER)) {
    if (state->settings->window_state == 0) {
      event_fire(EVENT_CODE_TOGGLE_BORDERLESS, event_context());
    }
    else {
      event_fire(EVENT_CODE_TOGGLE_WINDOWED, event_context(
        get_optimum_render_resolution(state->settings->display_ratio).first, 
        get_optimum_render_resolution(state->settings->display_ratio).second
      ));
    }
  }
  if(IsWindowFocused() && !IsWindowState(FLAG_WINDOW_TOPMOST) && state->settings->window_state != 0) {
    SetWindowState(FLAG_WINDOW_TOPMOST);
  }
  else if(!IsWindowFocused() && IsWindowState(FLAG_WINDOW_TOPMOST)) {
    ClearWindowState(FLAG_WINDOW_TOPMOST);
  }
  if (state->settings->window_width  != GetMonitorWidth(GetCurrentMonitor()) || state->settings->window_height != GetMonitorHeight(GetCurrentMonitor())) {
    if (state->settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
      toggle_borderless();
    }
    if (state->settings->window_state == FLAG_FULLSCREEN_MODE) {
      toggle_fullscreen();
    }
  }

  update_scene_scene();
  update_sound_system();
  update_time();
  return true;
}

bool app_render(void) {
  //if (GetFPS() > TARGET_FPS) return true;

  BeginTextureMode(state->drawing_target);
    ClearBackground(CLEAR_BACKGROUND_COLOR);

    render_scene_world();
    render_scene_interface();
    
    DrawFPS(state->settings->render_width * .75f, SCREEN_OFFSET.y * 10);
  EndTextureMode();
  
  state->screen_space_camera.target = Vector2 {0.f, 0.f};
  state->screen_space_camera.zoom = 1.f;
  BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR);
    BeginMode2D(state->screen_space_camera);
      Rectangle source = Rectangle {0, 0, (f32)state->drawing_target.texture.width, (f32)-state->drawing_target.texture.height};
      Rectangle dest = Rectangle {0, 0, static_cast<f32>(state->settings->window_width), static_cast<f32>(-state->settings->window_height)
      };

    BeginShaderMode(get_shader_by_enum(SHADER_ID_POST_PROCESS)->handle);
      DrawTexturePro(state->drawing_target.texture, source, dest, ZEROVEC2, 0, WHITE);
    EndShaderMode();

    EndMode2D();
  EndDrawing();
  return true;
}

constexpr void toggle_borderless(void) {
  if (IsWindowState(FLAG_FULLSCREEN_MODE)) {
    ClearWindowState(FLAG_FULLSCREEN_MODE);
  }

  set_resolution(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
  set_window_size(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
  event_fire(EVENT_CODE_CAMERA_SET_DRAWING_EXTENT, event_context(state->settings->render_width, state->settings->render_height));
  UnloadRenderTexture(state->drawing_target);
  state->drawing_target = LoadRenderTexture(state->settings->render_width, state->settings->render_height);
  SetWindowSize(state->settings->window_width, state->settings->window_height);

  ToggleBorderlessWindowed();
  state->settings->window_state = FLAG_BORDERLESS_WINDOWED_MODE;
}
constexpr void toggle_fullscreen(void) {
  if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
    ClearWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
  }

  set_resolution(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
  set_window_size(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
  event_fire(EVENT_CODE_CAMERA_SET_DRAWING_EXTENT, event_context(state->settings->render_width, state->settings->render_height));
  UnloadRenderTexture(state->drawing_target);
  state->drawing_target = LoadRenderTexture(state->settings->render_width, state->settings->render_height);
  SetWindowSize(state->settings->window_width, state->settings->window_height);

  ToggleFullscreen();
  state->settings->window_state = FLAG_FULLSCREEN_MODE;
}
constexpr void toggle_windowed(i32 width, i32 height) {
  if (state->settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
    ToggleBorderlessWindowed();
  }
  else if (IsWindowFullscreen()) {
    ToggleFullscreen();
  }
  state->settings->window_state = 0;
  set_resolution(width, height);  
  set_window_size(width, height);
  event_fire(EVENT_CODE_CAMERA_SET_DRAWING_EXTENT, event_context(state->settings->render_width, state->settings->render_height));
  
  UnloadRenderTexture(state->drawing_target);
  state->drawing_target = LoadRenderTexture(state->settings->render_width, state->settings->render_height);

  SetWindowSize(state->settings->window_width, state->settings->window_height);
  SetWindowPosition(
    (GetMonitorWidth(GetCurrentMonitor())  / 2.f) - (GetScreenWidth()  / 2.f), 
    (GetMonitorHeight(GetCurrentMonitor()) / 2.f) - (GetScreenHeight() / 2.f)
  );
  if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
    ClearWindowState(FLAG_WINDOW_UNDECORATED);
  }
  if (IsWindowState(FLAG_WINDOW_TRANSPARENT)) {
    ClearWindowState(FLAG_WINDOW_TRANSPARENT);
  }
}

bool application_on_event(i32 code, event_context context) {
  switch (code)
  {
  case EVENT_CODE_APPLICATION_QUIT:{
    state->app_running = context.data.i32[0];
    state->app_running = false;
    return true;
  }
  case EVENT_CODE_TOGGLE_BORDERLESS: {
    toggle_borderless();
    return true;
  }
  case EVENT_CODE_TOGGLE_FULLSCREEN: {
    toggle_fullscreen();
    return true;
  }
  case EVENT_CODE_TOGGLE_WINDOWED: {
    toggle_windowed(context.data.i32[0], context.data.i32[1]);
    return true;
  }
  case EVENT_CODE_SET_POST_PROCESS_FADE_VALUE: {
    i32 process_uniform_loc = GetShaderLocation(state->post_process_shader->handle, "process");
    set_shader_uniform(SHADER_ID_POST_PROCESS, process_uniform_loc, data128(static_cast<f32>(context.data.f32[0])));
    return true;
  }
  default:
    break;
  }
  return false;
}
