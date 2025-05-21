#include "settings.h"
#include <raylib.h>

#include "core/fmemory.h"

#include "tools/lexer_ini.h"

typedef struct app_settings_system_state {
  app_settings settings;
  app_settings default_settings;
  u16 offset;
} app_settings_system_state;

static app_settings_system_state *state;

#define TEXT_WINDOWED "windowed"
#define TEXT_BORDERLESS "borderless"
#define TEXT_FULLSCREEN "fullscreen"

bool write_ini_file(char* _ini);
bool create_ini_file(const char* title, u32 width, u32  height, u32 master_volume, i32 window_mode, const char * language);
const char* get_default_ini_file(void);

bool settings_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "settings::settings_initialize()::Called twice");
    return false;
  }
  state = (app_settings_system_state *)allocate_memory_linear(sizeof(app_settings_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "settings::settings_initialize()::State allocation failed");
    return false;
  }

  return true;
}

/**
 * @brief Constantly we assume it is next to where .exe file
 */
bool set_settings_from_ini_file(const char * file_name) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::set_settings_from_ini_file()::settings "
                          "state didn't initialized yet");
    return false;
  }
  if (!FileExists(file_name)) {
    write_ini_file((char*)get_default_ini_file());
  }
  if (!parse_app_settings_ini(file_name, &state->settings)) {
    TraceLog(LOG_WARNING,
             "settings::set_settings_from_ini_file()::parse_app returns false");
    return false;
  }
  state->settings.normalized_ratio = state->settings.window_size.at(0) / BASE_RENDER_RES.x;
  state->settings.scale_ratio.push_back(BASE_RENDER_RES.x / state->settings.window_size.at(0));
  state->settings.scale_ratio.push_back(BASE_RENDER_RES.y / state->settings.window_size.at(1));
  state->offset = 5;

  return true;
}

void set_resolution(u32 width, u32 height) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::set_resolution()::settings state didn't initialized yet");
    return;
  }
  state->settings.window_size = {(f32)width, (f32)height};
  state->settings.normalized_ratio = state->settings.window_size.at(0) / BASE_RENDER_RES.x;
  state->settings.scale_ratio.at(0) = BASE_RENDER_RES.x / state->settings.window_size.at(0);
  state->settings.scale_ratio.at(1) = BASE_RENDER_RES.y / state->settings.window_size.at(1);
  state->offset = 5;
}

void set_language(const char* lang) {
  if (!state) {
    TraceLog(LOG_ERROR, "settings::set_language()::State is not valid");
    return;
  }

  state->settings.language = lang;
}

bool set_master_sound(u32 volume) {
  if (!state) {
    TraceLog(
        LOG_WARNING,
        "settings::set_master_sound()::settings state didn't initialized yet");
    return false;
  }

  state->settings.master_sound_volume = volume;

  return true;
}
app_settings *get_app_settings(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_app_settings()::settings state didn't initialized yet");
    return 0;
  }

  return &state->settings;
}
i32 get_window_state(void) {
  if (!state) {
    TraceLog(LOG_WARNING,"settings::get_window_state()::settings state didn't initialized yet");
    return 0;
  }

  return state->settings.window_state;
}

const char* get_default_ini_file(void) {
  return 
  "[title]\n"
  "title = \"" GAME_TITLE "\"\n"
  "[resolution]\n"
  "width = " DEFAULT_SETTINGS_WINDOW_WIDTH "\n"
  "height = " DEFAULT_SETTINGS_WINDOW_HEIGHT "\n"
  "[sound]\n"
  "master = " DEFAULT_SETTINGS_MASTER_VOLUME "\n"
  "[window]\n"
  "mode = \"" DEFAULT_SETTINGS_WINDOW_MODE "\"\n"
  "[localization]\n"
  "language = \"" DEFAULT_SETTINGS_LANGUAGE "\"\n";
}
bool create_ini_file(const char* title, u32 width, u32  height, u32 master_volume, i32 window_mode, const char * language) {
  
  std::string _window_mode = "";
  if (window_mode == 0) {
    _window_mode = "windowed";
  }
  else if (window_mode == FLAG_BORDERLESS_WINDOWED_MODE) {
    _window_mode = "borderless";
  }
  else if (window_mode == FLAG_FULLSCREEN_MODE) {
    _window_mode = "fullscreen";
  }
  else {
    _window_mode = "windowed";
  }

  const char* _ini = TextFormat("%s%s%s%s %s%s%d%s%s%d%s %s%s%d%s %s%s%s%s %s%s%s%s",
    "[title]\n",
    "title = \"", title, "\"\n",
    "[resolution]\n",
    "width = ", width, "\n",
    "height = ", height, "\n",
    "[sound]\n",
    "master = ", master_volume, "\n",
    "[window]\n",
    "mode = \"", _window_mode.c_str(), "\"\n",
    "[localization]\n",
    "language = \"", language, "\"\n"
  );

  return write_ini_file((char *)_ini);
}
bool write_ini_file(char* _ini) {

  return SaveFileText(CONFIG_FILE_LOCATION, _ini);
}
bool save_ini_file(void) {
  return create_ini_file(
    GAME_TITLE, 
    state->settings.window_size.at(0), state->settings.window_size.at(1), 
    0, 
    state->settings.window_state, 
    state->settings.language.c_str()
  );
}
