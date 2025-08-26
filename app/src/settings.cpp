#include "settings.h"
#include <raylib.h>
#include <cmath>

#include "core/fmemory.h"
#include "core/logger.h"

#include "tools/lexer_ini.h"

typedef struct app_settings_system_state {
  app_settings settings;
  app_settings initializer;
  u16 offset;
  std::vector<std::pair<i32, i32>> all_supported_resolutions;

  std::vector<std::pair<i32, i32>> ratio_3_2_resolutions;
  std::vector<std::pair<i32, i32>> ratio_4_3_resolutions;
  std::vector<std::pair<i32, i32>> ratio_5_3_resolutions;
  std::vector<std::pair<i32, i32>> ratio_5_4_resolutions;
  std::vector<std::pair<i32, i32>> ratio_8_5_resolutions;
  std::vector<std::pair<i32, i32>> ratio_16_9_resolutions;
  std::vector<std::pair<i32, i32>> ratio_16_10_resolutions;
  std::vector<std::pair<i32, i32>> ratio_21_9_resolutions;
  std::vector<std::pair<i32, i32>> ratio_32_9_resolutions;
  std::vector<std::pair<i32, i32>> ratio_37_27_resolutions;
  std::vector<std::pair<i32, i32>> ratio_43_18_resolutions;
  std::vector<std::pair<i32, i32>> ratio_64_27_resolutions;
  std::vector<std::pair<i32, i32>> ratio_custom_resolutions;

  std::pair<i32, i32> minimum_resolution;

  app_settings_system_state(void) {
    this->settings = app_settings();
    this->initializer = app_settings();
    this->offset = 0u;
    this->all_supported_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_3_2_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_4_3_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_5_3_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_5_4_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_8_5_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_16_9_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_16_10_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_21_9_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_32_9_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_37_27_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_43_18_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_64_27_resolutions = std::vector<std::pair<i32, i32>>();
    this->ratio_custom_resolutions = std::vector<std::pair<i32, i32>>();
    this->minimum_resolution = std::pair<i32, i32>(0, 0);
  }
} app_settings_system_state;

static app_settings_system_state * state = nullptr;

#define TEXT_WINDOWED "windowed"
#define TEXT_BORDERLESS "borderless"
#define TEXT_FULLSCREEN "fullscreen"

bool write_ini_file(app_settings _ini);
bool create_ini_file(i32 width, i32  height, i32 master_volume, i32 window_mode, const char * language);

bool settings_initialize(void) {
  if (state and state != nullptr) {
    return true;
  }
  state = (app_settings_system_state *)allocate_memory_linear(sizeof(app_settings_system_state), true);
  if (not state or state == nullptr) {
    IFATAL("settings::settings_initialize()::State allocation failed");
    return false;
  }
  state->initializer = app_settings();
  state->initializer.master_sound_volume = 0;
  state->initializer.window_state  = 0;
  state->initializer.window_width  = 374;
  state->initializer.window_height = 448;
  state->initializer.render_width  = 374;
  state->initializer.render_height = 448;
  state->initializer.render_width_div2  = state->initializer.render_width  * .5f;
  state->initializer.render_height_div2 = state->initializer.render_height * .5f;
  state->initializer.scale_ratio.push_back(static_cast<f32>(state->settings.render_width)  / static_cast<f32>(state->settings.window_width));
  state->initializer.scale_ratio.push_back(static_cast<f32>(state->settings.render_height) / static_cast<f32>(state->settings.window_height));
  state->initializer.language = DEFAULT_SETTINGS_LANGUAGE; // Recieve from args
  
  // Supported resolutions
  { 
    state->ratio_3_2_resolutions  .push_back(std::pair<i32, i32>(720,   480));
    state->ratio_3_2_resolutions  .push_back(std::pair<i32, i32>(1080,  720));
    state->ratio_3_2_resolutions  .push_back(std::pair<i32, i32>(1440,  960));
    state->ratio_3_2_resolutions  .push_back(std::pair<i32, i32>(2160, 1440));

    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(640,   480));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(800,   600));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(960,   720));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(1024,  768));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(1152,  864));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(1440, 1080));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(1536, 1152));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(2048, 1536));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(3200, 2400));
    state->ratio_4_3_resolutions  .push_back(std::pair<i32, i32>(4096, 3072));

    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(1200,  720));
    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(1280,  768));
    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(1800, 1080));
    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(1920, 1152));
    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(2048, 1229));
    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(3840, 2304));
    state->ratio_5_3_resolutions  .push_back(std::pair<i32, i32>(4096, 2458));

    state->ratio_5_4_resolutions  .push_back(std::pair<i32, i32>(900,   720));
    state->ratio_5_4_resolutions  .push_back(std::pair<i32, i32>(1350, 1080));
    state->ratio_5_4_resolutions  .push_back(std::pair<i32, i32>(1440, 1152));
    state->ratio_5_4_resolutions  .push_back(std::pair<i32, i32>(2024, 1620));
    state->ratio_5_4_resolutions  .push_back(std::pair<i32, i32>(2160, 1728));
    state->ratio_5_4_resolutions  .push_back(std::pair<i32, i32>(2700, 2160));

    state->ratio_8_5_resolutions  .push_back(std::pair<i32, i32>(1280,  800));
    state->ratio_8_5_resolutions  .push_back(std::pair<i32, i32>(1440,  900));
    state->ratio_8_5_resolutions  .push_back(std::pair<i32, i32>(1680, 1050));
    state->ratio_8_5_resolutions  .push_back(std::pair<i32, i32>(1920, 1200));
    state->ratio_8_5_resolutions  .push_back(std::pair<i32, i32>(2560, 1600));
    state->ratio_8_5_resolutions  .push_back(std::pair<i32, i32>(3200, 2048));

    state->ratio_16_9_resolutions .push_back(std::pair<i32, i32>(1280,  720));
    state->ratio_16_9_resolutions .push_back(std::pair<i32, i32>(1366,  768));
    state->ratio_16_9_resolutions .push_back(std::pair<i32, i32>(1920, 1080));
    state->ratio_16_9_resolutions .push_back(std::pair<i32, i32>(2048, 1152));
    state->ratio_16_9_resolutions .push_back(std::pair<i32, i32>(2560, 1440));
    state->ratio_16_9_resolutions .push_back(std::pair<i32, i32>(3840, 2160));

    state->ratio_16_10_resolutions.push_back(std::pair<i32, i32>(1280,  800));
    state->ratio_16_10_resolutions.push_back(std::pair<i32, i32>(1440,  900));
    state->ratio_16_10_resolutions.push_back(std::pair<i32, i32>(1680, 1050));
    state->ratio_16_10_resolutions.push_back(std::pair<i32, i32>(1920, 1200));
    state->ratio_16_10_resolutions.push_back(std::pair<i32, i32>(2560, 1600));

    state->ratio_21_9_resolutions .push_back(std::pair<i32, i32>(1680,  720));
    state->ratio_21_9_resolutions .push_back(std::pair<i32, i32>(2560, 1080));
    state->ratio_21_9_resolutions .push_back(std::pair<i32, i32>(3440, 1440));
    state->ratio_21_9_resolutions .push_back(std::pair<i32, i32>(3840, 1600));

    state->ratio_32_9_resolutions .push_back(std::pair<i32, i32>(2560,  720));
    state->ratio_32_9_resolutions .push_back(std::pair<i32, i32>(3840, 1080));
    state->ratio_32_9_resolutions .push_back(std::pair<i32, i32>(5120, 1440));

    state->ratio_43_18_resolutions.push_back(std::pair<i32, i32>(1680,  720));
    state->ratio_43_18_resolutions.push_back(std::pair<i32, i32>(2560, 1080));
    state->ratio_43_18_resolutions.push_back(std::pair<i32, i32>(3440, 1440));
    state->ratio_43_18_resolutions.push_back(std::pair<i32, i32>(3840, 1600));

    state->ratio_64_27_resolutions.push_back(std::pair<i32, i32>(2560, 1080));
    state->ratio_64_27_resolutions.push_back(std::pair<i32, i32>(5420, 2160));
    state->ratio_64_27_resolutions.push_back(std::pair<i32, i32>(3840, 1600));
  }
  // Supported resolutions

  // All resolutions
  {
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(640,   480));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(720,   480));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(800,   600));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(900,   720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(960,   720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1080,  720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1200,  720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1280,  720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1680,  720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1680,  720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2560,  720));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1024,  768));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1280,  768));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1366,  768));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1280,  800));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1152,  864));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1440,  900));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1440,  960));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1680, 1050));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1350, 1080));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1440, 1080));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1800, 1080));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1920, 1080));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2560, 1080));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3840, 1080));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1440, 1152));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1536, 1152));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1920, 1152));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2048, 1152));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(1920, 1200));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2048, 1229));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2160, 1440));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2560, 1440));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3440, 1440));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(5120, 1440));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2048, 1536));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2560, 1600));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3840, 1600));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2024, 1620));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2160, 1728));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3200, 2048));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(2700, 2160));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3840, 2160));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(5420, 2160));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3840, 2304));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(3200, 2400));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(4096, 2458));
    state->all_supported_resolutions.push_back(std::pair<i32, i32>(4096, 3072));
  }
  // All resolutions

  state->minimum_resolution = std::pair<i32, i32>(640, 480);

  return true;
}

/**
 * @brief Requires Raylib context!
 * @brief For now, we assume it is next to where .exe file
 */
bool set_settings_from_ini_file(const char * file_name) {
  if (not state or state == nullptr) {
    IERROR("settings::set_settings_from_ini_file()::settings state didn't initialized yet");
    return false;
  }
  if (not FileExists(file_name)) {
    write_ini_file(get_default_ini_file());
  }
  if (not parse_app_settings_ini(file_name, __builtin_addressof(state->settings))) {
    IWARN("settings::set_settings_from_ini_file()::parse_app returns false");
    write_ini_file(get_default_ini_file());
    return false;
  }
  state->settings.render_width  = state->settings.window_width;
  state->settings.render_height = state->settings.window_height;
  state->settings.render_width_div2  = state->settings.window_width  * .5f;
  state->settings.render_height_div2 = state->settings.window_height * .5f;
  
  state->settings.scale_ratio.push_back(static_cast<f32>(state->settings.render_width)  / static_cast<f32>(state->settings.window_width));
  state->settings.scale_ratio.push_back(static_cast<f32>(state->settings.render_height) / static_cast<f32>(state->settings.window_height));
  state->settings.display_ratio = get_aspect_ratio(state->settings.window_width, state->settings.window_height);
  state->offset = 5u;
  return true;
}

bool update_app_settings_state(void) {
  if (not state or state == nullptr) {
    IERROR("settings::update_app_settings_state()::State is not valid");
    return false;
  }
  state->settings.master_sound_volume = FCLAMP(state->settings.master_sound_volume, 0, 100);
  return true;
}

void set_resolution(i32 width, i32 height) {
  if (not state or state == nullptr) {
    IERROR("settings::set_resolution()::settings state didn't initialized yet");
    return;
  }
  state->settings.render_width  = width;
  state->settings.render_height = height;
  state->settings.render_width_div2   = state->settings.render_width  * .5f;
  state->settings.render_height_div2  = state->settings.render_height * .5f;
}
void set_window_size(i32 width, i32 height) {
  if (not state or state == nullptr) {
    IERROR("settings::set_resolution()::settings state didn't initialized yet");
    return;
  }
  state->settings.window_width  = width;
  state->settings.window_height = height;
  state->settings.scale_ratio.push_back(static_cast<f32>(state->settings.render_width)  / static_cast<f32>(state->settings.window_width));
  state->settings.scale_ratio.push_back(static_cast<f32>(state->settings.render_height) / static_cast<f32>(state->settings.window_height));
}

void set_language(const char* lang) {
  if (not state or state == nullptr) {
    IERROR("settings::set_language()::State is invalid");
    return;
  }
  if (not lang or lang == nullptr) {
    IWARN("settings::set_language()::Language name is invalid");
    return;
  }
  state->settings.language = lang;
}
bool set_master_sound(i32 volume) {
  if (not state or state == nullptr) {
    IERROR("settings::set_master_sound()::State is invalid");
    return false;
  }
  state->settings.master_sound_volume = volume;
  return true;
}
app_settings * get_app_settings(void) {
  if (not state or state == nullptr) {
    IERROR("settings::get_app_settings()::State is invalid");
    return nullptr;
  }
  return __builtin_addressof(state->settings);
}
i32 get_window_state(void) {
  if (not state or state == nullptr) {
    IERROR("settings::get_window_state()::State is invalid");
    return 0;
  }
  return state->settings.window_state;
}

app_settings get_default_ini_file(void) {
  if (not state or state == nullptr) {
    IERROR("settings::get_default_ini_file()::State is not valid");
    return app_settings();
  }
  app_settings _defaults = app_settings();
  _defaults.window_width  = GetMonitorWidth(GetCurrentMonitor()),
  _defaults.window_height = GetMonitorHeight(GetCurrentMonitor()),
  _defaults.master_sound_volume = DEFAULT_SETTINGS_MASTER_VOLUME;
  _defaults.window_state  = FLAG_BORDERLESS_WINDOWED_MODE;
  _defaults.language = DEFAULT_SETTINGS_LANGUAGE;

  _defaults.render_width  = _defaults.window_width;
  _defaults.render_height = _defaults.window_height;
  _defaults.render_width_div2  = _defaults.render_width  * .5f;
  _defaults.render_height_div2 = _defaults.render_height * .5f;

  _defaults.scale_ratio.push_back(static_cast<f32>(state->settings.render_width)  / static_cast<f32>(state->settings.window_width));
  _defaults.scale_ratio.push_back(static_cast<f32>(state->settings.render_height) / static_cast<f32>(state->settings.window_height));
  _defaults.display_ratio = get_monitor_aspect_ratio();

  return _defaults;
}
bool create_ini_file(i32 width, i32  height, i32 master_volume, i32 window_mode, const char * language) {
  if (not language or language == nullptr) {
    IWARN("settings::create_ini_file()::Language name is invalid");
    return false;
  }
  return write_ini_file(app_settings(window_mode, width, height, master_volume, std::string(language)));
}
bool save_ini_file(void) {
  return create_ini_file(
    state->settings.window_width, state->settings.window_height, 
    0, 
    state->settings.window_state, 
    state->settings.language.c_str()
  );
}

bool write_ini_file(app_settings _settings) { 
  std::string _window_mode = "";
  if (_settings.window_state == 0) {
    _window_mode = "windowed";
  }
  else if (_settings.window_state == FLAG_BORDERLESS_WINDOWED_MODE) {
    _window_mode = "borderless";
  }
  else if (_settings.window_state == FLAG_FULLSCREEN_MODE) {
    _window_mode = "fullscreen";
  }
  else {
    _window_mode = "windowed";
  }

  const char* _ini = TextFormat("%s%s%d%s%s%d%s %s%s%d%s %s%s%s%s %s%s%s%s",
    "[resolution]\n",
    "width = ", _settings.window_width, "\n",
    "height = ", _settings.window_height, "\n",
    "[sound]\n",
    "master = ", _settings.master_sound_volume, "\n",
    "[window]\n",
    "mode = \"", _window_mode.c_str(), "\"\n",
    "[localization]\n",
    "language = \"", _settings.language.c_str(), "\"\n"
  );

  return SaveFileText(CONFIG_FILE_LOCATION, _ini); 
}
aspect_ratio get_window_aspect_ratio(void) {
  return get_aspect_ratio(state->settings.window_width, state->settings.window_height);
}
aspect_ratio get_monitor_aspect_ratio(void) {
  return get_aspect_ratio(GetMonitorWidth(GetCurrentMonitor()),GetMonitorHeight(GetCurrentMonitor()));
}
aspect_ratio get_aspect_ratio(i32 width, i32 height) {
  i32 width_ratio =  0;
  i32 height_ratio = 0;
  i32 monitor_width  = 0;
  i32 monitor_height = 0;
  i32 ratio = 0;
  /**
   * @brief Function from ->  https://medium.com/@hosseini_hasan/aspect-ratio-tutorial-unreal-engine-4-c-3aebdfda54db
   */
  { 
    i32 target_width  = width;
    i32 target_height = height;
    monitor_width  = target_width;
    monitor_height = target_height;

    if (target_width <= 0 || target_height <= 0){ 
      return ASPECT_RATIO_UNDEFINED;
    }

    i32 reminder = 0;
    while (target_height != 0)
    {
      reminder = std::fmod(target_width, target_height);
      target_width = target_height;
      target_height = reminder;
    }
    ratio = target_width;
  }

  width_ratio =  monitor_width  / ratio;
  height_ratio = monitor_height / ratio;

  if(height_ratio == 2) switch (width_ratio) {
    case 3: return ASPECT_RATIO_3_2;      
  }
  else if(height_ratio == 3) switch (width_ratio) {
    case 4: return ASPECT_RATIO_4_3;      
    case 5: return ASPECT_RATIO_5_3;      
  }
  else if(height_ratio == 4) switch (width_ratio) {
    case 5:return ASPECT_RATIO_5_4;      
  }
  else if(height_ratio == 5) switch (width_ratio) {
    case 8: return ASPECT_RATIO_8_5;      
  }
  else if(height_ratio ==  9) switch (width_ratio) {
    case 16: return ASPECT_RATIO_16_9;      
    case 21: return ASPECT_RATIO_21_9;      
    case 32: return ASPECT_RATIO_32_9;      
  }
  else if(height_ratio == 384) switch (width_ratio) {
    case 683: return ASPECT_RATIO_16_9;      
  }
  else if(height_ratio ==  10) switch (width_ratio) {
    case 16: return ASPECT_RATIO_16_10;
  }
  else if(height_ratio ==  18) switch (width_ratio) {
    case 43: return ASPECT_RATIO_43_18;      
  }
  else if(height_ratio ==  27) switch (width_ratio) {
    case 37: return ASPECT_RATIO_37_27;      
    case 64: return ASPECT_RATIO_64_27;      
  }
  else {
    IWARN("settings::get_aspect_ratio()::Unknown resolution. Setting ratio to custom");
    return ASPECT_RATIO_CUSTOM;
  }

  IERROR("settings::get_aspect_ratio()::Function ended unexpectedly");
  return ASPECT_RATIO_UNDEFINED;
}
const app_settings * get_initializer_settings(void) {
  return __builtin_addressof(state->initializer);
}
std::pair<i32, i32> get_optimum_render_resolution(aspect_ratio ratio) {
  switch (ratio) {
    case ASPECT_RATIO_3_2:   return std::pair<i32, i32>(1280, 854  );
    case ASPECT_RATIO_4_3:   return std::pair<i32, i32>(1280, 960  );
    case ASPECT_RATIO_5_3:   return std::pair<i32, i32>(1280, 768  );
    case ASPECT_RATIO_5_4:   return std::pair<i32, i32>(1280, 1024 );
    case ASPECT_RATIO_8_5:   return std::pair<i32, i32>(1280, 800  );
    case ASPECT_RATIO_16_9:  return std::pair<i32, i32>(1280, 720  );
    case ASPECT_RATIO_16_10: return std::pair<i32, i32>(1280, 800  );
    case ASPECT_RATIO_21_9:  return std::pair<i32, i32>(3440, 1440 );
    case ASPECT_RATIO_32_9:  return std::pair<i32, i32>(3840, 1080 );
    case ASPECT_RATIO_37_27: return std::pair<i32, i32>(2560, 1080 );
    case ASPECT_RATIO_43_18: return std::pair<i32, i32>(3440, 1440 );
    case ASPECT_RATIO_64_27: return std::pair<i32, i32>(2560, 1080 );
    case ASPECT_RATIO_CUSTOM: {
      return std::pair<i32, i32>(
        static_cast<f32>(GetMonitorWidth(GetCurrentMonitor())),
        static_cast<f32>(GetMonitorHeight(GetCurrentMonitor()))
      );
    }
    default: {
      IWARN("settings::get_optimum_render_resolution()::Unknown resolution. Setting to custom");
      state->settings.display_ratio = ASPECT_RATIO_CUSTOM;
      return std::pair<i32, i32>(static_cast<f32>(GetMonitorWidth(GetCurrentMonitor())), static_cast<f32>(GetMonitorHeight(GetCurrentMonitor())));
    }
  }
  
  IERROR("settings::get_optimum_render_resolution()::Function ended unexpectedly");
  return std::pair<i32, i32>(-1, -1);
}
std::pair<i32, i32> get_optimum_render_resolution(i32 width, i32 height) {
  const std::vector<std::pair<i32, i32>> *const resolutions = get_supported_render_resolutions();

  i32 nearest_width = state->minimum_resolution.first;
  i32 nearest_height = state->minimum_resolution.second;
  for (const std::pair<i32, i32>& sup_res : (*resolutions)) {
    if (sup_res.first <= width) {
      if (sup_res.first > nearest_width and sup_res.first <= GetMonitorWidth(GetCurrentMonitor()) ) {
        nearest_width = sup_res.first;
      }
    }
    if (sup_res.second <= height and sup_res.second <= GetMonitorHeight(GetCurrentMonitor()) ) {
      if (sup_res.second > nearest_height) {
        nearest_height = sup_res.second;
      }
    }
  }

  return std::pair<i32, i32>(nearest_width, nearest_height);
}
const std::vector<std::pair<i32, i32>> * get_supported_render_resolutions(void) {

  if (state->settings.display_ratio != ASPECT_RATIO_CUSTOM) {
    switch (state->settings.display_ratio) {
      case ASPECT_RATIO_3_2:   return __builtin_addressof(state->ratio_3_2_resolutions);
      case ASPECT_RATIO_4_3:   return __builtin_addressof(state->ratio_4_3_resolutions);
      case ASPECT_RATIO_5_3:   return __builtin_addressof(state->ratio_5_3_resolutions);
      case ASPECT_RATIO_5_4:   return __builtin_addressof(state->ratio_5_4_resolutions);
      case ASPECT_RATIO_8_5:   return __builtin_addressof(state->ratio_8_5_resolutions);
      case ASPECT_RATIO_16_9:  return __builtin_addressof(state->ratio_16_9_resolutions);
      case ASPECT_RATIO_16_10: return __builtin_addressof(state->ratio_16_10_resolutions);
      case ASPECT_RATIO_21_9:  return __builtin_addressof(state->ratio_21_9_resolutions);
      case ASPECT_RATIO_32_9:  return __builtin_addressof(state->ratio_32_9_resolutions);
      case ASPECT_RATIO_37_27: return __builtin_addressof(state->ratio_37_27_resolutions);
      case ASPECT_RATIO_43_18: return __builtin_addressof(state->ratio_43_18_resolutions);
      case ASPECT_RATIO_64_27: return __builtin_addressof(state->ratio_64_27_resolutions);
      default: {
        IWARN("settings::get_supported_render_resolutions()::Unsupported ratio");
        return nullptr;
      }
    }
  }
  else {
    state->ratio_custom_resolutions.clear();
    state->ratio_custom_resolutions.push_back(std::pair<i32, i32>(state->settings.window_width, state->settings.window_height));
    return __builtin_addressof(state->ratio_custom_resolutions);
  }

  IERROR("settings::get_supported_render_resolutions()::Function ended unexpectedly");
  return nullptr;
}

