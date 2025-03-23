#include "settings.h"
#include "defines.h"

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

  state->settings.resolution_5div4.x   = state->settings.resolution[0] / 1.25f;
  state->settings.resolution_5div4.y   = state->settings.resolution[1] / 1.25f;
  state->settings.resolution_3div2.x   = state->settings.resolution[0] / 1.50f;
  state->settings.resolution_3div2.y   = state->settings.resolution[1] / 1.50f;
  state->settings.resolution_35div20.x = state->settings.resolution[0] / 1.75f;
  state->settings.resolution_35div20.y = state->settings.resolution[1] / 1.75f;
  state->settings.resolution_38div20.x = state->settings.resolution[0] / 1.90f;
  state->settings.resolution_38div20.y = state->settings.resolution[1] / 1.90f;
  state->settings.resolution_div2.x    = state->settings.resolution[0] / 2.f;
  state->settings.resolution_div2.y    = state->settings.resolution[1] / 2.f;
  state->settings.resolution_div3.x    = state->settings.resolution[0] / 3.f;
  state->settings.resolution_div3.y    = state->settings.resolution[1] / 3.f;
  state->settings.resolution_div4.x    = state->settings.resolution[0] / 4.f;
  state->settings.resolution_div4.y    = state->settings.resolution[1] / 4.f;
  state->offset = 5;

  return true;
}

bool set_resolution(u32 width, u32 height) {
  if (!state) {
    TraceLog(
        LOG_WARNING,
        "settings::set_resolution()::settings state didn't initialized yet");
    return false;
  }

  state->settings.resolution[0] = width;
  state->settings.resolution[1] = height;
  state->settings.resolution_5div4.x   = state->settings.resolution[0] / 1.25f;
  state->settings.resolution_5div4.y   = state->settings.resolution[1] / 1.25f;
  state->settings.resolution_3div2.x   = state->settings.resolution[0] / 1.50f;
  state->settings.resolution_3div2.y   = state->settings.resolution[1] / 1.50f;
  state->settings.resolution_35div20.x = state->settings.resolution[0] / 1.75f;
  state->settings.resolution_35div20.y = state->settings.resolution[1] / 1.75f;
  state->settings.resolution_38div20.x = state->settings.resolution[0] / 1.90f;
  state->settings.resolution_38div20.y = state->settings.resolution[1] / 1.90f;
  state->settings.resolution_div2.x    = state->settings.resolution[0] / 2.f;
  state->settings.resolution_div2.y    = state->settings.resolution[1] / 2.f;
  state->settings.resolution_div3.x    = state->settings.resolution[0] / 3.f;
  state->settings.resolution_div3.y    = state->settings.resolution[1] / 3.f;
  state->settings.resolution_div4.x    = state->settings.resolution[0] / 4.f;
  state->settings.resolution_div4.y    = state->settings.resolution[1] / 4.f;

  return true;
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
    TraceLog(
        LOG_WARNING,
        "settings::get_app_settings()::settings state didn't initialized yet");
    return (app_settings *){0};
  }

  return &state->settings;
}
i32 get_window_state(void) {
  if (!state) {
    TraceLog(
        LOG_WARNING,
        "settings::get_window_state()::settings state didn't initialized yet");
    return 0;
  }

  return state->settings.window_state;
}
Vector2* get_resolution_div2(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div2()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }
  return &state->settings.resolution_div2;
}
Vector2* get_resolution_div3(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div3()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }
  return &state->settings.resolution_div3;
}
Vector2* get_resolution_div4(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div4()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }

  return &state->settings.resolution_div4;
}
Vector2* get_resolution_3div2(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div2()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }
  return &state->settings.resolution_3div2;
}
Vector2* get_resolution_5div4(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div3()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }
  return &state->settings.resolution_5div4;
}
Vector2* get_resolution_35div20(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div4()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }

  return &state->settings.resolution_35div20;
}
Vector2* get_resolution_38div20(void) {
  if (!state) {
    TraceLog(LOG_WARNING, "settings::get_resolution_div4()::settings state "
                          "didn't initialized yet");
    return (Vector2*){0};
  }

  return &state->settings.resolution_38div20;
}
Vector2 get_screen_offset(void) {
  if (!state) {
    TraceLog(
        LOG_WARNING,
        "settings::get_screen_offset()::settings state didn't initialized yet");
    return (Vector2) {0, 0};
  }

  return (Vector2) {state->offset, state->offset};
}

bool create_ini_file(void) {

  return SaveFileText(CONFIG_FILE_LOCATION, 
  "[title]\n"
  "title = \"Hello No\"\n"
  "[resolution]\n"
  "width = 1280\n"
  "height = 720\n"
  "[sound]\n"
  "master = 100\n"
  "[window]\n"
  "mode = \"windowed\"\n"
  );
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

  const char* ini_text = TextFormat("%s %s%s%s %s %s%d%s %s%d%s %s %s %s %s%s%s",
    "[title]\n",
    "title = \"", state->settings.title, "\"\n",
    "[resolution]\n",
    "width = \"", state->settings.resolution[0], "\"\n",
    "height = \"", state->settings.resolution[1], "\"\n",
    "[sound]\n",
    "master = 100\n",
    "[window]\n",
    "mode = \"", windows_state_char, "\"\n"
  );

  return SaveFileText(CONFIG_FILE_LOCATION, (char*)ini_text);
}
