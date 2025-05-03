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

bool create_ini_file(void);

void settings_initialize(void) {
  if (state) {
    TraceLog(LOG_ERROR,
             "settings::settings_initialize()::Called twice. Aborted");
    return;
  }
  state = (app_settings_system_state *)allocate_memory_linear(sizeof(app_settings_system_state), true);
}

/**
 * @brief Constantly we assume it is next to where .exe file
 */
bool set_settings_from_ini_file(const char *file_name) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::set_settings_from_ini_file()::settings "
                          "state didn't initialized yet");
    return false;
  }
  if (!FileExists(file_name)) {
    create_ini_file();
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

bool create_ini_file(void) {
  const char* _ini = 
  "[title]\n"
  "title = \"Hello No\"\n"
  "[resolution]\n"
  "width = 1280\n"
  "height = 720\n"
  "[sound]\n"
  "master = 100\n"
  "[window]\n"
  "mode = \"windowed\"\n";

  return SaveFileText(CONFIG_FILE_LOCATION, (char *)_ini);
}
bool save_ini_file(void) {

  i8 windows_state_char[12] = {0};
  if (state->settings.window_state == 0) {
    copy_memory(windows_state_char, TEXT_WINDOWED, TextLength(TEXT_WINDOWED));
  }
  else if (state->settings.window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
    copy_memory(windows_state_char, TEXT_BORDERLESS, TextLength(TEXT_BORDERLESS));
  }
  else if (state->settings.window_state == FLAG_FULLSCREEN_MODE) {
    copy_memory(windows_state_char, TEXT_FULLSCREEN, TextLength(TEXT_FULLSCREEN));
  }

  const char* ini_text = TextFormat("%s %s%.0f%s %s%.0f%s %s %s %s %s%s%s",
    "[resolution]\n",
    "width = \"", state->settings.window_size.at(0), "\"\n",
    "height = \"", state->settings.window_size.at(1), "\"\n",
    "[sound]\n",
    "master = 100\n",
    "[window]\n",
    "mode = \"", windows_state_char, "\"\n"
  );

  return SaveFileText(CONFIG_FILE_LOCATION, (char*)ini_text);
}
