#include "app.h"
#include "game/game_types.h"

#include "settings.h"
#include "sound.h"

#include "tools/loc_parser.h"
#include "tools/pak_parser.h"

#include "core/event.h"
#include "core/ftime.h"
#include "core/fmemory.h"
#include "core/logger.h"

#include "game/resource.h"
#include "game/scenes/scene_manager.h"
#include "game/world.h"
#include "game/fshader.h"

typedef struct app_system_state {
  bool app_running {};
  app_settings* settings;
  RenderTexture2D drawing_target;
  Camera2D screen_space_camera;
  const fshader* post_process_shader;
  app_system_state(void) {
    this->settings = nullptr;
    this->drawing_target = RenderTexture2D { 0u, ZERO_TEXTURE, ZERO_TEXTURE};
    this->screen_space_camera = Camera2D {ZEROVEC2, ZEROVEC2, 0.f, 0.f};
    this->post_process_shader = nullptr;
  }
} app_system_state;

static app_system_state* state = nullptr;

bool application_on_event(i32 code, event_context context);

constexpr void toggle_borderless(void);
constexpr void toggle_fullscreen(void);
constexpr void toggle_windowed(i32 width, i32 height);

bool app_initialize(i32 build_id) {
  // Subsystems
  memory_system_initialize();
  if(not event_system_initialize()) {
    alert("Event system init failed", "Fatal");
    return false;
  }
  if (not time_system_initialize()) {
    alert("Time system init failed", "Fatal");
    return false;
  }

  state = (app_system_state*)allocate_memory_linear(sizeof(app_system_state), true);
  if (not state or state == nullptr) {
    alert("App state allocation failed", "Fatal");
    return false;
  }
  *state = app_system_state();
  
  // Platform    
  if(not settings_initialize()) {
    alert("Settings state init failed", "Fatal");
    return false; // TODO: Set default settings instead
  }
  const app_settings * initializer = get_initializer_settings();
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_UNDECORATED);
  InitWindow(initializer->window_width ,initializer->window_height, GAME_TITLE);

  if(not logging_system_initialize(build_id)) { // INFO: Requires Raylib to file system operations
    alert("Logging system init failed", "Fatal");
    return false;
  }
  set_settings_from_ini_file(CONFIG_FILE_LOCATION);
  state->settings = get_app_settings();

	if (not pak_parser_system_initialize()) {
  	IFATAL("app::app_initialize()::Cannot initialize par parser");
  	return false;
	}

  // Game
  #if USE_PAK_FORMAT 
    const file_buffer * loading_thumbnail = fetch_asset_file_buffer(PAK_FILE_ASSET1, PAK_FILE_ASSET1_THUMBNAIL);
    if (loading_thumbnail and loading_thumbnail != nullptr) {
      Image loading_image = LoadImageFromMemory(".png",
		  	reinterpret_cast<const u8 *>(loading_thumbnail->content.data()), 
		  	loading_thumbnail->content.size()
		  );
      Texture loading_tex = LoadTextureFromImage(loading_image);
      
      BeginDrawing();
      ClearBackground(CLEAR_BACKGROUND_COLOR); //TODO: Loading screen
      DrawTexturePro(loading_tex, 
        Rectangle {0.f, 0.f, static_cast<f32>(loading_image.width), static_cast<f32>(loading_image.height)}, 
        Rectangle {0.f, 0.f, static_cast<f32>(GetScreenWidth()), static_cast<f32>(GetScreenHeight())}, ZEROVEC2, 0.f, WHITE
		  );
      EndDrawing();
      UnloadImage(loading_image);
      UnloadTexture(loading_tex);
    }


  #else
    Texture loading_tex = LoadTexture(RESOURCE_PATH "aaa_game_start_loading_screen.png");

    BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR); //TODO: Loading screen
    DrawTexturePro(loading_tex, 
      {0, 0, (f32)loading_tex.width, (f32)loading_tex.height}, 
      {0, 0, (f32)GetScreenWidth(),  (f32)GetScreenHeight()}, Vector2{0.f, 0.f}, 0, WHITE);
    EndDrawing();
    UnloadTexture(loading_tex);
  #endif
  
  parse_asset_pak(PAK_FILE_ASSET1);
  parse_asset_pak(PAK_FILE_ASSET2);
  parse_map_pak();
  pak_parser_drop_pak_data(PAK_FILE_ASSET1);
  pak_parser_drop_pak_data(PAK_FILE_ASSET2);
  pak_parser_drop_map_pak_data();

  if (not loc_parser_system_initialize()) {
    IFATAL("app::app_initialize()::Localization system init failed");
    return false; // TODO: Set default language instead
  }
  _loc_parser_parse_localization_data();

  if(not resource_system_initialize()) {
    IFATAL("app::app_initialize()::Resource system init failed");
    return false;
  }
  if(not sound_system_initialize()) {
    IFATAL("app::app_initialize()::Sound system init failed");
    return false;
  }

  if(not world_system_initialize(state->settings)) {
    IFATAL("app::app_initialize()::World initialize failed");
    return false;
  }
  if(not scene_manager_initialize(state->settings)) {
    IFATAL("app::app_initialize()::Scene manager initialize failed");
    return false;
  }
  if (not initialize_shader_system()) {
    IFATAL("app::app_initialize()::Shader system initialization failed");
    return false;
  }
  state->post_process_shader = get_shader_by_enum(SHADER_ID_POST_PROCESS);

  event_register(EVENT_CODE_APPLICATION_QUIT, application_on_event);
  event_register(EVENT_CODE_TOGGLE_BORDERLESS, application_on_event);
  event_register(EVENT_CODE_TOGGLE_FULLSCREEN, application_on_event);
  event_register(EVENT_CODE_TOGGLE_WINDOWED, application_on_event);
  event_register(EVENT_CODE_SET_POST_PROCESS_FADE_VALUE, application_on_event);

  state->drawing_target = LoadRenderTexture(state->settings->render_width, state->settings->render_height);

  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

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
  if (not state or state == nullptr) {
    IFATAL("app::app_update()::State is invalid");
    return false;
  }
  update_app_settings_state();

  if ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_W)) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_F4))) {
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
  update_time();
  return true;
}

bool app_render(void) {
  BeginTextureMode(state->drawing_target);
    ClearBackground(CLEAR_BACKGROUND_COLOR);

    render_scene_world();
    render_scene_interface();
    
    DrawFPS(state->settings->render_width * .8f , state->settings->render_height - SCREEN_OFFSET.y * 5.f );
  EndTextureMode();
  
  state->screen_space_camera.target = Vector2 {0.f, 0.f};
  state->screen_space_camera.zoom = 1.f;
  BeginDrawing();
    ClearBackground(CLEAR_BACKGROUND_COLOR);
    BeginMode2D(state->screen_space_camera);
      Rectangle source = Rectangle {0.f, 0.f, (f32)state->drawing_target.texture.width, (f32)-state->drawing_target.texture.height};
      Rectangle dest = Rectangle {0.f, 0.f, static_cast<f32>(state->settings->window_width), static_cast<f32>(-state->settings->window_height)
      };

    BeginShaderMode(get_shader_by_enum(SHADER_ID_POST_PROCESS)->handle);
      DrawTexturePro(state->drawing_target.texture, source, dest, ZEROVEC2, 0.f, WHITE);
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
  SetWindowPosition(0, 0);

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
  SetWindowPosition(0, 0);

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
  std::pair<i32, i32> new_res = get_optimum_render_resolution(width, height);

  state->settings->window_state = 0;
  set_resolution(new_res.first, new_res.second);  
  set_window_size(new_res.first, new_res.second);
  event_fire(EVENT_CODE_CAMERA_SET_DRAWING_EXTENT, event_context(state->settings->render_width, state->settings->render_height));
  
  UnloadRenderTexture(state->drawing_target);
  state->drawing_target = LoadRenderTexture(state->settings->render_width, state->settings->render_height);


  SetWindowSize(new_res.first, new_res.second);
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
    set_shader_uniform(SHADER_ID_POST_PROCESS, "fade", data128(static_cast<f32>(context.data.f32[0])));
    return true;
  }
  default:
    break;
  }
  return false;
}
