#include "user_interface.h"
#include <reasings.h>
#include <settings.h>
#include "loc_types.h"

#include <tools/pak_parser.h>

#include "core/event.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "core/ftime.h"
#include "core/fmath.h"

#include "game_types.h"
#include "game/spritesheet.h"
#include "game/fshader.h"

typedef struct user_interface_system_state {
  const app_settings * in_app_settings;
  const camera_metrics * in_camera_metrics;
  localization_package * display_language;
  
  std::array<checkbox, CHECKBOX_ID_MAX> checkboxes;
  std::array<checkbox_type, CHECKBOX_ID_MAX> checkbox_types;
  std::array<button, BTN_ID_MAX> buttons;
  std::array<button_type, BTN_TYPE_MAX> button_types;
  std::array<slider, SDR_ID_MAX> sliders;
  std::array<slider_type, SDR_TYPE_MAX> slider_types;
  std::array<progress_bar, PRG_BAR_ID_MAX> prg_bars;
  std::array<progress_bar_type, PRG_BAR_TYPE_ID_MAX> prg_bar_types;

  panel default_background_panel;
  spritesheet ss_to_draw_bg;
  Vector2 mouse_pos_screen;
  std::array<localization_package, LANGUAGE_INDEX_MAX> localization_info;
  std::vector<ui_error_display_control_system> errors_on_play;
  floating_text_display_system_state cfft_display_state;
  Vector2 error_text_start_position; // TODO: Put the error location and timer variables inside the error display system
  f32 error_text_end_height;
  f32 error_text_duration_stay_on_screen;
  f32 error_text_duration_in_and_out;

  user_interface_system_state(void) {
    this->in_app_settings = nullptr;
    this->display_language = nullptr;

    this->checkboxes.fill(checkbox());
    this->checkbox_types.fill(checkbox_type());
    this->buttons.fill(button());
    this->button_types.fill(button_type());
    this->sliders.fill(slider());
    this->slider_types.fill(slider_type());
    this->prg_bars.fill(progress_bar());
    this->prg_bar_types.fill(progress_bar_type());
    
    this->default_background_panel = panel();
    this->ss_to_draw_bg = spritesheet();
    this->mouse_pos_screen = ZEROVEC2;
    this->localization_info.fill(localization_package());
    this->errors_on_play = std::vector<ui_error_display_control_system>();
    this->cfft_display_state = floating_text_display_system_state();
    this->error_text_start_position = ZEROVEC2;
    this->error_text_end_height = 0.f;
    this->error_text_duration_stay_on_screen = 0.f; // in seconds
    this->error_text_duration_in_and_out = 0.f; // in seconds
  }
} user_interface_system_state;

static user_interface_system_state * state = nullptr;

#define BUTTON_TEXT_UP_COLOR WHITE_ROCK
#define BUTTON_TEXT_HOVER_COLOR WHITE
#define BUTTON_TEXT_PRESSED_COLOR WHITE
#define TEXT_SHADOW_COLOR BLACK
#define TEXT_SHADOW_SIZE_MULTIPLY 1.1f

#define MAX_UI_WORDWRAP_WORD_LENGTH 20
#define MAX_UI_WORDWRAP_SENTENCE_LENGTH 300

#define UI_ITALIC_FONT state->display_language->italic_font
#define UI_ITALIC_FONT_SIZE state->display_language->italic_font.baseSize
#define UI_LIGHT_FONT state->display_language->light_font
#define UI_LIGHT_FONT_SIZE state->display_language->light_font.baseSize
#define UI_REGULAR_FONT state->display_language->regular_font
#define UI_REGULAR_FONT_SIZE state->display_language->regular_font.baseSize
#define UI_BOLD_FONT state->display_language->bold_font
#define UI_BOLD_FONT_SIZE state->display_language->bold_font.baseSize
#define UI_TITLE_FONT state->display_language->mood
#define UI_TITLE_FONT_SIZE state->display_language->mood.baseSize

#define MENU_BUTTON_FONT UI_BOLD_FONT
#define MENU_BUTTON_FONT_SIZE_SCALE 1
#define MINI_BUTTON_FONT UI_REGULAR_FONT
#define MINI_BUTTON_FONT_SIZE_SCALE 1
#define LABEL_FONT UI_REGULAR_FONT
#define LABEL_FONT_SIZE_SCALE 1
#define DEFAULT_MENU_BUTTON_SCALE 4
#define SLIDER_FONT FONT_TYPE_LIGHT
#define DEFAULT_SLIDER_FONT_SIZE 1
#define DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH 10
#define DEFAULT_ERROR_FONT_TYPE FONT_TYPE_REGULAR
#define DEFAULT_ERROR_FONT_SIZE 2

#define ERROR_TEXT_DURATION_STAY_ON_SCREEN 2.1f
#define ERROR_TEXT_DURATION_IN_AND_OUT .5f

#define SDR_AT(ID) state->sliders.at(ID)
#define SDR_CURR_OPT_VAL(ID) state->sliders.at(ID).options.at(state->sliders.at(ID).current_value)
#define SDR_ASSERT_SET_CURR_VAL(EXPR, ID, INDEX) {\
  if(EXPR) state->sliders.at(ID).current_value = INDEX;\
}
#define SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(EXPR, ID) {\
  if(EXPR) state->sliders.at(ID).current_value = state->sliders.at(ID).options.size() - 1;\
}

#define UI_BASE_RENDER_SCALE(SCALE) VECTOR2(\
  static_cast<f32>(state->in_app_settings->render_width * SCALE),\
  static_cast<f32>(state->in_app_settings->render_height * SCALE))
#define UI_BASE_RENDER_DIV2 VECTOR2(static_cast<f32>(state->in_app_settings->render_width_div2), static_cast<f32>(state->in_app_settings->render_height_div2))
#define UI_BASE_RENDER_RES_VEC VECTOR2(static_cast<f32>(state->in_app_settings->render_width), static_cast<f32>(state->in_app_settings->render_height))

#define UI_BASE_RENDER_WIDTH state->in_app_settings->render_width
#define UI_BASE_RENDER_HEIGHT state->in_app_settings->render_height
#define COMBAT_FEEDBACK_FLOATING_TEXT_MIN_DURATION 0.8f
#define COMBAT_FEEDBACK_FLOATING_TEXT_MAX_DURATION 0.8f
#define COMBAT_FEEDBACK_FLOATING_TEXT_MIN_SCALE 1.8f
#define COMBAT_FEEDBACK_FLOATING_TEXT_MAX_SCALE 2.8f

#define IF_NOT_STATE(FUNCTION, RETURN) do { if (not state or state == nullptr) {\
  IERROR("user_interface::" FUNCTION "::State is invalid");\
  RETURN\
} } while(0);

bool user_interface_on_event(i32 code, event_context context);
 
void update_buttons(void);
void update_sliders(void);
void update_checkboxes(void);
void update_display_errors(f32 delta_time);
void render_display_errors(void);
void combat_feedback_update_floating_texts(f32 delta_time);
void combat_feedback_render_floating_texts(void);
 
void draw_slider_body(const slider *const sdr);
void draw_atlas_texture_regular(atlas_texture_id _id, Rectangle dest, Color tint, bool should_center);
void draw_atlas_texture_npatch(atlas_texture_id _id, Rectangle dest, Vector4 offsets, bool should_center);
void gui_draw_settings_screen(void);
bool gui_button(const char* text, button_id _id, Font font, i32 font_size_scale, Vector2 pos, bool play_on_click_sound);
void gui_checkbox(checkbox_id _id, Vector2 pos);
void gui_display_error(ui_error_display_control_system err);

void register_checkbox(checkbox_id _cb_id, checkbox_type_id _cb_type_id);
void register_checkbox_type(checkbox_type_id _cb_type_id, spritesheet_id _ss_type, Vector2 frame_dim, f32 _scale, bool _should_center);
void register_button(button_id _btn_id, button_type_id _btn_type_id);
void register_button_type(button_type_id _btn_type_id, spritesheet_id _ss_type, Vector2 frame_dim, f32 _scale, bool _should_center);
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id);
void register_progress_bar_type(progress_bar_type prg_bar_type);
void register_slider_type(slider_type_id _sdr_type_id, spritesheet_id _ss_sdr_body_type, f32 _scale, i32 _width_multiply, button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, i32 _char_limit);

void draw_text_shader(const char *text, shader_id sdr_id, Vector2 position, Font font, float fontsize, Color tint, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord);
void draw_text_ex(const char *text, Vector2 position, Font font, f32 _fontsize, Color tint, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord);
void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);
const char* wrap_text(const char* text, Font font, i32 font_size, Rectangle bounds, bool center_x);
Font load_font(pak_file_id pak_id, i32 asset_id, i32 font_size, i32* _codepoints, i32 _codepoint_count);
localization_package* load_localization(std::string language_name, i32 lang_index, std::string _codepoints, i32 font_size);
localization_package* ui_get_localization_by_name(const char * language_name);
localization_package* ui_get_localization_by_index(language_index index);

bool ui_sound_slider_on_left_button_trigger(void);
bool ui_sound_slider_on_right_button_trigger(void);

constexpr Font font_type_to_font(font_type in_font_type) {
  if (not state->display_language or state->display_language == nullptr) {
    return GetFontDefault();
  }
  switch (in_font_type) {
    case FONT_TYPE_ITALIC: {
      return UI_ITALIC_FONT;
      break;
    }
    case FONT_TYPE_LIGHT: {
      return UI_LIGHT_FONT;
      break;
    }
    case FONT_TYPE_REGULAR: {
      return UI_REGULAR_FONT;
      break;
    }
    case FONT_TYPE_BOLD: {
      return UI_BOLD_FONT;
      break;
    }
    case FONT_TYPE_TITLE: {
      return UI_TITLE_FONT;
      break;
    }
    default: return GetFontDefault();
  }
  return GetFontDefault();
}
constexpr void draw_text_simple(const char* text, Vector2 pos, font_type in_font_type, i32 fontsize, Color color) {
  if (not text or text == nullptr) {
    return;
  }
  Font font = font_type_to_font(in_font_type);
  BeginShaderMode(get_shader_by_enum(SHADER_ID_SDF_TEXT)->handle);
    DrawTextEx(font, text, pos, font.baseSize * fontsize, UI_FONT_SPACING, color);
  EndShaderMode();
}
constexpr void draw_text(const char* text, Vector2 pos, font_type in_font_type, i32 fontsize, Color color, bool center_horizontal, bool center_vertical, bool use_grid_align = false, 
  Vector2 grid_coord = ZEROVEC2
) {
  if (not text or text == nullptr) {
    return;
  }
  Font font = font_type_to_font(in_font_type);
  Vector2 text_measure = MeasureTextEx(font, text, font.baseSize * fontsize, UI_FONT_SPACING);
  Vector2 text_position = pos;
  if (use_grid_align) {
    text_position = position_element_by_grid(text_position, grid_coord, SCREEN_OFFSET);
  }
  if (center_horizontal) {
    text_position.x -= (text_measure.x * .5f);
  }
  if (center_vertical) {
    text_position.y -= (text_measure.y * .5f);
  }
  BeginShaderMode(get_shader_by_enum(SHADER_ID_SDF_TEXT)->handle);
    DrawTextEx(font, text, text_position, font.baseSize * fontsize, UI_FONT_SPACING, color);
  EndShaderMode();
}
inline void draw_text_ex(const char* text, Vector2 pos, ::font_type font_type, f32 fontsize, Color tint) {
  if (not text or text == nullptr) {
    return;
  }
  Font font = ui_get_font(font_type);

  BeginShaderMode(get_shader_by_enum(SHADER_ID_SDF_TEXT)->handle);
    DrawTextEx(font, text, pos, fontsize, UI_FONT_SPACING, tint);
  EndShaderMode();
}
bool user_interface_system_initialize(const camera_metrics * in_camera_metrics) {
  if (state or state != nullptr) {
    return true;
  }  
  if (not in_camera_metrics or in_camera_metrics == nullptr) {
    IERROR("user_interface::user_interface_system_initialize()::Camera pointer is invalid");
    return false;
  }
  state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);
  IF_NOT_STATE("user_interface_system_initialize", return false; );

  *state = user_interface_system_state();
  
  state->in_camera_metrics = in_camera_metrics;
  state->in_app_settings = get_app_settings();
  state->error_text_duration_in_and_out = ERROR_TEXT_DURATION_IN_AND_OUT;
  state->error_text_duration_stay_on_screen = ERROR_TEXT_DURATION_STAY_ON_SCREEN;
  state->cfft_display_state = floating_text_display_system_state(
    COMBAT_FEEDBACK_FLOATING_TEXT_MIN_DURATION,
    COMBAT_FEEDBACK_FLOATING_TEXT_MAX_DURATION,
    COMBAT_FEEDBACK_FLOATING_TEXT_MIN_SCALE,
    COMBAT_FEEDBACK_FLOATING_TEXT_MAX_SCALE
  );

  const std::array<loc_data, LANGUAGE_INDEX_MAX> *const langs = loc_parser_get_loc_langs();
  if (not langs or langs == nullptr) {
    IERROR("user_interface::user_interface_system_initialize()::Localization parse failed");
    return false;
  }
  for (size_t itr_000 = 0u; itr_000 < langs->size(); ++itr_000) {
    const loc_data *const data = __builtin_addressof(langs->at(itr_000));
    if (data->index <= LANGUAGE_INDEX_UNDEFINED or data->index >= LANGUAGE_INDEX_MAX) {
      continue;
    }
    const localization_package *const _loc_data = load_localization(data->language_name, data->index, data->codepoints, 24);
    if (not _loc_data or _loc_data == nullptr) {
      IERROR("user_interface::user_interface_system_initialize()::Loading localization:%s is failed", data->language_name.c_str());
      continue;
    }
  }
  const localization_package* settings_loc_lang = ui_get_localization_by_name(state->in_app_settings->language.c_str());
  if (not settings_loc_lang or settings_loc_lang == nullptr) {
    IERROR("user_interface::user_interface_system_initialize()::User interface state is corrupted or file parsing failed");
    return false;
  }
  if (settings_loc_lang->index <= LANGUAGE_INDEX_UNDEFINED or settings_loc_lang->index >= LANGUAGE_INDEX_MAX) {
    IWARN("user_interface::user_interface_system_initialize()::The language requested by config (%s) cannot loaded. Setting to builtin", state->in_app_settings->language.c_str());
    if(not loc_parser_set_active_language_builtin()) {
      return false;
    }
    settings_loc_lang = ui_get_localization_by_index(LANGUAGE_INDEX_BUILTIN);
  }
  if(loc_parser_set_active_language_by_index(settings_loc_lang->index)) {
    loc_data * _loc_data = loc_parser_get_active_language();
    if (not _loc_data or _loc_data == nullptr) {
      IERROR("user_interface::user_interface_system_initialize()::Language init failed");
      return false;
    }
    state->display_language = ui_get_localization_by_index(_loc_data->index);
  }
  if(not initialize_shader_system()) {
    IERROR("user_interface::user_interface_system_initialize()::Shader system failed to initialize!");
    return false;
  }

  // BUTTON TYPES
  {
  register_button_type(
    BTN_TYPE_MENU_BUTTON, SHEET_ID_MENU_BUTTON, Vector2{88, 13}, DEFAULT_MENU_BUTTON_SCALE, false);
  register_button_type(
    BTN_TYPE_SLIDER_LEFT_BUTTON, SHEET_ID_SLIDER_LEFT_BUTTON, Vector2{12, 12}, 3, false);
  register_button_type(
    BTN_TYPE_SLIDER_RIGHT_BUTTON, SHEET_ID_SLIDER_RIGHT_BUTTON, Vector2{12, 12}, 3, false);
  register_button_type(
    BTN_TYPE_FLAT_BUTTON, SHEET_ID_FLAT_BUTTON, Vector2{88, 13}, 2, false);
  }
  // BUTTON TYPES

  // SLIDER TYPES
  {
  register_slider_type(
    SDR_TYPE_PERCENT, SHEET_ID_SLIDER_PERCENT, 
    5, 6,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  register_slider_type(
    SDR_TYPE_OPTION, SHEET_ID_SLIDER_OPTION, 
    5, 3,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  register_slider_type(
    SDR_TYPE_NUMBER, SHEET_ID_SLIDER_OPTION,
    DEFAULT_MENU_BUTTON_SCALE, 2,
    BTN_TYPE_SLIDER_LEFT_BUTTON, BTN_TYPE_SLIDER_RIGHT_BUTTON, 6
  );
  }
  // SLIDER TYPES

  // PROGRES BAR TYPES
  {
    register_progress_bar_type(progress_bar_type(
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR, ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL, ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL, SHADER_ID_PROGRESS_BAR_MASK, 
      Rectangle {3.f, 0.f, 42.f, 4}, Rectangle {23.f, 0.f, 14.f, 4.f}, Rectangle {16.f, 0.f, 16.f, 4.f})
    );
    register_progress_bar_type(progress_bar_type(
      PRG_BAR_TYPE_ID_DARK_FANTASY_BOSSBAR_6_YELLOW_INNER, ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_INSIDE_YELLOW, ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_OUTSIDE, SHADER_ID_PROGRESS_BAR_MASK, 
      Rectangle {0.f, 0.f, 192.f, 7}, Rectangle {32.f, 0.f, 128.f, 7}, Rectangle {32.f, 0.f, 128.f, 7})
    );
    register_progress_bar_type(progress_bar_type(
      PRG_BAR_TYPE_ID_DARK_FANTASY_BOSSBAR_6_WHITE_INNER, ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_INSIDE_WHITE, ATLAS_TEX_ID_DARK_FANTASY_BOSSBAR_6_OUTSIDE, SHADER_ID_PROGRESS_BAR_MASK, 
      Rectangle {0.f, 0.f, 192.f, 7}, Rectangle {32.f, 0.f, 128.f, 7}, Rectangle {32.f, 0.f, 128.f, 7})
    );
  }
  // PROGRES BAR TYPES

  // PROGRES BARS
  {
    register_progress_bar(PRG_BAR_ID_PLAYER_EXPERIANCE, PRG_BAR_TYPE_ID_DARK_FANTASY_BOSSBAR_6_YELLOW_INNER);
    register_progress_bar(PRG_BAR_ID_PLAYER_HEALTH, PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR);
    register_progress_bar(PRG_BAR_ID_BOSS_HEALTH, PRG_BAR_TYPE_ID_DARK_FANTASY_BOSSBAR_6_WHITE_INNER);
  }
  // PROGRES BARS

  // CHECKBOX TYPES
  {
    register_checkbox_type(CHECKBOX_TYPE_BUTTON, SHEET_ID_SLIDER_PERCENT, Vector2{9, 9}, 4, true);
  }
  // CHECKBOX TYPES

  // IN GAME
  {
    register_button(BTN_ID_IN_GAME_BUTTON_RETURN_MENU, BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_IN_GAME_BUTTON_CHEST_OPENING_ACCEPT, BTN_TYPE_FLAT_BUTTON);
  }
  // IN GAME

  // MAIN MENU
  {
    register_button(BTN_ID_MAINMENU_BUTTON_PLAY,                        BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_EDITOR,                      BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_SETTINGS,                    BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_ENTER_STATE_CHARACTER,       BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_EXIT,                        BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_SETTINGS_CANCEL,                    BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_STATE_CHARACTER_BACK,               BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_STATE_CHARACTER_BUY_STAT_UPGRADE,   BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_INVENTORY,BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_STATE_CHARACTER_ENTER_TAB_STATS,    BTN_TYPE_FLAT_BUTTON);
    register_button(BTN_ID_MAINMENU_MAP_CHOICE_BACK,                    BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_TRAIT_CHOICE_BACK,                  BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_TRAIT_CHOICE_ACCEPT,                BTN_TYPE_MENU_BUTTON);
  }
  // MAIN MENU

  // EDITOR
  {
    register_button(BTN_ID_EDITOR_ADD_MAP_COLLISION,   BTN_TYPE_MENU_BUTTON);
    register_checkbox(CHECKBOX_ID_IS_PROP_YBASED, CHECKBOX_TYPE_BUTTON);
  }
  // EDITOR

  // USER INTERFACE
  {
    register_button(BTN_ID_PAUSEMENU_BUTTON_RESUME,            BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_PAUSEMENU_BUTTON_SETTINGS,          BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_MAIN_MENU, BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_DESKTOP,   BTN_TYPE_MENU_BUTTON);
  }
  // USER INTERFACE

  // SETTINGS
  {
    register_slider(
      SDR_ID_SETTINGS_SOUND_SLIDER,  SDR_TYPE_PERCENT, 
      BTN_ID_SETTINGS_SLIDER_SOUND_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_SOUND_RIGHT_BUTTON, true, false);
    register_slider(
      SDR_ID_SETTINGS_RES_SLIDER,  SDR_TYPE_OPTION, 
      BTN_ID_SETTINGS_SLIDER_RES_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_RES_RIGHT_BUTTON, false, false);
    register_slider(
      SDR_ID_SETTINGS_WIN_MODE_SLIDER,  SDR_TYPE_OPTION, 
      BTN_ID_SETTINGS_SLIDER_WIN_MODE_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_WIN_MODE_RIGHT_BUTTON, false, true);
    register_slider(
      SDR_ID_SETTINGS_LANGUAGE,  SDR_TYPE_OPTION, 
      BTN_ID_SETTINGS_SLIDER_LANGUAGE_LEFT_BUTTON, BTN_ID_SETTINGS_SLIDER_LANGUAGE_RIGHT_BUTTON, false, true);
    register_button(BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, BTN_TYPE_MENU_BUTTON);
  }
  // SETTINGS

    // SLIDER OPTIONS
  {
    if (ui_get_localization_by_name(loc_parser_lang_index_to_name(LANGUAGE_INDEX_ENGLISH))) {
      gui_slider_add_option(SDR_ID_SETTINGS_LANGUAGE, data_pack(DATA_TYPE_I32, data128(LANGUAGE_INDEX_ENGLISH), 1), LOC_TEXT_SETTINGS_BUTTON_ENGLISH, "");
      SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->display_language->index == LANGUAGE_INDEX_ENGLISH, SDR_ID_SETTINGS_LANGUAGE);
    }

    if (ui_get_localization_by_name(loc_parser_lang_index_to_name(LANGUAGE_INDEX_TURKISH))) {
      gui_slider_add_option(SDR_ID_SETTINGS_LANGUAGE, data_pack(DATA_TYPE_I32, data128(LANGUAGE_INDEX_TURKISH), 1), LOC_TEXT_SETTINGS_BUTTON_TURKISH, "");
      SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->display_language->index == LANGUAGE_INDEX_TURKISH, SDR_ID_SETTINGS_LANGUAGE);
    }

    if (state->sliders.at(SDR_ID_SETTINGS_LANGUAGE).options.empty()) {
      gui_slider_add_option(SDR_ID_SETTINGS_LANGUAGE, data_pack(DATA_TYPE_I32, data128(LANGUAGE_INDEX_BUILTIN), 1), LOC_TEXT_SETTINGS_BUTTON_ENGLISH, "");
      SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->display_language->index == LANGUAGE_INDEX_BUILTIN, SDR_ID_SETTINGS_LANGUAGE);
    }

    gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, data_pack(DATA_TYPE_I32, data128(0), 1), LOC_TEXT_SETTINGS_SDR_WINDOW_MODE_WINDOWED,   "");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->in_app_settings->window_state == 0, SDR_ID_SETTINGS_WIN_MODE_SLIDER)
    gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, data_pack(DATA_TYPE_I32, data128(FLAG_BORDERLESS_WINDOWED_MODE), 1), LOC_TEXT_SETTINGS_SDR_WINDOW_MODE_BORDERLESS, "");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->in_app_settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE, SDR_ID_SETTINGS_WIN_MODE_SLIDER)
    gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, data_pack(DATA_TYPE_I32, data128(FLAG_FULLSCREEN_MODE), 1), LOC_TEXT_SETTINGS_SDR_WINDOW_MODE_FULLSCREEN, "");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->in_app_settings->window_state == FLAG_FULLSCREEN_MODE, SDR_ID_SETTINGS_WIN_MODE_SLIDER)
  }
  // SLIDER OPTIONS

  state->default_background_panel = panel(BTN_STATE_UNDEFINED, ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG, ATLAS_TEX_ID_DARK_FANTASY_PANEL_SELECTED, 
    Vector4 {6.f, 6.f, 6.f, 6.f}, Color { 30, 39, 46, 128}
  );

  ui_refresh_setting_sliders_to_default();

  state->sliders.at(SDR_ID_SETTINGS_SOUND_SLIDER).on_left_button_trigger = ui_sound_slider_on_left_button_trigger;
  state->sliders.at(SDR_ID_SETTINGS_SOUND_SLIDER).on_right_button_trigger = ui_sound_slider_on_right_button_trigger;

  event_register(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, user_interface_on_event);
  return true;
}
void update_user_interface(f32 delta_time) {
  IF_NOT_STATE("update_user_interface", return; );

  Vector2 mouse_pos_screen_unscaled = GetMousePosition();
  state->mouse_pos_screen.x = mouse_pos_screen_unscaled.x * state->in_app_settings->scale_ratio.at(0);
  state->mouse_pos_screen.y = mouse_pos_screen_unscaled.y * state->in_app_settings->scale_ratio.at(1);
  state->error_text_start_position = Vector2 { 
    state->in_app_settings->render_width * .5f, -state->in_app_settings->render_height * .025f
  };
  state->error_text_end_height = state->in_app_settings->render_height * .075f;

  update_buttons();
  update_sliders();
  update_checkboxes();
  update_display_errors(delta_time);
  combat_feedback_update_floating_texts(delta_time);
}
 void update_buttons(void) {
  for (size_t itr_000 = 0; itr_000 < state->buttons.size(); ++itr_000) {
    if (state->buttons.at(itr_000).id == BTN_ID_UNDEFINED) {
      continue;
    }
    if (!state->buttons.at(itr_000).on_screen) { continue; }

    button* btn = __builtin_addressof(state->buttons.at(itr_000));
    if (CheckCollisionPointRec(state->mouse_pos_screen, btn->dest)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        btn->prev_state = btn->current_state;
        btn->current_state = BTN_STATE_PRESSED;
      } else {
        if (btn->current_state == BTN_STATE_PRESSED) {
          btn->prev_state = btn->current_state;
          btn->current_state = BTN_STATE_RELEASED;
        }
        else if (btn->current_state != BTN_STATE_HOVER) {
          btn->prev_state = btn->current_state;
          btn->current_state = BTN_STATE_HOVER;
        }
      }
    } else {
      btn->prev_state = btn->current_state;
      btn->current_state = BTN_STATE_UP;
    }
    btn->on_screen = false;
  }
}
 void update_sliders(void) {
  for (size_t itr_000 = 0u; itr_000 < state->sliders.size(); ++itr_000) {
    if ((state->sliders.at(itr_000).id <= SDR_ID_UNDEFINED or state->sliders.at(itr_000).id >= SDR_ID_MAX) or not state->sliders.at(itr_000).on_screen) continue;
    
    slider *const sdr = __builtin_addressof(state->sliders.at(itr_000));
    if (not sdr or not sdr->is_registered) {
      IWARN("user_interface::update_sliders()::Using slider didn't registered");
      sdr->on_screen = false;
      continue;
    }

    switch (sdr->sdr_type.id) {
      case SDR_TYPE_PERCENT: { 
        Rectangle sdr_rect = {
          (f32)sdr->position.x, (f32)sdr->position.y, 
          (f32)sdr->sdr_type.body_width * sdr->sdr_type.width_multiply, (f32)sdr->sdr_type.dest_frame_dim.y
        };

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && sdr->is_clickable) {
          if (CheckCollisionPointRec(state->mouse_pos_screen, sdr_rect)) {
            f32 relative = state->mouse_pos_screen.x - sdr_rect.x;
            f32 ratio = relative / sdr_rect.width;
            sdr->current_value = ratio * sdr->options.size() - 1;
            sdr->current_value = static_cast<size_t>(FCLAMP(static_cast<size_t>(sdr->current_value), 0u, sdr->options.size() - 1u));

            if (sdr->on_click != nullptr) {
              sdr->on_click();
            }
          }
        }
        break; 
      }
      case SDR_TYPE_OPTION:  { 
        Rectangle sdr_rect = {
          (f32)sdr->position.x, (f32)sdr->position.y, 
          (f32)sdr->sdr_type.body_width * sdr->sdr_type.width_multiply, (f32)sdr->sdr_type.dest_frame_dim.y
        };

        if (sdr->is_clickable and sdr->on_click != nullptr and IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ) {
          if (CheckCollisionPointRec(state->mouse_pos_screen, sdr_rect)) {
            sdr->on_click();
          }
        }
        break; 
      }
      case SDR_TYPE_NUMBER:  {
        Rectangle sdr_rect = {
          (f32)sdr->position.x, (f32)sdr->position.y, 
          (f32)sdr->sdr_type.body_width * sdr->sdr_type.width_multiply, (f32)sdr->sdr_type.dest_frame_dim.y
        };

        if (sdr->is_clickable and sdr->on_click != nullptr and IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ) {
          if (CheckCollisionPointRec(state->mouse_pos_screen, sdr_rect)) {
            sdr->on_click();
          }
        }
        break; 
      }
      default: { break; }
    }

    sdr->on_screen = false;
  }  
}
void update_checkboxes(void) {  
  for (size_t itr_000 = 0; itr_000 < state->checkboxes.size(); ++itr_000) {
    checkbox& _checkbox = state->checkboxes.at(itr_000);
    if (_checkbox.id >= CHECKBOX_ID_MAX or _checkbox.id <= CHECKBOX_ID_UNDEFINED) {
      continue;
    }
    if (!_checkbox.on_screen) { continue; }

    if (CheckCollisionPointRec(state->mouse_pos_screen, _checkbox.dest)) {
      checkbox_state current_state = _checkbox.state;
      if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        _checkbox.state = (current_state == CHECKBOX_STATE_CHECKED) ? CHECKBOX_STATE_UNCHECKED : CHECKBOX_STATE_CHECKED;
      }
      if (current_state != _checkbox.state and _checkbox.pfn_on_change and _checkbox.pfn_on_change != nullptr) {
        _checkbox.pfn_on_change();
      }
    }
    _checkbox.on_screen = false;
  }
}
void update_display_errors(f32 delta_time) {
  for (size_t itr_000 = 0u; itr_000 < state->errors_on_play.size(); ++itr_000) {
    ui_error_display_control_system& err = state->errors_on_play.at(itr_000);

    switch (err.display_state) {
      case ERROR_DISPLAY_ANIMATION_STATE_MOVE_IN: {
        err.location.y = EaseQuadIn(
          err.accumulator, 
          state->error_text_start_position.y, 
          state->error_text_end_height, 
          err.duration
        );

        err.accumulator += delta_time;

        if (err.accumulator > err.duration) {
          err.display_state = ERROR_DISPLAY_ANIMATION_STATE_STAY;
          err.duration = ERROR_TEXT_DURATION_STAY_ON_SCREEN;
          err.accumulator = 0.f;
        }
        return;
      }
      case ERROR_DISPLAY_ANIMATION_STATE_STAY: {
        err.accumulator += delta_time;

        if (err.accumulator > err.duration) {
          err.display_state = ERROR_DISPLAY_ANIMATION_STATE_MOVE_OUT;
          err.duration = ERROR_TEXT_DURATION_IN_AND_OUT;
          err.accumulator = 0.f;
        }
        return;
      }
      case ERROR_DISPLAY_ANIMATION_STATE_MOVE_OUT: {
        err.location.y = EaseQuadIn(
          err.accumulator, 
          state->error_text_start_position.y + state->error_text_end_height, 
          -state->error_text_end_height, 
          err.duration
        );

        err.accumulator += delta_time;

        if (err.accumulator > err.duration) {
          state->errors_on_play.erase(state->errors_on_play.begin() + itr_000);
        }
        return;
      }
      default: {
        IWARN("user_interface::update_display_errors()::Unsupported state");
        state->errors_on_play.erase(state->errors_on_play.begin() + itr_000);
        return;
      }
    }
  }
}
void render_user_interface(void) {
  IF_NOT_STATE("render_user_interface", return; );

  render_display_errors();
  combat_feedback_render_floating_texts();
}
void render_display_errors(void) {
  for (ui_error_display_control_system& err : state->errors_on_play) {
    gui_display_error(err);
  }
}
bool gui_menu_button(const char* text, button_id _id, Vector2 grid, Vector2 grid_location, bool play_on_click_sound) {
  if (not text or text == nullptr) {
    return false;
  }
  if (not state->display_language or state->display_language == nullptr) {
    return false;
  }
  grid_location.x -= state->buttons.at(_id).btn_type.dest_frame_dim.x * .5f;
  grid_location.y -= state->buttons.at(_id).btn_type.dest_frame_dim.y * .5f;
  return gui_button(text, _id, MENU_BUTTON_FONT, MENU_BUTTON_FONT_SIZE_SCALE, position_element_by_grid(grid_location, grid, Vector2 {
    UI_BASE_RENDER_WIDTH * .00390625f, UI_BASE_RENDER_HEIGHT * .00694f
  }), play_on_click_sound);
}
bool gui_mini_button(const char* text, button_id _id, Vector2 grid, bool play_on_click_sound) {
  if (not text or text == nullptr) {
    return false;
  }
  if (not state->display_language or state->display_language == nullptr) {
    return false;
  }
  return gui_button(text, _id, 
    MINI_BUTTON_FONT, MINI_BUTTON_FONT_SIZE_SCALE, 
    position_element_by_grid(UI_BASE_RENDER_DIV2, 
    grid, SCREEN_OFFSET
  ),
    play_on_click_sound
  );
}
bool gui_slider_button(button_id _id, Vector2 pos) {
  return gui_button("", _id, Font {}, 0, pos, true);
}
bool gui_button(const char* text, button_id _id, Font font, i32 font_size_scale, Vector2 pos, bool play_on_click_sound) {
  if (not text or text == nullptr) {
    return false;
  }
  if (_id >= BTN_ID_MAX or _id <= BTN_ID_UNDEFINED) {
    IWARN("user_interface::gui_button()::Recieved button type out of bound");
    return false;
  }
  button *const _btn = __builtin_addressof(state->buttons.at(_id));

  if (!_btn->is_registered) {
    IWARN("user_interface::gui_button()::The button is not registered");
    return false;
  }
  _btn->on_screen = true;
  _btn->dest.x = pos.x;
  _btn->dest.y = pos.y;

  Vector2 text_measure;
  Vector2 text_pos;

  if (not text or not TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(font, text, font.baseSize * font_size_scale, UI_FONT_SPACING);
    text_pos = Vector2 {
      .x = _btn->dest.x + (_btn->dest.width / 2.f)  - (text_measure.x / 2.f),
      .y = _btn->dest.y + (_btn->dest.height / 2.f) - (text_measure.y / 2.f)
    };
  }

  Vector2 draw_sprite_scale = Vector2 {_btn->btn_type.scale,_btn->btn_type.scale};

  if (_btn->current_state == BTN_STATE_PRESSED) {
    draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 1);
    if (not TextIsEqual(text, "")) {
      draw_text_ex(text, text_pos, font, font_size_scale, _btn->btn_type.forground_color_btn_state_pressed, false, false, false, VECTOR2(0.f, 0.f));
    }
    if (play_on_click_sound and _btn->prev_state == BTN_STATE_HOVER) event_fire(EVENT_CODE_PLAY_SOUND_GROUP, event_context(SOUNDGROUP_ID_BUTTON_ON_CLICK, static_cast<i32>(true)));
  }
  if (_btn->current_state == BTN_STATE_HOVER) {
    draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 1);
    if (not TextIsEqual(text, "")) {
      draw_text_ex(text, text_pos, font, font_size_scale, _btn->btn_type.forground_color_btn_state_hover, false, false, false, VECTOR2(0.f, 0.f));
    }
    event_fire(EVENT_CODE_RESET_SOUND_GROUP, event_context(static_cast<i32>(SOUNDGROUP_ID_BUTTON_ON_CLICK)));
  }
  if (_btn->current_state == BTN_STATE_UP) {
    draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 0);
    if (_btn->current_state != BTN_STATE_HOVER) {
      draw_text_ex(text, text_pos, font, font_size_scale, _btn->btn_type.forground_color_btn_state_up, false, false, false, VECTOR2(0.f, 0.f));
    }
  }
  
  return _btn->current_state == _btn->signal_state;
}
bool gui_draw_local_button(const char* text, local_button* const btn, const font_type _font_type, const i32 font_size_scale, const Vector2 pos, text_alignment align_to, const bool play_on_click_sound) {
  if (not text or text == nullptr or not btn or btn == nullptr) {
    return false;
  }

  if (!btn->is_active) {
    return false;
  } 
  btn->on_screen = true;
  btn->dest.x = pos.x;
  btn->dest.y = pos.y;

  Vector2 text_measure = ZEROVEC2;
  Vector2 text_pos = ZEROVEC2;
  Font _font = ui_get_font(_font_type);
  if (!TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(_font, text, _font.baseSize * font_size_scale, UI_FONT_SPACING);
    text_pos = ui_align_text(btn->dest, text_measure, align_to);
  }


  Vector2 draw_sprite_scale = Vector2 {btn->btn_type.scale, btn->btn_type.scale};

  if (btn->current_state == BTN_STATE_PRESSED) {
    draw_sprite_on_site_by_id(btn->btn_type.ss_type, WHITE, VECTOR2(btn->dest.x, btn->dest.y), draw_sprite_scale, 1);
    if (!TextIsEqual(text, "")) {
      draw_text_ex(text, text_pos, _font, font_size_scale, btn->btn_type.forground_color_btn_state_pressed, false, false, false, VECTOR2(0.f, 0.f));
    }
    if (play_on_click_sound and btn->prev_state == BTN_STATE_HOVER) event_fire(EVENT_CODE_PLAY_SOUND_GROUP, event_context(SOUNDGROUP_ID_BUTTON_ON_CLICK, static_cast<i32>(true)));
  } 
  if (btn->current_state == BTN_STATE_HOVER) {
    draw_sprite_on_site_by_id(btn->btn_type.ss_type, WHITE, VECTOR2(btn->dest.x, btn->dest.y), draw_sprite_scale, 1);
    if (!TextIsEqual(text, "")) {
      draw_text_ex(text, text_pos, _font, font_size_scale, btn->btn_type.forground_color_btn_state_hover, false, false, false, VECTOR2(0.f, 0.f));
    }
    event_fire(EVENT_CODE_RESET_SOUND_GROUP, event_context(static_cast<i32>(SOUNDGROUP_ID_BUTTON_ON_CLICK)));
  }
  if (btn->current_state == BTN_STATE_UP) {
    draw_sprite_on_site_by_id(btn->btn_type.ss_type, WHITE, VECTOR2(btn->dest.x, btn->dest.y), draw_sprite_scale, 0);
    if (btn->current_state != BTN_STATE_HOVER) {
      draw_text_ex(text, text_pos, _font, font_size_scale, btn->btn_type.forground_color_btn_state_up, false, false, false, VECTOR2(0.f, 0.f));
    }
  }
  
  return btn->current_state == btn->signal_state;
}
void gui_checkbox_grid(checkbox_id _id, Vector2 grid, Vector2 grid_location) {
  grid_location.x -= state->checkboxes.at(_id).cb_type.dest_frame_dim.x * .5f;
  grid_location.y -= state->checkboxes.at(_id).cb_type.dest_frame_dim.y * .5f;
  gui_checkbox(_id, position_element_by_grid(grid_location, grid, SCREEN_OFFSET));
}
void gui_checkbox(checkbox_id _id, Vector2 pos) {
  if (_id >= CHECKBOX_ID_MAX || _id <= CHECKBOX_ID_UNDEFINED) { return; }

  checkbox* _cb = __builtin_addressof(state->checkboxes.at(_id));

  if (!_cb->is_registered) { return; }
  _cb->on_screen = true;
  _cb->dest.x = pos.x;
  _cb->dest.y = pos.y;

  Vector2 draw_sprite_scale = Vector2 {_cb->cb_type.scale,_cb->cb_type.scale};

  if (_cb->state == CHECKBOX_STATE_CHECKED) {
    draw_sprite_on_site_by_id(_cb->cb_type.ss_type, WHITE, VECTOR2(pos.x, pos.y), draw_sprite_scale, 1);
  } 
  else if (_cb->state == CHECKBOX_STATE_UNCHECKED){
    draw_sprite_on_site_by_id(_cb->cb_type.ss_type, WHITE, VECTOR2(pos.x, pos.y), draw_sprite_scale, 0);
  }
}
void gui_progress_bar(progress_bar_id bar_id, Rectangle dest, bool _should_center, Color inside_tint, Color outside_tint) {
  IF_NOT_STATE("gui_progress_bar", return; );

  if (bar_id <= PRG_BAR_ID_UNDEFINED or bar_id >= PRG_BAR_ID_MAX) {
    IWARN("user_interface::gui_progress_bar()::Progress bar id is out of bound");
    return;
  }
  progress_bar prg_bar = state->prg_bars.at(bar_id);
  if (not prg_bar.is_initialized) {
    IWARN("user_interface::gui_progress_bar()::Player experiance process bar didn't initialized");
    return;
  }
  const atlas_texture *const inside_tex = ss_get_atlas_texture_by_enum(prg_bar.type.body_inside);
  const atlas_texture *const outside_tex = ss_get_atlas_texture_by_enum(prg_bar.type.body_outside);
  const Texture2D *const atlas = ss_get_texture_by_enum(ATLAS_TEXTURE_ID);
  if (not inside_tex or inside_tex == nullptr) {
    IWARN("user_interface::gui_progress_bar()::progress bar inside texture is invalid");
    return;
  }
  if (not outside_tex or outside_tex == nullptr) {
    IWARN("user_interface::gui_progress_bar()::progress bar outside atlas is invalid");
    return;
  }
  if (not atlas or atlas == nullptr) {
    IWARN("user_interface::gui_progress_bar()::Atlas texture is invalid");
    return;
  }
  if (_should_center) {
    dest.x -= dest.width * .5f;
    dest.y -= dest.height * .5f;
  }
  Rectangle strecth_part_inside = prg_bar.type.strecth_part_inside;
  Rectangle strecth_part_outside = prg_bar.type.strecth_part_outside;
  f32 left_src_width_ratio = strecth_part_outside.x / outside_tex->source.height;
  Rectangle left_source_outside = Rectangle {0, 0, strecth_part_outside.x, outside_tex->source.height};

  Rectangle& scissor = prg_bar.type.body_inside_scissor;
  f32 norm_x = scissor.x / left_source_outside.width;
  f32 norm_y = scissor.y / outside_tex->source.height;
  f32 norm_width = (scissor.width) / outside_tex->source.width;
  f32 norm_height = scissor.height / outside_tex->source.height;
  Rectangle left_dest   = Rectangle {dest.x, dest.y,  dest.height * left_src_width_ratio, dest.height};
  Rectangle scr_rect = {
    dest.x + (left_dest.width * norm_x), 
    dest.y + (dest.height * norm_y),
    dest.width * norm_width,
    dest.height + (dest.height * norm_height),
  };
  f32 start_uv = inside_tex->source.x / atlas->width;
  f32 end_uv = (inside_tex->source.x + inside_tex->source.width) / atlas->width;
  f32 progress = start_uv + (end_uv - start_uv) * prg_bar.progress;

  BeginScissorMode(scr_rect.x,scr_rect.y,scr_rect.width,scr_rect.height);
    BeginShaderMode(get_shader_by_enum(prg_bar.type.mask_shader_id)->handle);
    {
      set_shader_uniform(prg_bar.type.mask_shader_id, "progress", data128(progress));
      set_shader_uniform(prg_bar.type.mask_shader_id, "tint", data128(
        static_cast<f32>(inside_tint.r), static_cast<f32>(inside_tint.g),
        static_cast<f32>(inside_tint.b), static_cast<f32>(inside_tint.a)
      ));
      draw_atlas_texture_stretch(prg_bar.type.body_inside, strecth_part_inside, dest, false);
    }
    EndShaderMode();
  EndScissorMode();

  draw_atlas_texture_stretch(prg_bar.type.body_outside, strecth_part_outside, dest, false, outside_tint);
}
void draw_atlas_texture_stretch(atlas_texture_id tex_id, Rectangle stretch_part, Rectangle dest, bool should_center, Color tint) {
  if (tex_id >= ATLAS_TEX_ID_MAX or tex_id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::draw_atlas_texture_stretch()::Texture id is out of bound");
    return;
  }
  const atlas_texture* tex = ss_get_atlas_texture_by_enum(tex_id);
  if (not tex or tex == nullptr) {
    IERROR("user_interface::draw_atlas_texture_stretch()::Texture resource is invalid");
    return;
  }
  if (should_center) {
    dest.x -= dest.width * 0.5f;
    dest.y -= dest.height * 0.5f;
  }
  
  if (stretch_part.width <= 0 or stretch_part.height <= 0 or stretch_part.x < 0 or stretch_part.y < 0 
    or (stretch_part.x + stretch_part.width  - tex->atlas_handle->width  > tex->source.width) 
    or (stretch_part.y + stretch_part.height - tex->atlas_handle->height > tex->source.height)) {
    IWARN("user_interface::draw_atlas_texture_stretch()::Invalid stretch_part dimensions");
    return;
  }
  f32 left_src_width_ratio = stretch_part.x / stretch_part.height;
  f32 right_src_width_ratio = (tex->source.width - (stretch_part.x + stretch_part.width)) / stretch_part.height;
  
  Rectangle left_source = {0, stretch_part.y, stretch_part.x, stretch_part.height};
  Rectangle right_source = {stretch_part.x + stretch_part.width, stretch_part.y, tex->source.width - (stretch_part.x + stretch_part.width), stretch_part.height};
  
  Rectangle left_dest = {dest.x, dest.y, dest.height * left_src_width_ratio, dest.height};
  Rectangle right_dest = {dest.x + dest.width - (dest.height * right_src_width_ratio), dest.y, dest.height * right_src_width_ratio, dest.height};
  Rectangle middle_dest = {left_dest.x + left_dest.width, dest.y, dest.width - (left_dest.width + right_dest.width), dest.height};
  
  if (middle_dest.width < 0) {
    middle_dest.width = 0;
  }
  gui_draw_atlas_texture_id_pro(tex_id, left_source, left_dest, false, TEXTURE_WRAP_CLAMP, tint);
  gui_draw_atlas_texture_id_pro(tex_id, stretch_part, middle_dest, false, TEXTURE_WRAP_REPEAT, tint);
  gui_draw_atlas_texture_id_pro(tex_id, right_source, right_dest, false, TEXTURE_WRAP_CLAMP, tint);
}

atlas_texture_id draw_scrolling_textures(
  const std::vector<atlas_texture_id>& item_tex_ids, f32 _offset, f32 draw_width, Rectangle center_unit, bool use_dest_dim, f32 unit_gap, Color item_tint, f32 source_scale,
  f32* out_distance_to_center
) {
  if (item_tex_ids.empty()) {
    return ATLAS_TEX_ID_UNSPECIFIED;
  }
  auto wrap_index = [](i32 index, i32 size) -> i32 {
    return (index % size + size) % size;
  };

  const i32 texture_array_size = static_cast<i32>(item_tex_ids.size());
  const Rectangle _center_rect = center_unit;
  const f32 true_center = _center_rect.x + _center_rect.width * .5f;
  const f32 unit_width_with_gap = center_unit.width + unit_gap;

  // Calculate total number of units needed to fill the screen plus a buffer on each side
  const f32 unit_count_f = math_ceil((draw_width + unit_width_with_gap * 2.f) / unit_width_with_gap);
  const i32 unit_count = static_cast<i32>(unit_count_f);

  // --- Fixed Scrolling Logic ---
  // 1. Get the integer number of units scrolled
  const i32 tex_id_scroll = math_floor(_offset / unit_width_with_gap);
  
  // 2. Get the remainder, which is always positive and consistent with floor
  const f32 scroll_remainder = _offset - (static_cast<f32>(tex_id_scroll) * unit_width_with_gap);

  // 3. Apply the remainder (subtract for natural scroll direction)
  center_unit.x -= scroll_remainder;
  // -----------------------------

  // Lambda helper to avoid repeating draw logic
  auto draw_unit = [&](atlas_texture_id tex_id, Rectangle dest) {
    Vector2 origin = {dest.width * .5f, dest.height * .5f};
    if (not use_dest_dim) {
      const atlas_texture* const tex = ss_get_atlas_texture_by_enum(tex_id);
      if (tex) { // Safety check
        dest.width = tex->source.width * source_scale;
        dest.height = tex->source.height * source_scale;
        origin = {dest.width * .5f, dest.height * .5f};
      }
    }
    gui_draw_atlas_texture_id(tex_id, dest, origin, 0.f, item_tint);
  };

  // Calculate the texture index for the base unit
  const i32 base_center_index = math_floor(unit_count_f * .5f) + tex_id_scroll;
  const i32 center_unit_index = wrap_index(base_center_index, texture_array_size);

  // Draw the center unit
  Rectangle _center_unit = center_unit;
  draw_unit(item_tex_ids.at(center_unit_index), _center_unit);

  // Loop outwards from the center, drawing two items per loop
  for (i32 i = 1; i <= (unit_count / 2) + 1; i++) {
    // --- LEFT ITEM ---
    const f32 leftmost_x = center_unit.x - (i * unit_width_with_gap);
    Rectangle leftmost_dest = {leftmost_x, center_unit.y, center_unit.width, center_unit.height};
    // Fixed: Index is also minus
    const size_t leftmost_tex_id = static_cast<size_t>(wrap_index(center_unit_index - i, texture_array_size));
    draw_unit(item_tex_ids.at(leftmost_tex_id), leftmost_dest);

    // --- RIGHT ITEM ---
    const f32 rightmost_x = center_unit.x + (i * unit_width_with_gap);
    Rectangle rightmost_dest = {rightmost_x, center_unit.y, center_unit.width, center_unit.height};
    // Fixed: Index is also plus
    const size_t rightmost_tex_id = static_cast<size_t>(wrap_index(center_unit_index + i, texture_array_size));
    draw_unit(item_tex_ids.at(rightmost_tex_id), rightmost_dest);
  }
  
  // --- Logic for finding the "most central" item ---
  atlas_texture_id most_center = ATLAS_TEX_ID_UNSPECIFIED;
  f32 distance_to_center = 0.f;
  const bool units_moving_right = _offset >= 0;

  if (units_moving_right) {
    const bool center_unit_is_on_center = ((true_center + (unit_gap * .5f)) - center_unit.x) >= 0;
    if (center_unit_is_on_center) {
      most_center = item_tex_ids.at(center_unit_index);
      distance_to_center = _center_rect.x - center_unit.x;
    } else {
      most_center = item_tex_ids.at(static_cast<size_t>(wrap_index(center_unit_index + 1, texture_array_size)));
      distance_to_center = _center_rect.x + _center_rect.width + unit_gap - center_unit.x;
    }
  } else {
    const bool center_unit_is_on_center = ((center_unit.x + center_unit.width + unit_gap * .5f) - true_center) >= 0;
    if (center_unit_is_on_center) {
      most_center = item_tex_ids.at(center_unit_index);
      distance_to_center = center_unit.x - _center_rect.x;
    } else {
      most_center = item_tex_ids.at(static_cast<size_t>(wrap_index(center_unit_index - 1, texture_array_size)));
      distance_to_center = _center_rect.x - _center_rect.width - unit_gap + center_unit.x;
    }
  }

  // Cleaner pointer check
  if (out_distance_to_center) {
    *out_distance_to_center = distance_to_center;
  }
  
  return most_center;
}

void gui_slider(slider_id _id, Vector2 pos, Vector2 grid) {
  IF_NOT_STATE("gui_slider", return; );

  if (_id >= SDR_ID_MAX or _id <= SDR_ID_UNDEFINED or not state or state == nullptr) {
    IWARN("user_interface::gui_slider()::One of recieved ids was out of bound");
    return;
  }
  slider *const sdr = __builtin_addressof(state->sliders.at(_id));

  const slider_type *const sdr_type = __builtin_addressof(sdr->sdr_type);

  pos.x -= sdr_type->dest_frame_dim.x * .5f;
  pos.y -= sdr_type->dest_frame_dim.y * .5f;

  sdr->position = position_element_by_grid(pos, grid, SCREEN_OFFSET);
  if (not sdr->is_registered or (sdr_type->id == SDR_TYPE_OPTION and sdr->options.size() >= MAX_SLIDER_OPTION_SLOT)) return;
 
  Vector2 btn_left_dest = Vector2 { 
    sdr->position.x - state->button_types.at(sdr_type->left_btn_type_id).dest_frame_dim.x, 
    sdr->position.y - (state->button_types.at(sdr_type->left_btn_type_id).dest_frame_dim.y * .5f) + (sdr_type->dest_frame_dim.y * .5f)
  };
  Vector2 btn_right_dest = Vector2 { 
    sdr->position.x + sdr_type->body_width * sdr_type->width_multiply, 
    sdr->position.y - (state->button_types.at(sdr_type->right_btn_type_id).dest_frame_dim.y * .5f) + (sdr_type->dest_frame_dim.y * .5f)
  };

  sdr->on_screen = true;
  draw_slider_body(sdr);
  if(sdr_type->left_btn_id != 0) if (gui_slider_button(sdr_type->left_btn_id, btn_left_dest)) {
    if (sdr->on_left_button_trigger != nullptr) {
      sdr->on_left_button_trigger();
    }
    else if (sdr->current_value > 0) {
      sdr->current_value--;
    }
  }

  if(sdr_type->right_btn_id != 0) if (gui_slider_button(sdr_type->right_btn_id, btn_right_dest)) {
    if (sdr->on_right_button_trigger != nullptr) {
      sdr->on_right_button_trigger();
    }
    else if (static_cast<size_t>(sdr->current_value) < sdr->options.size() - 1u) {
      sdr->current_value++;
    }
  }
}
void draw_slider_body(const slider *const sdr) {
  IF_NOT_STATE("draw_slider_body", return; );

  if (not sdr or sdr == nullptr ) {
    IERROR("user_interface::draw_slider_body()::Slider is invalid");
    return;
  }

  slider_type sdr_type = sdr->sdr_type;

  switch (sdr->sdr_type.id) {
    case SDR_TYPE_PERCENT: {
      const spritesheet * circle_sprite = ss_get_spritesheet_by_enum(SHEET_ID_SLIDER_PERCENT);
      u16 total_body_width = sdr_type.body_width * sdr_type.width_multiply;
      u16 each_body_width = (total_body_width - ((DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH) * SCREEN_OFFSET.x)) / DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH;
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = Vector2 {each_body_scale, each_body_scale};
      Vector2 _pos_temp = Vector2 {
        sdr->position.x + each_body_width + SCREEN_OFFSET.x * 1.4f, 
        sdr->position.y - (circle_sprite->current_frame_rect.height * draw_sprite_scale.y * .5f) + (sdr_type.dest_frame_dim.y * .5f)
      };
      for (i32 iter = 0; iter < DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH; ++iter) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + SCREEN_OFFSET.x) * (iter-1); 
        draw_sprite_on_site_by_id(SHEET_ID_SLIDER_PERCENT, WHITE, _pos, draw_sprite_scale, (iter < sdr->current_value) ? 1 : 0);
      }
      break;
    }
    case SDR_TYPE_OPTION: {
      u16 total_body_width = sdr_type.body_width * sdr_type.width_multiply;
      u16 each_body_width = (total_body_width - ((sdr->options.size()) * SCREEN_OFFSET.x)) / sdr->options.size();
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = Vector2 {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = Vector2 {sdr->position.x + SCREEN_OFFSET.x, sdr->position.y};
      std::string text = std::string();
      if (sdr->localize_text) {
        text = lc_txt(sdr->options.at(sdr->current_value).localization_symbol);
      } else {
        text = sdr->options.at(sdr->current_value).no_localized_text;
      }

      for (size_t itr_000 = 0; itr_000 < sdr->options.size(); ++itr_000) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + SCREEN_OFFSET.x) * (itr_000); 
        draw_sprite_on_site_by_id(sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, (itr_000 == static_cast<size_t>(sdr->current_value)) ? 1u : 0u);
      }
      Vector2 text_pos = Vector2 { sdr->position.x + total_body_width * .5f, sdr->position.y + sdr_type.dest_frame_dim.y * .5f };
      draw_text(text.c_str(), text_pos, SLIDER_FONT, DEFAULT_SLIDER_FONT_SIZE, BUTTON_TEXT_UP_COLOR, true, true, false, VECTOR2(0.f, 0.f));
      break;
    }
    case SDR_TYPE_NUMBER: {
      i32 total_body_width = static_cast<i32>(sdr_type.body_width) * static_cast<i32>(sdr_type.width_multiply);
      f32 each_body_width = (static_cast<f32>(total_body_width) - static_cast<f32>(sdr->options.size()) * SCREEN_OFFSET.x) / static_cast<f32>(sdr->options.size() - 1.f);
      f32 each_body_scale = static_cast<f32>(each_body_width) / static_cast<f32>(sdr_type.origin_body_width);
      Vector2 draw_sprite_scale = Vector2 {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = Vector2 {sdr->position.x + SCREEN_OFFSET.x, sdr->position.y};
      std::string text = sdr->options.at(0).no_localized_text;
      
      for (size_t itr_000 = 0; itr_000 < sdr->options.size(); ++itr_000) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + SCREEN_OFFSET.x) * (itr_000-1); 
        draw_sprite_on_site_by_id(sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, (itr_000 == static_cast<size_t>(sdr->current_value) ? 1u : 0u));
      }
      Vector2 text_pos = Vector2 { sdr->position.x + total_body_width * .5f, sdr->position.y + sdr_type.dest_frame_dim.y * .5f };

      draw_text(text.c_str(), text_pos, SLIDER_FONT, DEFAULT_SLIDER_FONT_SIZE, BUTTON_TEXT_UP_COLOR, true, true, false, VECTOR2(0.f, 0.f));
      break;
    }
    default: IWARN("user_interface::render_slider_body()::Unsupported slider type");
    break;
  }
}
void gui_panel(panel pan, Rectangle dest, bool _should_center) {
  Rectangle bg_dest = dest;
  if (_should_center) {
    bg_dest.x -= bg_dest.width / 2.f;
    bg_dest.y -= bg_dest.height / 2.f;
  }
  DrawRectanglePro(bg_dest, Vector2 {0, 0}, 0, pan.bg_tint);
  draw_atlas_texture_npatch(pan.frame_tex_id, dest, pan.offsets, _should_center);
}
bool gui_panel_active(panel *const pan, Rectangle dest, bool _should_center) {
  IF_NOT_STATE("gui_panel_active", return false; );

  if (not pan or pan == nullptr) {
    IWARN("user_interface::gui_panel_active()::Panel is invalid");
    return false;
  }
  if (pan->signal_state == BTN_STATE_UNDEFINED) {
    IWARN("user_interface::gui_panel_active()::Panel was not meant to be active");
    return false;
  }
  if (_should_center) {
    dest.x -= dest.width / 2.f;
    dest.y -= dest.height / 2.f;
  }

  if (CheckCollisionPointRec(state->mouse_pos_screen, dest)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      pan->current_state = BTN_STATE_PRESSED;
    } else {
      if (pan->current_state == BTN_STATE_PRESSED) {
        pan->current_state = BTN_STATE_RELEASED;
      }
      else if (pan->current_state != BTN_STATE_HOVER) {
        pan->current_state = BTN_STATE_HOVER;
      }
    }
  } else {
    if (pan->current_state != BTN_STATE_UP) { 
      pan->current_state = BTN_STATE_UP;
    }
  }

  if(pan->current_state == BTN_STATE_HOVER) {
    draw_atlas_texture_regular(pan->bg_tex_id, dest, pan->bg_hover_tint, false);
    //DrawRectanglePro(dest, Vector2 {0, 0}, 0, pan->bg_hover_tint)
  }
  else {
    draw_atlas_texture_regular(pan->bg_tex_id, dest, pan->bg_tint, false);
    //DrawRectanglePro(dest, Vector2 {0, 0}, 0, pan->bg_tint);
  }

  draw_atlas_texture_npatch(pan->frame_tex_id, dest, pan->offsets, false);

  return pan->current_state == pan->signal_state;
}
void gui_label_box(const char* text, font_type type, i32 font_size, Rectangle dest, Color tint, text_alignment alignment) {
  if (not text or text == nullptr) {
    return;
  }
  Font font = font_type_to_font(type);
  Vector2 text_measure = MeasureTextEx(font, text, font.baseSize * font_size, UI_FONT_SPACING);
  Vector2 position = ui_align_text(dest, text_measure, alignment);
  draw_text_simple(text, position, type, font_size, tint);
}
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v) {
  if (not text or text == nullptr) {
    return;
  }
  draw_text(text, position, type, font_size, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
}
void gui_label_shader(const char* text, shader_id sdr_id, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v) {
  if (not text or text == nullptr) {
    return;
  }
  if (not state->display_language or state->display_language == nullptr) {
    draw_text_shader(text, sdr_id, position, GetFontDefault(), font_size * GetFontDefault().baseSize, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
    return;
  }
  switch (type) {
    case FONT_TYPE_ITALIC: {
      draw_text_shader(text, sdr_id, position, UI_ITALIC_FONT, font_size * UI_ITALIC_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_LIGHT: {
      draw_text_shader(text, sdr_id, position, UI_LIGHT_FONT, font_size * UI_LIGHT_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_REGULAR: {
      draw_text_shader(text, sdr_id, position, UI_REGULAR_FONT, font_size * UI_REGULAR_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_BOLD: {
      draw_text_shader(text, sdr_id, position, UI_BOLD_FONT, font_size * UI_BOLD_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_TITLE: {
      draw_text_shader(text, sdr_id, position, UI_TITLE_FONT, font_size * UI_TITLE_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    default: IWARN("user_interface::gui_label_shader()::Unsupported font type");
    break;
  }
}
void gui_label_wrap(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center) {
  if (not text or text == nullptr) {
    return;
  }
  if (_should_center) {
    position.x -= (position.width / 2.f);
    position.y -= (position.height / 2.f);
  }
  if (not state->display_language or state->display_language == nullptr) {
    DrawTextBoxed(GetFontDefault(), text, position, font_size * GetFontDefault().baseSize, UI_FONT_SPACING, true, tint);
    return;
  }
  switch (type) {
    case FONT_TYPE_ITALIC: {
      DrawTextBoxed(UI_ITALIC_FONT, text, position, font_size * UI_ITALIC_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_LIGHT: {
      DrawTextBoxed(UI_LIGHT_FONT, text, position, font_size * UI_LIGHT_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_REGULAR: {
      DrawTextBoxed(UI_REGULAR_FONT, text, position, font_size * UI_REGULAR_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_BOLD: {
      DrawTextBoxed(UI_BOLD_FONT, text, position, font_size * UI_BOLD_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_TITLE: {
      DrawTextBoxed(UI_TITLE_FONT, text, position, font_size * UI_TITLE_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    default: IWARN("user_interface::gui_label_wrap()::Unsupported font type");
    break;
  }
}
void gui_label_grid(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v, Vector2 grid_coord) {
  if (not text or text == nullptr) {
    return;
  }
  draw_text(text, position, type, font_size, tint, _center_h, _center_v, true, grid_coord);
}
void gui_label_wrap_grid(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center, Vector2 grid_pos) {
  if (not text or text == nullptr) {
    return;
  }
  Vector2 _position = position_element_by_grid(grid_pos, VECTOR2(position.x, position.y), SCREEN_OFFSET);
  position.x = _position.x;
  position.y = _position.y;

  if (_should_center) {
    position.x -= (position.width / 2.f);
    position.y -= (position.height / 2.f);
  }
  if (not state->display_language or state->display_language == nullptr) {
    DrawTextBoxed(GetFontDefault(), text, position, font_size * GetFontDefault().baseSize, UI_FONT_SPACING, true, tint);
    return;
  }
  switch (type) {
    case FONT_TYPE_ITALIC: {
      DrawTextBoxed(UI_ITALIC_FONT, text, position, font_size * UI_ITALIC_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_LIGHT: {
      DrawTextBoxed(UI_LIGHT_FONT, text, position, font_size * UI_LIGHT_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_REGULAR: {
      DrawTextBoxed(UI_REGULAR_FONT, text, position, font_size * UI_REGULAR_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_BOLD: {
      DrawTextBoxed(UI_BOLD_FONT, text, position, font_size * UI_BOLD_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_TITLE: {
      DrawTextBoxed(UI_TITLE_FONT, text, position, font_size * UI_TITLE_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    default: IWARN("user_interface::gui_label_wrap_grid()::Unsupported font type");
    break;
  }
}
void gui_draw_settings_screen(void) { // TODO: Return to settings later
  gui_draw_default_background_panel();

  gui_slider(SDR_ID_SETTINGS_SOUND_SLIDER, UI_BASE_RENDER_DIV2, VECTOR2(0.f,-20.f));

  gui_slider(SDR_ID_SETTINGS_RES_SLIDER, UI_BASE_RENDER_DIV2, VECTOR2(0.f, -10.f));

  gui_slider(SDR_ID_SETTINGS_WIN_MODE_SLIDER, UI_BASE_RENDER_DIV2, VECTOR2(0.f, 0.f));

  gui_slider(SDR_ID_SETTINGS_LANGUAGE, UI_BASE_RENDER_DIV2, VECTOR2(0.f, 10.f));

  if(gui_menu_button(lc_txt(LOC_TEXT_SETTINGS_BUTTON_APPLY), BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, VECTOR2(35.f, 66.5f), UI_BASE_RENDER_DIV2, true)) {

    i32 window_mod = SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_WIN_MODE_SLIDER).content.data.i32[0];
    Vector2 new_res = pVECTOR2(SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.i32);

    if (window_mod == FLAG_BORDERLESS_WINDOWED_MODE && !IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
      event_fire(EVENT_CODE_TOGGLE_BORDERLESS, event_context());
    }
    else if (window_mod == FLAG_FULLSCREEN_MODE && !IsWindowFullscreen()) {
      event_fire(EVENT_CODE_TOGGLE_FULLSCREEN, event_context());
    }
    else if (window_mod == 0) {
      event_fire(EVENT_CODE_TOGGLE_WINDOWED, event_context(static_cast<i32>(new_res.x), static_cast<i32>(new_res.y)));
    }
    
    language_index index = static_cast<language_index>(SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_LANGUAGE).content.data.i32[0]);
    if (state->display_language->index != index) {
      if (loc_parser_set_active_language_by_index(index)) {
        loc_data* loc = loc_parser_get_active_language();
        if (not loc or loc == nullptr) {
          IERROR("user_interface::gui_draw_settings_screen()::Failed to get active language");
        }
        else {
          state->display_language = ui_get_localization_by_index(loc->index);
          set_language(loc->language_name.c_str());
        }
      }
    }
    

    ui_refresh_setting_sliders_to_default();
    save_ini_file();
  }
}
void ui_refresh_setting_sliders_to_default(void) {	
  /////////////////////// RESOLUTION SLIDER
  slider *const ressdr = get_slider_by_id(SDR_ID_SETTINGS_RES_SLIDER);
  if (ressdr and ressdr != nullptr) {
    ressdr->options.clear();
    { // Getting resolutions based on aspect ratio
	    const std::vector<std::pair<i32, i32>> *const supported_resolution_list = get_supported_render_resolutions();
	    for (size_t itr_000 = 0u; itr_000 < supported_resolution_list->size(); ++itr_000) {
	    	i32 _suppres_window_width  = supported_resolution_list->at(itr_000).first;
	    	i32 _suppres_window_height = supported_resolution_list->at(itr_000).second;
	    	gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, data_pack(DATA_TYPE_I32, 
          data128(_suppres_window_width, _suppres_window_height), 2) , 0, TextFormat("%dx%d", _suppres_window_width, _suppres_window_height)
        );
	    	SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(
          state->in_app_settings->window_width  == _suppres_window_width && 
          state->in_app_settings->window_height == _suppres_window_height, SDR_ID_SETTINGS_RES_SLIDER
        )
	    }
    }

	  { // Settings current resolution to corresponding slider index
	    ressdr->current_value = -1;
      const i32 _currres_window_width = state->in_app_settings->window_width;
      const i32 _currres_window_height = state->in_app_settings->window_height;
	    for (size_t itr_000 = 0; itr_000 < ressdr->options.size(); ++itr_000) {
	    	slider_option& option = ressdr->options.at(itr_000);
	    	if (option.content.data.i32[0] == _currres_window_width && option.content.data.i32[1] == _currres_window_height) {
	    		ressdr->current_value = itr_000;
	    		break;
	    	}
	    }
    }

	  { // If there is no equal slider option somehow
	    if (ressdr->current_value < 0) {
        ressdr->options.clear();
        const i32 _noeqres_window_width = state->in_app_settings->window_width;
        const i32 _noeqres_window_height = state->in_app_settings->window_height;
	      IWARN("user_interface::user_interface_system_initialize()::Setting window size to custom");
	    	gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, data_pack(DATA_TYPE_I32, 
          data128(_noeqres_window_width, _noeqres_window_height), 2) , 0, TextFormat("%dx%d", _noeqres_window_width, _noeqres_window_height)
        );
        SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(1, SDR_ID_SETTINGS_RES_SLIDER);
      }
    }
  } else {
	  IWARN("user_interface::ui_refresh_setting_sliders_to_default()::Resolution slider is invalid");
	  return;
  }
	/////////////////////// RESOLUTION SLIDER

  set_master_sound(state->in_app_settings->master_sound_volume, true);
	SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value = state->in_app_settings->master_sound_volume;

	for (size_t itr_000 = 0u; itr_000 < SDR_AT(SDR_ID_SETTINGS_WIN_MODE_SLIDER).options.size(); ++itr_000) {
    i32 window_mode_slider_val = SDR_AT(SDR_ID_SETTINGS_WIN_MODE_SLIDER).options.at(itr_000).content.data.i32[0];

		if (window_mode_slider_val == state->in_app_settings->window_state) {
			state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).current_value = itr_000;
			break;
		}
	}

	i32 language_index = loc_parser_get_active_language()->index;
	for (size_t itr_000 = 0u; itr_000 < SDR_AT(SDR_ID_SETTINGS_LANGUAGE).options.size(); ++itr_000) {
		data_pack opt = SDR_AT(SDR_ID_SETTINGS_LANGUAGE).options.at(itr_000).content;
		if (opt.data.i32[0] == language_index && (opt.type_flag < DATA_TYPE_MAX && opt.type_flag > DATA_TYPE_UNRESERVED)) {
			SDR_AT(SDR_ID_SETTINGS_LANGUAGE).current_value = itr_000;
			break;
		}
  }
}
void gui_draw_pause_screen(bool in_game_play_state) {
  Rectangle dest = Rectangle {
    UI_BASE_RENDER_DIV2.x,
    UI_BASE_RENDER_DIV2.y,
    UI_BASE_RENDER_WIDTH * .2f,
    UI_BASE_RENDER_HEIGHT * .6f
  };
  gui_panel(panel(), dest, true);

  if (in_game_play_state) {
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_PAUSE_BUTTON_TEXT_RESUME), BTN_ID_PAUSEMENU_BUTTON_RESUME, Vector2 {0.f, -15.f}, UI_BASE_RENDER_DIV2, true)) {
      event_fire(EVENT_CODE_RESUME_GAME, event_context());
    }
  }
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_SETTINGS), BTN_ID_PAUSEMENU_BUTTON_SETTINGS, Vector2 {0.f, -5.f}, UI_BASE_RENDER_DIV2, true)) {}
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_PAUSE_BUTTON_TEXT_EXIT_TO_MAINMENU), BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_MAIN_MENU, Vector2 { 0.f, 5.f}, UI_BASE_RENDER_DIV2, true)) {
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context(static_cast<i32>(false)));
  }
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_PAUSE_BUTTON_TEXT_EXIT_TO_DESKTOP), BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_DESKTOP, Vector2 { 0.f, 15.f}, UI_BASE_RENDER_DIV2, true)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, event_context());
  }
}
void gui_fire_display_error(int loc_text_id) {
  IF_NOT_STATE("gui_fire_display_error", return; );

  if (not state->errors_on_play.empty() and state->errors_on_play.back().error_text == lc_txt(loc_text_id)) {
    return;
  }
  ui_error_display_control_system& err = state->errors_on_play.emplace_back(ui_error_display_control_system());

  err.error_text = lc_txt(loc_text_id);
  err.bg_tex_id = ATLAS_TEX_ID_HEADER;
  err.duration = state->error_text_duration_in_and_out;
  err.location = state->error_text_start_position;
  err.display_state = ERROR_DISPLAY_ANIMATION_STATE_MOVE_IN;
}
void gui_display_error(ui_error_display_control_system err) {
  Font font = font_type_to_font(DEFAULT_ERROR_FONT_TYPE);
  Vector2 text_measure = MeasureTextEx(font, err.error_text.c_str(), font.baseSize * DEFAULT_ERROR_FONT_SIZE, UI_FONT_SPACING);
  Vector2 text_position = err.location;
  Rectangle bg_dest = Rectangle {text_position.x, text_position.y, text_measure.x * 1.25f, text_measure.y * 1.25f};

  text_position.x -= (text_measure.x * .5f);
  text_position.y -= (text_measure.y * .5f);

  // TODO: Make stretch part dynamic
  draw_atlas_texture_stretch(err.bg_tex_id, Rectangle {64, 0, 32, 32}, bg_dest, true, WHITE);
  draw_text_simple(err.error_text.c_str(), text_position, DEFAULT_ERROR_FONT_TYPE, DEFAULT_ERROR_FONT_SIZE, WHITE);
}
Rectangle gui_draw_default_background_panel(void) {
  Rectangle header_loc = {0, 0, static_cast<f32>(UI_BASE_RENDER_WIDTH), UI_BASE_RENDER_HEIGHT * .1f};
  Rectangle footer_loc = {0, 
    UI_BASE_RENDER_HEIGHT - UI_BASE_RENDER_HEIGHT *.1f, 
    static_cast<f32>(UI_BASE_RENDER_WIDTH), UI_BASE_RENDER_HEIGHT * .1f};
  DrawRectangleRec(header_loc, Color{0, 0, 0, 50});
  DrawRectangleRec(footer_loc, Color{0, 0, 0, 50});

  state->default_background_panel.dest = Rectangle{ UI_BASE_RENDER_WIDTH * .025f, UI_BASE_RENDER_HEIGHT * .075f, UI_BASE_RENDER_WIDTH * .95f, UI_BASE_RENDER_HEIGHT * .85f};
  gui_panel(state->default_background_panel, state->default_background_panel.dest, false);
  return state->default_background_panel.dest;
}
void combat_feedback_spawn_floating_text(const char* _text, combat_feedback_floating_text_type type, Vector2 start_position) {
  IF_NOT_STATE("combat_feedback_spawn_floating_text", return; );

  atlas_texture_id bg_tex_id = ATLAS_TEX_ID_UNSPECIFIED;
  f32 bg_tex_scale = 1.f;
  switch (type) {
    case COMBAT_FEEDBACK_FLOATING_TEXT_TYPE_DAMAGE: {
      bg_tex_id = ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG;
      bg_tex_scale = 0.5f;
      break;
    }
    case COMBAT_FEEDBACK_FLOATING_TEXT_TYPE_HEAL: {
      bg_tex_id = ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG;
      bg_tex_scale = 0.5f;
      break;
    }
    case COMBAT_FEEDBACK_FLOATING_TEXT_TYPE_CONDITION: {
      bg_tex_id = ATLAS_TEX_ID_DARK_FANTASY_PANEL_BG;
      bg_tex_scale = 0.5f;
      break;
    }
    default: {
      return;
    }
  }
  const f32 text_duration = get_random(
    static_cast<i32>(state->cfft_display_state.duration_min * 100.f), 
    static_cast<i32>(state->cfft_display_state.duration_max * 100.f)
  ) * 0.01f;

  const f32 _font_nonpixel_size_nat = 1.f;
  const Font cfft_font = ui_get_font(FONT_TYPE_ITALIC);
  const Vector2 text_measure = MeasureTextEx(cfft_font, _text, cfft_font.baseSize * _font_nonpixel_size_nat, UI_FONT_SPACING);
  Vector2 _end_pos = Vector2 {
    start_position.x - (text_measure.x * 2.f),
    start_position.y + (text_measure.y * 1.5f),
  };
  state->cfft_display_state.queue.push_back(combat_feedback_floating_text(
    state->cfft_display_state.next_cfft_id++,
    _text,
    FONT_TYPE_ITALIC,
    _font_nonpixel_size_nat,
    start_position,
    _end_pos,
    type,
    text_duration,
    bg_tex_id,
    bg_tex_scale,
    Color { 223, 249, 251, 95},
    Color { 223, 249, 251, 255}
  ));
}
void combat_feedback_update_floating_texts(f32 delta_time) {
  IF_NOT_STATE("combat_feedback_update_floating_texts", return; );

  floating_text_display_system_state& system = state->cfft_display_state;

  for (auto iterator = system.queue.begin(); iterator != system.queue.end();) {
    iterator->accumulator += delta_time;
    iterator->accumulator = FCLAMP(iterator->accumulator, 0, iterator->duration);

    Vector2& interpolate = iterator->interpolate;
    const Font font = ui_get_font(iterator->font_type);
    
    f32 change_x = iterator->initial.x - iterator->target.x;
    f32 change_y = iterator->initial.y - iterator->target.y;

    interpolate.x = EaseQuadIn(iterator->accumulator, iterator->initial.x, change_x, iterator->duration);
    interpolate.y = EaseCubicOut(iterator->accumulator, iterator->initial.y, change_y, iterator->duration);

    interpolate = GetWorldToScreen2D(iterator->interpolate, state->in_camera_metrics->handle);

    iterator->interpolated_font_size = EaseBounceInOut(
      iterator->accumulator, 
      iterator->initial_font_size * font.baseSize, 
      -iterator->initial_font_size * font.baseSize, 
      iterator->duration
    );
    const f32 bg_tex_font_ratio = 1.5f;
    const Vector2 text_measure = MeasureTextEx(font, iterator->text.c_str(), iterator->interpolated_font_size, UI_FONT_SPACING);
    const Vector2 texture_size = Vector2 {text_measure.x * bg_tex_font_ratio, text_measure.y * bg_tex_font_ratio};
    Vector2 texture_position = Vector2 { 
      iterator->interpolate.x + text_measure.x * .5f, 
      iterator->interpolate.y + text_measure.y * .5f
    };
    iterator->tex_dest = Rectangle {
      texture_position.x, 
      texture_position.y,
      texture_size.x,
      texture_size.y
    };
    iterator->tex_origin = Vector2 {
      iterator->tex_dest.width * .5f, 
      iterator->tex_dest.height * .5f
    };

    if (iterator->accumulator >= iterator->duration) {
      iterator = system.queue.erase(iterator);
    } else {
      ++iterator;
    }
  }
}
void combat_feedback_render_floating_texts(void) {
  IF_NOT_STATE("combat_feedback_render_floating_texts", return; );

  floating_text_display_system_state& system = state->cfft_display_state;

  for (auto iterator = system.queue.begin(); iterator != system.queue.end(); iterator++) {
    gui_draw_atlas_texture_id(iterator->background_tex_id, iterator->tex_dest, iterator->tex_origin, 0.f, iterator->background_tint);
    draw_text_ex(iterator->text.c_str(), iterator->interpolate, iterator->font_type, iterator->interpolated_font_size, iterator->font_tint);
  }
}

bool gui_slider_add_option(slider_id _id, data_pack content, i32 _localization_symbol, std::string _no_localized_text) {
  if (_id >= SDR_ID_MAX or _id <= SDR_ID_UNDEFINED or not state or state == nullptr) {
    IWARN("user_interface::gui_slider_add_option()::Slider id was out of bound");
    return false;
  }
  slider *const sdr = __builtin_addressof(state->sliders.at(_id));
  if (not sdr->is_registered) {
    IWARN("user_interface::gui_slider_add_option()::Slider didn't registered");
    return false;
  }
  if (sdr->options.size() < MAX_SLIDER_OPTION_SLOT) {
    sdr->options.push_back(slider_option(_no_localized_text.c_str(), _localization_symbol, content));
    return true;
  }
  else {
    IWARN("user_interface::gui_slider_add_option()::Reached the maximum amouth of option slot");
    return false;
  }
}
void register_button_type(button_type_id _btn_type_id, spritesheet_id _ss_type, Vector2 frame_dim, f32 _scale, bool _should_center) {
  IF_NOT_STATE("register_button_type", return; );

  if (_ss_type >= SHEET_ID_SPRITESHEET_TYPE_MAX or _ss_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED or _btn_type_id >= BTN_TYPE_MAX or _btn_type_id <= BTN_TYPE_UNDEFINED) {
    IWARN("user_interface::register_button_type()::Recieved id was out of bound");
    return;
  }
  button_type btn_type = button_type(_btn_type_id, _ss_type, frame_dim, VECTOR2(frame_dim.x * _scale, frame_dim.y * _scale), _scale, _should_center);
  state->button_types.at(_btn_type_id) = btn_type;
}
void register_button(button_id _btn_id, button_type_id _btn_type_id) {
  if (not state or state == nullptr) {
    IERROR("user_interface::register_button()::State is invalid");
    return;
  }
  if (_btn_id >= BTN_ID_MAX or _btn_id <= BTN_ID_UNDEFINED or _btn_type_id >= BTN_TYPE_MAX or _btn_type_id <= BTN_TYPE_UNDEFINED) {
    IWARN("user_interface::register_button()::One of recieved ids was out of bound");
    return;
  }
  button_type *const _btn_type = __builtin_addressof(state->button_types.at(_btn_type_id));
  button btn = button(_btn_id, (*_btn_type), BTN_STATE_RELEASED, Rectangle{0.f, 0.f, _btn_type->dest_frame_dim.x, _btn_type->dest_frame_dim.y});

  state->buttons.at(_btn_id) = btn;
}
void register_checkbox(checkbox_id _cb_id, checkbox_type_id _cb_type_id) {
  IF_NOT_STATE("register_checkbox", return; );

  if (_cb_id >= CHECKBOX_ID_MAX or _cb_id <= CHECKBOX_ID_UNDEFINED or _cb_type_id >= CHECKBOX_TYPE_MAX or _cb_type_id <= CHECKBOX_TYPE_UNDEFINED) {
    IWARN("user_interface::register_checkbox()::One of recieved ids was out of bound");
    return;
  }
  checkbox_type *const _cb_type = __builtin_addressof(state->checkbox_types.at(_cb_type_id));
  checkbox cb = checkbox(_cb_id, state->checkbox_types.at(_cb_type_id), Rectangle{0.f, 0.f, _cb_type->dest_frame_dim.x, _cb_type->dest_frame_dim.y});

  state->checkboxes.at(_cb_id) = cb;
}
void register_checkbox_type(checkbox_type_id _cb_type_id, spritesheet_id _ss_type, Vector2 frame_dim, f32 _scale, bool _should_center) {
  IF_NOT_STATE("register_checkbox_type", return; );

  if (_ss_type >= SHEET_ID_SPRITESHEET_TYPE_MAX or _ss_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED or _cb_type_id >= CHECKBOX_TYPE_MAX or _cb_type_id <= CHECKBOX_TYPE_UNDEFINED) {
    IWARN("user_interface::register_checkbox_type()::Recieved id was out of bound");
    return;
  }
  checkbox_type cb_type = checkbox_type(_cb_type_id, _ss_type, frame_dim, VECTOR2(frame_dim.x * _scale, frame_dim.y * _scale), _scale, _should_center);
  state->checkbox_types.at(_cb_type_id) = cb_type;
}
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id) {
  IF_NOT_STATE("register_progress_bar", return; );

  if (_id >= PRG_BAR_ID_MAX or _id <= PRG_BAR_ID_UNDEFINED or _type_id>= PRG_BAR_TYPE_ID_MAX or _type_id <= PRG_BAR_TYPE_ID_UNDEFINED) {
    IWARN("user_interface::register_progress_bar()::Recieved id was out of bound");
    return;
  }
  progress_bar prg_bar = progress_bar();

  prg_bar.type = state->prg_bar_types.at(_type_id);
  prg_bar.id = _id;
  prg_bar.is_initialized = true;

  state->prg_bars.at(_id) = prg_bar;
}
void register_progress_bar_type(progress_bar_type prg_bar_type) {
  IF_NOT_STATE("register_progress_bar_type", return; );

  if (prg_bar_type.id >= PRG_BAR_TYPE_ID_MAX or prg_bar_type.id <= PRG_BAR_TYPE_ID_UNDEFINED) {
    IWARN("user_interface::register_progress_bar_type()::Id(s) is/are out of bound");
    return;
  }
  if (prg_bar_type.body_inside >= ATLAS_TEX_ID_MAX or prg_bar_type.body_inside <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::register_progress_bar_type()::Id(s) is/are out of bound");
    return;
  }
  if (prg_bar_type.body_outside >= ATLAS_TEX_ID_MAX or prg_bar_type.body_outside <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::register_progress_bar_type()::Id(s) is/are out of bound");
    return;
  }
  if (prg_bar_type.mask_shader_id >= SHADER_ID_MAX or prg_bar_type.mask_shader_id <= SHADER_ID_UNSPECIFIED) {
    IWARN("user_interface::register_progress_bar_type()::Id(s) is/are out of bound");
    return;
  }
  
  state->prg_bar_types.at(prg_bar_type.id) = prg_bar_type;
}
void register_slider_type(
  slider_type_id _sdr_type_id, spritesheet_id _ss_sdr_body_type, f32 _scale, i32 _width_multiply, button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, i32 _char_limit) 
{
  IF_NOT_STATE("register_slider_type", return; );

  if (_ss_sdr_body_type >= SHEET_ID_SPRITESHEET_TYPE_MAX or _ss_sdr_body_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED or _sdr_type_id >= SDR_TYPE_MAX or _sdr_type_id <= SDR_TYPE_UNDEFINED) {
    IWARN("WARNING::user_interface::register_slider_type()::Id(s) is/are out of bound");
    return;
  }
  if (_left_btn_type_id >= BTN_TYPE_MAX or _left_btn_type_id <= BTN_TYPE_UNDEFINED or _right_btn_type_id >= BTN_TYPE_MAX or _right_btn_type_id <= BTN_TYPE_UNDEFINED) {
    IWARN("WARNING::user_interface::register_slider_type()::Id(s) is/are out of bound");
    return;
  }

  spritesheet ss_body = (*ss_get_spritesheet_by_enum(_ss_sdr_body_type));
  const button_type *const left_btn_type = __builtin_addressof(state->button_types.at(_left_btn_type_id)); 
  const button_type *const right_btn_type = __builtin_addressof(state->button_types.at(_right_btn_type_id)); 

  slider_type sdr_type = slider_type(_sdr_type_id, _ss_sdr_body_type, 
    VECTOR2(ss_body.current_frame_rect.width, ss_body.current_frame_rect.height), 
    _scale, _width_multiply, 
    static_cast<i32>(_scale * _char_limit), // Char limit
    _left_btn_type_id, _right_btn_type_id, // Button ids
    static_cast<i32>(left_btn_type->source_frame_dim.x * left_btn_type->scale),  // Left button width
    static_cast<i32>(right_btn_type->source_frame_dim.x * right_btn_type->scale), // Right button width
    static_cast<i32>(ss_body.current_frame_rect.width), // Original body width
    static_cast<i32>(ss_body.current_frame_rect.width * _scale) // Body width
  );

  sdr_type.dest_frame_dim = Vector2 { static_cast<f32>(sdr_type.body_width) * sdr_type.width_multiply, ss_body.current_frame_rect.height * _scale };

  state->slider_types.at(_sdr_type_id) = sdr_type;
}
/**
 * @param _is_clickable for SDR_TYPE_PERCENT type sliders. Does not affect others
 */
void register_slider(slider_id _sdr_id, slider_type_id _sdr_type_id, button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable, bool _localize_text) {
  IF_NOT_STATE("register_slider", return; );

  if (_sdr_id >= SDR_ID_MAX or _sdr_id <= SDR_ID_UNDEFINED) {
    IWARN("user_interface::register_slider()::Id(s) is/are out of bound");
    return;
  }
  if (_left_btn_id  >= BTN_ID_MAX or _left_btn_id  < BTN_ID_UNDEFINED) {
    IWARN("user_interface::register_slider()::Id(s) is/are out of bound");
    return;
  }
  if (_right_btn_id >= BTN_ID_MAX or _right_btn_id < BTN_ID_UNDEFINED) {
    IWARN("user_interface::register_slider()::Id(s) is/are out of bound");
    return;
  }
  const slider_type *const _sdr_type = __builtin_addressof(state->slider_types.at(_sdr_type_id));
  slider sdr = slider(_sdr_id, *_sdr_type, 0u, _localize_text, _is_clickable);

  if(_left_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.left_btn_id = _left_btn_id;
    register_button(sdr.sdr_type.left_btn_id, sdr.sdr_type.left_btn_type_id);
  }
  if(_right_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.right_btn_id = _right_btn_id;
    register_button(sdr.sdr_type.right_btn_id, sdr.sdr_type.right_btn_type_id);
  }
  if (_sdr_type_id == SDR_TYPE_NUMBER) { // To display its value
    sdr.options.push_back(slider_option());
  }
  state->sliders.at(_sdr_id) = sdr;
}
void gui_draw_atlas_texture_to_background(atlas_texture_id _id) {
  draw_atlas_texture_regular(_id, Rectangle {0.f, 0.f, static_cast<f32>(UI_BASE_RENDER_WIDTH), static_cast<f32>(UI_BASE_RENDER_HEIGHT)}, WHITE, false);
}
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint) {
  IF_NOT_STATE("gui_draw_spritesheet_to_background", return; );

  if (_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_spritesheet_to_background()::Sheet id out of bound");
    return;
  }
  if (state->ss_to_draw_bg.sheet_id != _id) {
    state->ss_to_draw_bg = *ss_get_spritesheet_by_enum(_id);
    set_sprite(__builtin_addressof(state->ss_to_draw_bg), true, false);
  }
  Rectangle dest = Rectangle {0.f, 0.f, static_cast<f32>(UI_BASE_RENDER_WIDTH), static_cast<f32>(UI_BASE_RENDER_HEIGHT)};
  play_sprite_on_site(__builtin_addressof(state->ss_to_draw_bg), _tint, dest);
}
/**
 * @note  function, returns "Rectangle {}" if texture type returns null pointer
 * @return Rectangle { .x = 0, .y = 0, .width = tex->width, .height = tex->height}; 
 */
const Rectangle * get_atlas_texture_source_rect(atlas_texture_id _id) {
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::get_atlas_texture_source_rect()::Texture resource is invalid");
    return nullptr; 
  }
  return __builtin_addressof(tex->source);
}
const std::array<button_type, BTN_TYPE_MAX>* get_button_types(void) {
  IF_NOT_STATE("get_button_types", return nullptr; );

  return __builtin_addressof(state->button_types);
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
void draw_atlas_texture_regular(atlas_texture_id _id, Rectangle dest, Color tint, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::draw_texture_regular()::Texture id is out of bound"); 
    return; 
  }
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::draw_texture_regular()::Texture resource is invalid");
    return; 
  }
  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }
  DrawTexturePro( (*tex->atlas_handle), tex->source, dest, ZEROVEC2, 0.f, tint);
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
void draw_atlas_texture_npatch(atlas_texture_id _id, Rectangle dest, Vector4 offsets, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::draw_atlas_texture_npatch()::Texture id is out of bound"); 
    return; 
  }
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::draw_atlas_texture_npatch()::Texture resource is invalid"); 
    return; 
  }
  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }
  NPatchInfo npatch = { tex->source, (i32) offsets.x, (i32)offsets.y, (i32)offsets.z, (i32)offsets.w, NPATCH_NINE_PATCH };

  DrawTextureNPatch( (*tex->atlas_handle), npatch, dest, ZEROVEC2, 0.f, WHITE);
}
/**
 * @brief grid_pos, grid_coord, grid_dim 
 */
Vector2 position_element_by_grid(Vector2 grid_location, Vector2 grid, Vector2 grid_dim) {
  return Vector2{
      .x = grid_location.x + (grid.x * grid_dim.x) ,
      .y = grid_location.y + (grid.y * grid_dim.y) 
  };
}

void draw_text_shader(const char *text, shader_id sdr_id, Vector2 position, Font font, float fontsize, Color tint, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord) {
  if (not text or text == nullptr or not font.glyphs or font.glyphs == nullptr or font.baseSize == 0) {
    IWARN("user_interface::draw_text_shader()::Text or Font is invalid");
    return;
  }
  Vector2 text_measure = MeasureTextEx(font, text, fontsize, UI_FONT_SPACING);
  Vector2 text_position = Vector2 { position.x, position.y };

  if (use_grid_align) {
    text_position = position_element_by_grid(text_position, grid_coord, SCREEN_OFFSET);
  }
  if (center_horizontal) {
    text_position.x -= (text_measure.x * .5f);
  }
  if (center_vertical) {
    text_position.y -= (text_measure.y * .5f);
  }
  i32 length = TextLength(text);

  f32 textOffsetX = 0.f;
  f32 scaleFactor = fontsize/static_cast<f32>(font.baseSize);

  for (i32 itr_000 = 0; itr_000 < length; itr_000++)
  {
    i32 codepointByteCount = 0;
    i32 codepoint = GetCodepoint(__builtin_addressof(text[itr_000]), __builtin_addressof(codepointByteCount));
    i32 index = GetGlyphIndex(font, codepoint);
    f32 glyphWidth = 0;

    glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width * scaleFactor : font.glyphs[index].advanceX * scaleFactor;
    if (itr_000 + 1 < length) glyphWidth = glyphWidth + UI_FONT_SPACING;
    
    if ((codepoint != ' ') and (codepoint != '\t')) {
      //f32 letter_width  = (font.recs[index].width + 2.0f*font.glyphPadding) * scaleFactor;
      //f32 letter_height = (font.recs[index].height + 2.0f*font.glyphPadding) * scaleFactor;

      switch (sdr_id) {
        case SHADER_ID_FONT_OUTLINE: {
          BeginShaderMode(get_shader_by_enum(SHADER_ID_FONT_OUTLINE)->handle);
            //set_shader_uniform(SHADER_ID_FONT_OUTLINE, "texture_size", data128(letter_width, letter_height));
          DrawTextCodepoint(font, codepoint, Vector2{ text_position.x + textOffsetX, text_position.y }, fontsize, tint);
          EndShaderMode();
          break;
        }
        case SHADER_ID_SDF_TEXT: {
          BeginShaderMode(get_shader_by_enum(SHADER_ID_SDF_TEXT)->handle);

          DrawTextCodepoint(font, codepoint, Vector2{ text_position.x + textOffsetX, text_position.y }, fontsize, tint);
          EndShaderMode();
          break;
        }
        case SHADER_ID_CHEST_OPENING_SPIN_TEXT: {
          BeginShaderMode(get_shader_by_enum(SHADER_ID_CHEST_OPENING_SPIN_TEXT)->handle);

          DrawTextCodepoint(font, codepoint, Vector2{ text_position.x + textOffsetX, text_position.y }, fontsize, tint);
          EndShaderMode();
          break;
        }
        default: { break; }
      }
    }
    if ((textOffsetX != 0) or (codepoint != ' ')) textOffsetX += glyphWidth;
  }
}

/**
 * @brief TODO: This function's purpose is replacing the font when unsupported characters 
 */
void draw_text_ex(const char *text, Vector2 position, Font font, f32 _fontsize, Color tint, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord) {
  f32 font_size = _fontsize * font.baseSize;
  Vector2 text_measure = MeasureTextEx(font, text, font_size, UI_FONT_SPACING);
  Vector2 text_position = Vector2 { position.x, position.y };
  if (use_grid_align) {
    text_position = position_element_by_grid(text_position, grid_coord, SCREEN_OFFSET);
  }
  if (center_horizontal) {
    text_position.x -= (text_measure.x * .5f);
  }
  if (center_vertical) {
    text_position.y -= (text_measure.y * .5f);
  }
  int length = TextLength(text);

  float textOffsetX = 0.0f;
  float scaleFactor = font_size/(float)font.baseSize;

  BeginShaderMode(get_shader_by_enum(SHADER_ID_SDF_TEXT)->handle);
  for (i32 itr_000 = 0; itr_000 < length; itr_000++)
  {
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[itr_000], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);
    float glyphWidth = 0;

    glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;
    if (itr_000 + 1 < length) glyphWidth = glyphWidth + UI_FONT_SPACING;
    
    if ((codepoint != ' ') && (codepoint != '\t')) {
      DrawTextCodepoint(font, codepoint, Vector2{ text_position.x + textOffsetX, text_position.y }, font_size, tint);
    }
    if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;
  }
  EndShaderMode();
} 
/**
 * @brief NOTE: Source https://github.com/raysan5/raylib/blob/master/examples/text/text_rectangle_bounds.c
 */
void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint) {
  int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop
  float textOffsetY = 0;          // Offset between lines (on line break '\n')
  float textOffsetX = 0.0f;       // Offset X to next character to draw
  float scaleFactor = fontSize/(float)font.baseSize;     // Character rectangle scaling factor
  // Word/character wrapping mechanism variables
  enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
  int state = wordWrap? MEASURE_STATE : DRAW_STATE;
  int startLine = -1;         // Index where to begin drawing (where a line begins)
  int endLine = -1;           // Index where to stop drawing (where a line ends)
  int lastk = -1;             // Holds last value of the character position

  BeginShaderMode(get_shader_by_enum(SHADER_ID_SDF_TEXT)->handle);
  for (int i = 0, k = 0; i < length; i++, k++)
  {
    // Get next codepoint from byte string and glyph index in font
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[i], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);
    // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
    // but we need to draw all of the bad bytes using the '?' symbol moving one byte
    if (codepoint == 0x3f) codepointByteCount = 1;
    i += (codepointByteCount - 1);
    float glyphWidth = 0;
    if (codepoint != '\n')
    {
        glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;
        if (i + 1 < length) glyphWidth = glyphWidth + spacing;
    }
    // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
    // We store this info in startLine and endLine, then we change states, draw the text between those two variables
    // and change states again and again recursively until the end of the text (or until we get outside of the container).
    // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
    // and begin drawing on the next line before we can get outside the container.
    if (state == MEASURE_STATE)
    {
      // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
      // Ref: http://jkorpela.fi/chars/spaces.html
      if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;
        if ((textOffsetX + glyphWidth) > rec.width)
        {
          endLine = (endLine < 1)? i : endLine;
          if (i == endLine) endLine -= codepointByteCount;
          if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);
          state = !state;
        }
        else if ((i + 1) == length){
          endLine = i;
          state = !state;
        }
        else if (codepoint == '\n') state = !state;
        if (state == DRAW_STATE)
        {
          textOffsetX = 0;
          i = startLine;
          glyphWidth = 0;
          // Save character position when we switch states
          int tmp = lastk;
          lastk = k - 1;
          k = tmp;
        }
    } else {
      if (codepoint == '\n')
      {
        if (!wordWrap)
        {
          textOffsetY += (font.baseSize + font.baseSize/2.f)*scaleFactor;
          textOffsetX = 0;
        }
      }else {
        if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width)){
          textOffsetY += (font.baseSize + font.baseSize/2.f)*scaleFactor;
          textOffsetX = 0;
        }
          // When text overflows rectangle height limit, just stop drawing
          if ((textOffsetY + font.baseSize*scaleFactor) > rec.height) break;
          // Draw current character glyph
          if ((codepoint != ' ') && (codepoint != '\t'))
          {
            
            DrawTextCodepoint(font, codepoint, Vector2{ rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, tint);
          }
      }
        if (wordWrap && (i == endLine))
        {
          textOffsetY += (font.baseSize + font.baseSize/2.f)*scaleFactor;
          textOffsetX = 0;
          startLine = endLine;
          endLine = -1;
          glyphWidth = 0;
          k = lastk;
          state = !state;
        }
      }
    if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;  // avoid leading spaces
  }
  EndShaderMode();
}
void process_fade_effect(ui_fade_control_system *const fade) {
  IF_NOT_STATE("process_fade_effect", return; );

  if (not fade->fade_animation_playing or fade->is_fade_animation_played) {
    return;
  }
  switch (fade->fade_type) {
    case FADE_TYPE_FADEIN: {
      f32 process = EaseQuadIn(fade->fade_animation_timer, 0.f, 1.f, fade->fade_animation_duration);
      event_fire(EVENT_CODE_SET_POST_PROCESS_FADE_VALUE, event_context(static_cast<f32>(process)));
      break;
    }
    case FADE_TYPE_FADEOUT: {
      f32 process = EaseQuadOut(fade->fade_animation_timer, 1.f, -1.f, fade->fade_animation_duration);
      event_fire(EVENT_CODE_SET_POST_PROCESS_FADE_VALUE, event_context(static_cast<f32>(process)));
      break;
    }
    default: {
      return;
    }
  }
  if (fade->fade_animation_timer >= fade->fade_animation_duration) {
    fade->is_fade_animation_played = true; 
    fade->fade_animation_playing = false;
  }
  else fade->fade_animation_timer++;
}
/**
 * @brief relative if you want to draw from another atlas.
 */
void gui_draw_atlas_texture_id_pro(atlas_texture_id _id, Rectangle src, Rectangle dest, bool should_center, i32 texture_wrap, Color tint) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_atlas_texture_id_pro()::Texture id is out of bound"); 
    return; 
  }
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::gui_draw_atlas_texture_id_pro()::Texture resource is invalid");
    return; 
  }
  Rectangle source = tex->source;
  source.x += src.x;
  source.y += src.y;
  source.width  = src.width;
  source.height = src.height;
  if (should_center) {
    dest.x -= dest.width / 2.f;
    dest.y -= dest.height / 2.f;
  }
  SetTextureWrap( (*tex->atlas_handle), texture_wrap);
  DrawTexturePro( (*tex->atlas_handle), source, dest, ZEROVEC2, 0.f, tint);
  if (texture_wrap != TEXTURE_WRAP_REPEAT) {
    SetTextureWrap( (*tex->atlas_handle), TEXTURE_WRAP_REPEAT); // Repeat is default
  }
}
void gui_draw_atlas_texture_id(atlas_texture_id _id, Rectangle dest, Vector2 origin, f32 rotation, Color tint) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_atlas_texture_id()::Texture id is out of bound"); 
    return; 
  }
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::gui_draw_atlas_texture_id()::Texture resource is invalid");
    return; 
  }
  DrawTexturePro( (*tex->atlas_handle), tex->source, dest, origin, rotation, tint);
}
void gui_draw_atlas_texture_id_pro_grid(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_atlas_texture_id_pro_grid()::Texture id is out of bound"); 
    return; 
  }
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::gui_draw_atlas_texture_id_pro_grid()::Texture resource is invalid");
    return; 
  }
  if (relative) {
    src.x += tex->source.x;
    src.y += tex->source.y;
  }
  Vector2 pos = position_element_by_grid(UI_BASE_RENDER_DIV2, VECTOR2(dest.x, dest.y), SCREEN_OFFSET);
  dest.x = pos.x;
  dest.y = pos.y;
  DrawTexturePro( (*tex->atlas_handle), src, dest, Vector2 { tex->source.width * .5f, tex->source.height *.5f }, 0.f, WHITE);
}
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame) {
  if (_id >= SHEET_ID_SPRITESHEET_TYPE_MAX or _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_spritesheet_id()::Texture id is out of bound"); 
    return; 
  }
  draw_sprite_on_site_by_id(_id, _tint, pos, scale, frame);
}
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest, Color tint, Vector2 origin, i32 texture_wrap) {
  if (_id >= TEX_ID_MAX or _id <= TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_texture_id_pro()::Texture id is out of bound"); 
    return; 
  }
  const Texture2D *const tex = ss_get_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::gui_draw_texture_id_pro()::Texture resource is invalid");
    return; 
  }
  SetTextureWrap( (*tex), texture_wrap);
  DrawTexturePro( (*tex), src, dest, origin, 0.f, tint);
}
void gui_draw_texture_id(const texture_id _id, const Rectangle dest, const Vector2 origin, i32 texture_wrap) {
  if (_id >= TEX_ID_MAX or _id <= TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_texture_id()::Texture id is out of bound"); 
    return; 
  }
  const Texture2D *const tex = ss_get_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IERROR("user_interface::gui_draw_texture_id()::Texture resource is invalid");
    return; 
  }
  SetTextureWrap( (*tex), texture_wrap);
  DrawTexturePro( (*tex), Rectangle{0.f, 0.f, (f32) tex->width, (f32) tex->height}, dest, origin, 0.f, WHITE);
}
void gui_draw_atlas_texture_id_scale(atlas_texture_id _id, Vector2 position, f32 scale, Color tint, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX or _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    IWARN("user_interface::gui_draw_atlas_texture_id_scale()::Texture id is out of bound"); 
    return; 
  }
  const atlas_texture *const tex = ss_get_atlas_texture_by_enum(_id);
  if (not tex or tex == nullptr) { 
    IWARN("user_interface::gui_draw_atlas_texture_id_scale()::Texture resource is invalid");
    return; 
  }
  if (should_center) {
    position.x -= tex->source.width / 2.f; 
    position.y -= tex->source.height / 2.f; 
  }
  DrawTexturePro( (*tex->atlas_handle), tex->source, Rectangle {position.x, position.y, tex->source.width * scale, tex->source.height * scale}, ZEROVEC2, 0.f, tint);
}
Font ui_get_font(font_type font) {
  IF_NOT_STATE("ui_get_font", return GetFontDefault(); );

  if (not state->display_language or state->display_language == nullptr) {
    return GetFontDefault();
  }
  switch (font) {
  case FONT_TYPE_ITALIC: return UI_ITALIC_FONT;
  case FONT_TYPE_LIGHT:  return UI_LIGHT_FONT;
  case FONT_TYPE_REGULAR:return UI_REGULAR_FONT;
  case FONT_TYPE_BOLD:   return UI_BOLD_FONT;
  case FONT_TYPE_TITLE:  return UI_TITLE_FONT;
  default: IWARN("user_interface::ui_get_font()::Unsupported font type");
  }
  return GetFontDefault();
}
const Vector2* ui_get_mouse_pos_screen(void) {
  IF_NOT_STATE("ui_get_mouse_pos_screen", return nullptr; );

  return __builtin_addressof(state->mouse_pos_screen);
}
slider* get_slider_by_id(slider_id sdr_id) {
  IF_NOT_STATE("get_slider_by_id", return nullptr; );

  if (sdr_id <= SDR_ID_UNDEFINED or sdr_id >= SDR_ID_MAX) {
    IWARN("user_interface::get_slider_by_id()::Slider id is invalid");
    return nullptr;
  }
  return __builtin_addressof(state->sliders.at(sdr_id));
}
checkbox* get_checkbox_by_id(checkbox_id cb_id) {
  IF_NOT_STATE("get_checkbox_by_id", return nullptr; );

  if (cb_id <= CHECKBOX_ID_UNDEFINED or cb_id >= CHECKBOX_ID_MAX) {
    IWARN("user_interface::get_checkbox_by_id()::Checkbox id is out of bound");
    return nullptr;
  }
  return __builtin_addressof(state->checkboxes.at(cb_id));
}
Vector2 ui_measure_text(const char* in_str, font_type in_font_type, f32 in_font_size) {
  IF_NOT_STATE("ui_measure_text", return ZEROVEC2; );

  if (not in_str or in_str == nullptr) {
    return ZEROVEC2;
  }
  Font _font = ui_get_font(in_font_type);
  
  return MeasureTextEx(_font, in_str, in_font_size * _font.baseSize, UI_FONT_SPACING);
}
bool ui_set_slider_current_index(slider_id id, i32 index) {
  IF_NOT_STATE("ui_set_slider_current_index", return false; );

  if (id >= SDR_ID_MAX or id <= SDR_ID_UNDEFINED) {
    IWARN("user_interface::ui_set_slider_current_index()::Slider id is out of bound"); 
    return false;
  }
  state->sliders.at(id).current_value = index;
  return true;
}
bool ui_set_slider_current_value(slider_id id, slider_option value) {
  IF_NOT_STATE("ui_set_slider_current_value", return false; );

  if (id >= SDR_ID_MAX or id <= SDR_ID_UNDEFINED) {
    IWARN("user_interface::ui_set_slider_current_value()::Slider id is out of bound"); 
    return false;
  }
  slider& sdr = state->sliders.at(id);
  size_t last_elem = static_cast<size_t>(sdr.current_value);
  sdr.options.at(last_elem) = value;
  return true;
}
bool ui_sound_slider_on_left_button_trigger(void) {
  IF_NOT_STATE("ui_sound_slider_on_left_button_trigger", return false; );

  SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value--;
  if (SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value < 0) {
    SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value = 0;
  }
  set_master_sound(SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value, true);
  return true;
}
bool ui_sound_slider_on_right_button_trigger(void) {  
  IF_NOT_STATE("ui_sound_slider_on_right_button_trigger", return false; );

  SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value++;
  if (SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value > 10) {
    SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value = 10;
  }
  set_master_sound(SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value, true);
  return true;
}
data_pack* get_slider_current_value(slider_id id) {
  IF_NOT_STATE("get_slider_current_value", return nullptr; );

  if (id >= SDR_ID_MAX or id <= SDR_ID_UNDEFINED) {
    IWARN("user_interface::get_slider_current_value()::Slider id is out of bound"); 
    return nullptr;
  }
  return __builtin_addressof(state->sliders.at(id).options.at(state->sliders.at(id).current_value).content);
}
Font load_font(pak_file_id pak_id, i32 asset_id, i32 font_size, [[__maybe_unused__]] i32* _codepoints,  [[__maybe_unused__]] i32 _codepoint_count) {
  IF_NOT_STATE("load_font", return GetFontDefault(); );

  Font font = ZERO_FONT;
  font.baseSize = font_size;
 
  const file_buffer *const file = get_asset_file_buffer(pak_id, asset_id);
  if (not file or file == nullptr) {
    IWARN("user_interface::load_font()::Font cannot loading, returning default");
    return GetFontDefault();
  }
  else {
    //font = LoadFontFromMemory(file->file_extension.c_str(), reinterpret_cast<const u8 *>(file->content.c_str()), static_cast<i32>(file->content.size()), font_size, 0, 0); 
    font.glyphs = LoadFontData(reinterpret_cast<const u8 *>(file->content.c_str()), static_cast<i32>(file->content.size()), font_size, 0, 0, FONT_SDF, __builtin_addressof(font.glyphCount));
    Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, 0, font_size, 0, 1);
    font.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
  }
  return font;
}
localization_package* load_localization(std::string language_name, i32 lang_index, std::string _codepoints, i32 font_size) {
  IF_NOT_STATE("load_localization", return nullptr; );

  localization_package loc_pack = localization_package();

  i32 codepoint_count = 1;
  i32* codepoints = LoadCodepoints(_codepoints.c_str(), __builtin_addressof(codepoint_count));
  if (not codepoints or codepoints == nullptr) {
    IERROR("user_interface::load_localization()::Failed to load codepoints");
    return nullptr;
  }
  loc_pack.index = static_cast<language_index>(lang_index);
  loc_pack.language_name = language_name;
  loc_pack.codepoints = codepoints;
  loc_pack.italic_font = load_font(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_MIOSEVKA_ITALIC, font_size, codepoints, codepoint_count);
  loc_pack.light_font  = load_font(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_MIOSEVKA_LIGHT, font_size, codepoints, codepoint_count);
  loc_pack.regular_font  = load_font(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_MIOSEVKA_REGULAR, font_size, codepoints, codepoint_count);
  loc_pack.bold_font  = load_font(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_MIOSEVKA_BOLD, font_size, codepoints, codepoint_count);
  loc_pack.mood = load_font(PAK_FILE_ASSET1, PAK_FILE_ASSET1_FONT_MOOD, 28, codepoints, codepoint_count);
  UnloadCodepoints(codepoints);

  SetTextureFilter(loc_pack.italic_font.texture, TEXTURE_FILTER_ANISOTROPIC_16X);
  SetTextureFilter(loc_pack.light_font .texture, TEXTURE_FILTER_ANISOTROPIC_16X);
  SetTextureFilter(loc_pack.regular_font .texture, TEXTURE_FILTER_ANISOTROPIC_16X);
  SetTextureFilter(loc_pack.bold_font .texture, TEXTURE_FILTER_ANISOTROPIC_16X);
  SetTextureFilter(loc_pack.mood.texture, TEXTURE_FILTER_ANISOTROPIC_16X);

  state->localization_info.at(lang_index) = loc_pack;

  return __builtin_addressof(state->localization_info.at(lang_index));
}
localization_package* ui_get_localization_by_name(const char * language_name) {
  IF_NOT_STATE("ui_get_localization_by_name", return nullptr; );

  language_index index = loc_parser_lang_name_to_index(language_name);
  for (size_t iter = 0u; iter < state->localization_info.size(); iter++) {
    if (state->localization_info.at(iter).index == index) {
      return __builtin_addressof(state->localization_info.at(iter));
    }
  }
  return nullptr;
}
localization_package * ui_get_localization_by_index(language_index index) {
  IF_NOT_STATE("ui_get_localization_by_index", return nullptr; );

  if (index <= LANGUAGE_INDEX_UNDEFINED or index >= LANGUAGE_INDEX_MAX) {
    IERROR("user_interface::ui_get_localization_by_index()::Index is out of bound");
    return __builtin_addressof(state->localization_info.at(LANGUAGE_INDEX_BUILTIN));
  }

  return __builtin_addressof(state->localization_info.at(index));
}
Vector2 ui_align_text(Rectangle in_dest, Vector2 in_text_measure, text_alignment align_to) {
    switch (align_to) {
    case TEXT_ALIGN_TOP_LEFT: return Vector2 { in_dest.x, in_dest.y};
    
    case TEXT_ALIGN_TOP_CENTER: return Vector2 {
      in_dest.x + (in_dest.width * .5f) - (in_text_measure.x * .5f),
      in_dest.y
    };
    case TEXT_ALIGN_TOP_RIGHT: return Vector2 {
      in_dest.x + in_dest.width - in_text_measure.x,
      in_dest.y
    };
    case TEXT_ALIGN_BOTTOM_LEFT: return Vector2 {
      in_dest.x,
      in_dest.y + in_dest.height - in_text_measure.y
    };
    case TEXT_ALIGN_BOTTOM_CENTER: return Vector2 {
      in_dest.x + (in_dest.width * .5f)  - (in_text_measure.x * .5f),
      in_dest.y + in_dest.height         - in_text_measure.y
    };
    case TEXT_ALIGN_BOTTOM_RIGHT: return Vector2 {
      in_dest.x + in_dest.width  - in_text_measure.x,
      in_dest.y + in_dest.height - in_text_measure.y
    };
    case TEXT_ALIGN_LEFT_CENTER: return Vector2 {
      in_dest.x,
      in_dest.y + (in_dest.height * .5f) - (in_text_measure.y * .5f)
    };
    case TEXT_ALIGN_RIGHT_CENTER: return Vector2 {
      in_dest.x + in_dest.width - in_text_measure.x,
      in_dest.y + (in_dest.height * .5f) - (in_text_measure.y * .5f)
    };
    
    default: return Vector2 {in_dest.x,in_dest.y};
  }
}
// EXPOSED
void ui_play_sprite_on_site(spritesheet *sheet, Color _tint, Rectangle dest) {
  play_sprite_on_site(sheet, _tint, dest);
}
void ui_draw_sprite_on_site(spritesheet *const sheet, Color _tint, i32 frame) {
  draw_sprite_on_site(sheet, _tint, frame);
}
void ui_draw_sprite_on_site_by_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, i32 frame) {
  draw_sprite_on_site_by_id(_id, _tint, pos, scale, frame);
}
void ui_set_sprite(spritesheet *sheet, bool _play_looped, bool _play_once) {
  set_sprite(sheet, _play_looped, _play_once);
}
const spritesheet * ui_get_spritesheet_by_id(spritesheet_id type) {
  return ss_get_spritesheet_by_enum(type);
}
void ui_update_sprite(spritesheet *sheet, f32 delta_time) {
  update_sprite(sheet, delta_time);
}
// EXPOSED

void user_interface_system_destroy(void) {}

bool user_interface_on_event(i32 code, event_context context) {
  switch (code) {
    case EVENT_CODE_UI_UPDATE_PROGRESS_BAR: {
      state->prg_bars.at((i32)context.data.f32[0]).progress = context.data.f32[1];
      return true;
    }
  };

  return false;
}

#undef BUTTON_TEXT_UP_COLOR
#undef BUTTON_TEXT_HOVER_COLOR
#undef BUTTON_TEXT_PRESSED_COLOR
#undef TEXT_SHADOW_COLOR
#undef TEXT_SHADOW_SIZE_MULTIPLY
#undef MAX_UI_WORDWRAP_WORD_LENGTH
#undef MAX_UI_WORDWRAP_SENTENCE_LENGTH
#undef UI_LIGHT_FONT
#undef UI_LIGHT_FONT_SIZE
#undef UI_MEDIUM_FONT
#undef UI_MEDIUM_FONT_SIZE
#undef UI_BOLD_FONT
#undef UI_BOLD_FONT_SIZE
#undef UI_ITALIC_FONT
#undef UI_ITALIC_FONT_SIZE
#undef UI_ABRACADABRA_FONT
#undef UI_ABRACADABRA_FONT_SIZE
#undef MENU_BUTTON_FONT
#undef MENU_BUTTON_FONT_SIZE_SCALE
#undef MINI_BUTTON_FONT
#undef MINI_BUTTON_FONT_SIZE_SCALE
#undef LABEL_FONT
#undef LABEL_FONT_SIZE_SCALE
#undef DEFAULT_MENU_BUTTON_SCALE
#undef SLIDER_FONT
#undef DEFAULT_SLIDER_FONT_SIZE
#undef DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH
#undef SDR_AT
#undef SDR_CURR_OPT_VAL
#undef SDR_ASSERT_SET_CURR_VAL
#undef SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED
#undef UI_BASE_RENDER_SCALE
#undef UI_BASE_RENDER_DIV2
#undef UI_BASE_RENDER_RES_VEC
#undef UI_BASE_RENDER_WIDTH
#undef UI_BASE_RENDER_HEIGHT 
#undef COMBAT_FEEDBACK_FLOATING_TEXT_MIN_DURATION
#undef COMBAT_FEEDBACK_FLOATING_TEXT_MAX_DURATION
#undef COMBAT_FEEDBACK_FLOATING_TEXT_MIN_SCALE
#undef COMBAT_FEEDBACK_FLOATING_TEXT_MAX_SCALE
