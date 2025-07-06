#include "user_interface.h"
#include <reasings.h>
#include <settings.h>
#include "loc_types.h"

#include <tools/pak_parser.h>

#include "core/event.h"
#include "core/fmemory.h"

#include "game_types.h"
#include "game/spritesheet.h"
#include "game/fshader.h"

#include "raylib.h"

typedef struct user_interface_system_state {
  const app_settings * in_app_settings;
  localization_package * display_language;
  
  std::array<button, BTN_ID_MAX> buttons;
  std::array<button_type, BTN_TYPE_MAX> button_types;
  std::array<slider, SDR_ID_MAX> sliders;
  std::array<slider_type, SDR_TYPE_MAX> slider_types;
  std::array<progress_bar, PRG_BAR_ID_MAX> prg_bars;
  std::array<progress_bar_type, PRG_BAR_TYPE_ID_MAX> prg_bar_types;
  
  spritesheet ss_to_draw_bg;
  
  Vector2 mouse_pos;
  std::vector<localization_package> localization_info;

  u16 fade_animation_duration;
  f32 fade_animation_timer;
  bool fadein;
  bool fade_animation_playing;

  user_interface_system_state(void) {
    this->in_app_settings = nullptr;
    this->display_language = nullptr;
    this->buttons.fill(button());
    this->button_types.fill(button_type());
    this->sliders.fill(slider());
    this->slider_types.fill(slider_type());
    this->prg_bars.fill(progress_bar());
    this->prg_bar_types.fill(progress_bar_type());
    this->ss_to_draw_bg = spritesheet();
    this->mouse_pos = ZEROVEC2;
    this->localization_info.clear();
    this->fade_animation_duration = 0u;
    this->fade_animation_timer = 0.f;
    this->fadein = false;
    this->fade_animation_playing = false;
  }
} user_interface_system_state;

static user_interface_system_state * state;

#define BUTTON_TEXT_UP_COLOR WHITE_ROCK
#define BUTTON_TEXT_HOVER_COLOR WHITE
#define BUTTON_TEXT_PRESSED_COLOR WHITE
#define TEXT_SHADOW_COLOR BLACK
#define TEXT_SHADOW_SIZE_MULTIPLY 1.1f

#define MAX_UI_WORDWRAP_WORD_LENGTH 20
#define MAX_UI_WORDWRAP_SENTENCE_LENGTH 300

#define UI_LIGHT_FONT state->display_language->light_font
#define UI_LIGHT_FONT_SIZE state->display_language->light_font.baseSize * .1f
#define UI_MEDIUM_FONT state->display_language->medium_font
#define UI_MEDIUM_FONT_SIZE state->display_language->medium_font.baseSize * .1f
#define UI_BOLD_FONT state->display_language->bold_font
#define UI_BOLD_FONT_SIZE state->display_language->bold_font.baseSize * .1f
#define UI_ITALIC_FONT state->display_language->italic_font
#define UI_ITALIC_FONT_SIZE state->display_language->italic_font.baseSize * .1f

#define MENU_BUTTON_FONT UI_MEDIUM_FONT
#define MENU_BUTTON_FONT_SIZE UI_MEDIUM_FONT_SIZE * .3f
#define MINI_BUTTON_FONT UI_LIGHT_FONT
#define MINI_BUTTON_FONT_SIZE UI_LIGHT_FONT_SIZE * .45f
#define LABEL_FONT UI_ITALIC_FONT
#define LABEL_FONT_SIZE UI_ITALIC_FONT_SIZE
#define DEFAULT_MENU_BUTTON_SCALE 4
#define DEFAULT_SLIDER_FONT_SIZE 32
#define DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH 10

#define SDR_AT(ID) state->sliders.at(ID)
#define SDR_CURR_OPT_VAL(ID) state->sliders.at(ID).options.at(state->sliders.at(ID).current_value)
#define SDR_ASSERT_SET_CURR_VAL(EXPR, ID, INDEX) {\
  if(EXPR) state->sliders.at(ID).current_value = INDEX;\
}
#define SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(EXPR, ID) {\
  if(EXPR) state->sliders.at(ID).current_value = state->sliders.at(ID).max_value-1;\
}

bool user_interface_on_event(u16 code, event_context context);

void update_buttons(void);
void update_sliders(void);

void draw_fade_effect();
void draw_slider_body(slider* sdr);
void draw_atlas_texture_stretch(atlas_texture_id body, Vector2 pos, Vector2 scale, Rectangle stretch_part, u16 stretch_part_mltp, bool should_center);
void draw_atlas_texture_regular(atlas_texture_id _id, Rectangle dest, Color tint, bool should_center);
void draw_atlas_texture_npatch(atlas_texture_id _id, Rectangle dest, Vector4 offsets, bool should_center);
void gui_draw_settings_screen(void);
bool gui_button(const char* text, button_id _id, Font font, f32 font_size_scale, Vector2 pos, bool play_on_click_sound);

void register_button(button_id _btn_id, button_type_id _btn_type_id);
void register_button_type(button_type_id _btn_type_id, spritesheet_id _ss_type, Vector2 frame_dim, f32 _scale, bool _should_center);
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id, f32 width_multiply, Vector2 scale);
void register_progress_bar_type(progress_bar_type_id _type_id, atlas_texture_id _body_inside, atlas_texture_id _body_outside, shader_id _mask_shader_id);
void register_slider_type(slider_type_id _sdr_type_id, spritesheet_id _ss_sdr_body_type, f32 _scale, u16 _width_multiply, button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, u16 _char_limit);

constexpr void draw_text_shader(const char *text, shader_id sdr_id, Vector2 position, Font font, float fontsize, Color tint, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord);
void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);
const char* wrap_text(const char* text, Font font, i32 font_size, Rectangle bounds, bool center_x);
void set_resolution_slider_native_res(void);
Font load_font(const char* file_name, i32 font_size, i32* _codepoints, i32 _codepoint_count);
localization_package* load_localization(std::string language_name, u32 loc_index, std::string _codepoints, i32 font_size);
localization_package* ui_get_localization_by_name(std::string language_name);
localization_package* ui_get_localization_by_index(u32 _language_index);

Vector2 make_vector(f32 x, f32 y);
bool ui_sound_slider_on_left_button_trigger(void);
bool ui_sound_slider_on_right_button_trigger(void);

constexpr inline void draw_text(const char* text, Vector2 pos, Font font, i32 fontsize, Color color, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord) {
  Vector2 text_measure = MeasureTextEx(font, text, fontsize, UI_FONT_SPACING);
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

  DrawTextEx(font, text, text_position,  fontsize, UI_FONT_SPACING, color);
}
constexpr inline void draw_text_outline(const char* text, 
  Vector2 pos, Font font, i32 fontsize, Color color, f32 outline_thickness, 
  bool center_horizontal, bool center_vertical, 
  bool use_grid_align, Vector2 grid_coord
) {
  Vector2 text_measure = MeasureTextEx(font, text, fontsize, UI_FONT_SPACING);
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

  DrawTextEx(font, text, Vector2 {text_position.x - outline_thickness, text_position.y - outline_thickness}, fontsize, UI_FONT_SPACING, BLACK);
  DrawTextEx(font, text, Vector2 {text_position.x + outline_thickness, text_position.y - outline_thickness}, fontsize, UI_FONT_SPACING, BLACK);
  DrawTextEx(font, text, Vector2 {text_position.x - outline_thickness, text_position.y + outline_thickness}, fontsize, UI_FONT_SPACING, BLACK);
  DrawTextEx(font, text, Vector2 {text_position.x + outline_thickness, text_position.y + outline_thickness}, fontsize, UI_FONT_SPACING, BLACK);

  DrawTextEx(font, text, text_position,  fontsize, UI_FONT_SPACING, color);
}

bool user_interface_system_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "user_interface::user_interface_system_initialize()::Initialize called twice");
    return true;
  }
  state = (user_interface_system_state *)allocate_memory_linear(sizeof(user_interface_system_state), true);
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::State allocation failed");
    return false;
  }
  state->in_app_settings = get_app_settings();

  localized_languages langs = loc_parser_get_loc_langs();
  for (size_t iter = 0; iter < langs.lang.size(); ++iter) {
    loc_data& data = langs.lang.at(iter);
    localization_package* _loc_data = load_localization(data.language_name, data.index, data.codepoints, 34);
    if (_loc_data == nullptr) {
      TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::Loading localization:%s is failed", _loc_data->language_name.c_str());
      continue;
    }
  }
  localization_package* settings_loc_lang = ui_get_localization_by_name(state->in_app_settings->language);
  if (settings_loc_lang == nullptr) {
    TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::Settings language:%s cannot found", settings_loc_lang->language_name.c_str());
    if(!loc_parser_set_active_language_by_index(0)) {
      TraceLog(LOG_FATAL, "user_interface::user_interface_system_initialize()::No language found");
      return false;
    }
  }
  loc_parser_set_active_language_by_index(settings_loc_lang->language_index);
  loc_data * _loc_data = loc_parser_get_active_language();
  state->display_language = ui_get_localization_by_name(_loc_data->language_name);
  if (state->display_language == nullptr) {
    TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::Language:%s cannot found on the UI", state->display_language->language_name.c_str());
    return false;
  }
  if(!initialize_shader_system()) {
    TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::Shader system failed to initialize!");
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
    BTN_TYPE_FLAT_BUTTON, SHEET_ID_FLAT_BUTTON, Vector2{44, 14}, 2, false);
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
    register_progress_bar_type(
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      ATLAS_TEX_ID_PROGRESS_BAR_INSIDE_FULL, ATLAS_TEX_ID_PROGRESS_BAR_OUTSIDE_FULL,
      SHADER_ID_PROGRESS_BAR_MASK
    );
  }
  // PROGRES BAR TYPES

  // PROGRES BARS
  {
    register_progress_bar(
      PRG_BAR_ID_PLAYER_EXPERIANCE,
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      5,
      Vector2 {3,3}
    );
    register_progress_bar(
      PRG_BAR_ID_PLAYER_HEALTH,
      PRG_BAR_TYPE_ID_CRIMSON_FANT_BAR,
      3,
      Vector2 {3,3}
    );
  }
  // PROGRES BARS

  // IN GAME
  {
  register_button(BTN_ID_IN_GAME_BUTTON_RETURN_MENU, BTN_TYPE_MENU_BUTTON);
  }
  // IN GAME

  // MAIN MENU
  {  
    register_button(BTN_ID_MAINMENU_BUTTON_PLAY,        BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_EDITOR,      BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_SETTINGS,    BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_UPGRADE,     BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_BUTTON_EXIT,        BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_SETTINGS_CANCEL,    BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_UPGRADE_BACK,       BTN_TYPE_MENU_BUTTON);
    register_button(BTN_ID_MAINMENU_UPGRADE_BUY_UPGRADE,BTN_TYPE_MENU_BUTTON);
  }
  // MAIN MENU

  // EDITOR
  {
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
    Vector2 window_size = Vector2 {state->in_app_settings->window_size.at(0), state->in_app_settings->window_size.at(1)};

    gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, data_pack(DATA_TYPE_U16, data128( (u16)960, (u16)540 ), 2) , 0, "960x540"  );
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(window_size.x == 960.f && window_size.y == 540.f, SDR_ID_SETTINGS_RES_SLIDER)
    gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, data_pack(DATA_TYPE_U16, data128( (u16)1280, (u16)720 ), 2), 0, "1280x720" );
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(window_size.x == 1280.f && window_size.y == 720.f, SDR_ID_SETTINGS_RES_SLIDER)
    gui_slider_add_option(SDR_ID_SETTINGS_RES_SLIDER, data_pack(DATA_TYPE_U16, data128((u16)1920, (u16)1080), 2) , 0, "1920x1080");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(window_size.x == 1920.f && window_size.y == 1080.f, SDR_ID_SETTINGS_RES_SLIDER)

    localized_languages langs = loc_parser_get_loc_langs();

    for (size_t iter = 0; iter < langs.lang.size(); iter++) {
      gui_slider_add_option(SDR_ID_SETTINGS_LANGUAGE, data_pack(DATA_TYPE_U32, data128(static_cast<u32>(iter), 1u), 1), LOC_TEXT_SETTINGS_BUTTON_ENGLISH+iter, "");
      SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->display_language->language_index == iter, SDR_ID_SETTINGS_LANGUAGE)
    }

    gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)0), 1                            ),  LOC_TEXT_SETTINGS_SDR_WINDOW_MODE_WINDOWED,   "");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->in_app_settings->window_state == 0, SDR_ID_SETTINGS_WIN_MODE_SLIDER)
    gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)FLAG_BORDERLESS_WINDOWED_MODE), 1),  LOC_TEXT_SETTINGS_SDR_WINDOW_MODE_BORDERLESS, "");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->in_app_settings->window_state == FLAG_BORDERLESS_WINDOWED_MODE, SDR_ID_SETTINGS_WIN_MODE_SLIDER)
    gui_slider_add_option(SDR_ID_SETTINGS_WIN_MODE_SLIDER, data_pack(DATA_TYPE_I32, data128((i32)FLAG_FULLSCREEN_MODE), 1         ),  LOC_TEXT_SETTINGS_SDR_WINDOW_MODE_FULLSCREEN, "");
    SDR_ASSERT_SET_CURR_VAL_TO_LAST_ADDED(state->in_app_settings->window_state == FLAG_FULLSCREEN_MODE, SDR_ID_SETTINGS_WIN_MODE_SLIDER)
  }
  // SLIDER OPTIONS

  // SETTING SLIDER SETUP
  { 
    for (size_t iter = 0; iter < state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).max_value; ++iter) {
      if (state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).options.at(iter).content.data.i32[0] == state->in_app_settings->window_state && iter != 0) {
        state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).current_value = iter;
        break;
      }
    }
  
    const std::vector<f32>& window_size = state->in_app_settings->window_size;

    for (size_t iter = 0; iter < state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).max_value; ++iter) {
      slider_option& option = state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).options.at(iter);
      if (option.content.data.u16[0] == window_size.at(0) && option.content.data.u16[1] == window_size.at(1)) {
        state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).current_value = iter;
        break;
      }
    }
  
    if (SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16[0] != window_size.at(0) || SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16[1] != window_size.at(1)) {
      TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::Unsupported resolution: {%d:%d}", window_size.at(0), window_size.at(1));
      set_resolution(1920, 1080); // TODO: Set default resolution based on aspect ratio
      for (size_t iter = 0; iter < state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).max_value; ++iter) {
        slider_option& option = state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).options.at(iter);
        if (option.content.data.u16[0] == window_size.at(0) && option.content.data.u16[1] == window_size.at(1)) {
          state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).current_value = iter;
          break;
        }
      }
    }

    state->sliders.at(SDR_ID_SETTINGS_SOUND_SLIDER).on_left_button_trigger = ui_sound_slider_on_left_button_trigger;
    state->sliders.at(SDR_ID_SETTINGS_SOUND_SLIDER).on_right_button_trigger = ui_sound_slider_on_right_button_trigger;
  }

  event_register(EVENT_CODE_UI_UPDATE_PROGRESS_BAR, user_interface_on_event);
  event_register(EVENT_CODE_UI_START_FADEIN_EFFECT, user_interface_on_event);
  event_register(EVENT_CODE_UI_START_FADEOUT_EFFECT, user_interface_on_event);

  return true;
}

void update_user_interface(void) {
  state->mouse_pos.x = GetMousePosition().x * state->in_app_settings->scale_ratio.at(0);
  state->mouse_pos.y = GetMousePosition().y * state->in_app_settings->scale_ratio.at(1);

  update_buttons();
  update_sliders();
  if (state->fade_animation_playing) {
    if(state->fade_animation_timer == state->fade_animation_duration){
      state->fade_animation_timer = 0;
      state->fade_animation_playing = false;
    }
    else state->fade_animation_timer++; 
  }
}

void update_buttons(void) {
  for (size_t itr_000 = 0; itr_000 < BTN_ID_MAX; ++itr_000) {
    if (state->buttons.at(itr_000).id == BTN_ID_UNDEFINED) {
      continue;
    }
    if (!state->buttons.at(itr_000).on_screen) { continue; }

    button* btn = __builtin_addressof(state->buttons.at(itr_000));
    if (CheckCollisionPointRec(state->mouse_pos, btn->dest)) {
      if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        btn->state = BTN_STATE_PRESSED;
      } else {
        if (btn->state == BTN_STATE_PRESSED) { 
          btn->state = BTN_STATE_RELEASED;
        }
        else if (btn->state != BTN_STATE_HOVER) {
          btn->state = BTN_STATE_HOVER;
        }
      }
    } else {
      if (btn->state != BTN_STATE_UP) { 
        btn->state = BTN_STATE_UP;
      }
    }
    btn->on_screen = false;
  }
}
void update_sliders(void) {
  for (size_t itr_000 = 0; itr_000 < SDR_ID_MAX; ++itr_000) {
    if ((state->sliders.at(itr_000).id <= SDR_ID_UNDEFINED || state->sliders.at(itr_000).id >= SDR_ID_MAX) || !state->sliders.at(itr_000).on_screen) continue;
    
    slider* sdr = __builtin_addressof(state->sliders.at(itr_000));
    if (!sdr && !sdr->is_registered) {
      TraceLog(LOG_WARNING, "user_interface::update_sliders()::Using slider didn't registered");
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
          if (CheckCollisionPointRec(state->mouse_pos, sdr_rect)) {
            f32 relative = state->mouse_pos.x - sdr_rect.x;
            f32 ratio = relative / sdr_rect.width;
            sdr->current_value = ratio * sdr->max_value + 1;
            sdr->current_value = FCLAMP(sdr->current_value, sdr->min_value, sdr->max_value);

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

        if (sdr->is_clickable && sdr->on_click != nullptr && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ) {
          if (CheckCollisionPointRec(state->mouse_pos, sdr_rect)) {
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

        if (sdr->is_clickable && sdr->on_click != nullptr && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) ) {
          if (CheckCollisionPointRec(state->mouse_pos, sdr_rect)) {
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

void render_user_interface(void) {
  if (state->fade_animation_playing) {
    draw_fade_effect();
    return;
  }
}
bool gui_menu_button(const char* text, button_id _id, Vector2 grid, Vector2 grid_location, bool play_on_click_sound) {
  grid_location.x -= state->buttons.at(_id).btn_type.dest_frame_dim.x * .5f;
  grid_location.y -= state->buttons.at(_id).btn_type.dest_frame_dim.y * .5f;
  return gui_button(text, _id,
    MENU_BUTTON_FONT, MENU_BUTTON_FONT_SIZE,
    position_element_by_grid(grid_location, grid, SCREEN_OFFSET),
    play_on_click_sound
  );
}
bool gui_mini_button(const char* text, button_id _id, Vector2 grid, bool play_on_click_sound) {
  return gui_button(text, _id, 
    MINI_BUTTON_FONT, MINI_BUTTON_FONT_SIZE, 
    position_element_by_grid(BASE_RENDER_SCALE(.5f), grid, SCREEN_OFFSET),
    play_on_click_sound
  );
}
bool gui_slider_button(button_id _id, Vector2 pos) {
  return gui_button("", _id, Font {}, 0, pos, true);
}

bool gui_button(const char* text, button_id _id, Font font, f32 font_size_scale, Vector2 pos, bool play_on_click_sound) {
  if (_id >= BTN_ID_MAX || _id <= BTN_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "user_interface::gui_button()::Recieved button type out of bound");
    return false;
  }
  button* _btn = __builtin_addressof(state->buttons.at(_id));

  if (!_btn->is_registered) {
    TraceLog(LOG_WARNING, "user_interface::gui_button()::The button is not registered");
    return false;
  }
  _btn->on_screen = true;
  _btn->dest.x = pos.x;
  _btn->dest.y = pos.y;

  Vector2 text_measure;
  Vector2 text_pos;

  if (!TextIsEqual(text, "")) {
    text_measure = MeasureTextEx(font, text, font.baseSize * font_size_scale, UI_FONT_SPACING);
    text_pos = Vector2 {
      .x = _btn->dest.x + (_btn->dest.width / 2.f)  - (text_measure.x / 2.f),
      .y = _btn->dest.y + (_btn->dest.height / 2.f) - (text_measure.y / 2.f)
    };
  }

  Vector2 draw_sprite_scale = Vector2 {_btn->btn_type.scale,_btn->btn_type.scale};

  if (_btn->state == BTN_STATE_PRESSED) {
    draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 0);
    if (!TextIsEqual(text, "")) {
      draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_PRESSED_COLOR, false, false, false, VECTOR2(0.f, 0.f));
    }
    if (play_on_click_sound) event_fire(EVENT_CODE_PLAY_BUTTON_ON_CLICK, event_context((u16)true));
  } else {
    if (_btn->state == BTN_STATE_HOVER) {
      draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 1);
      if (!TextIsEqual(text, "")) {
        draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_HOVER_COLOR, false, false, false, VECTOR2(0.f, 0.f));
      }
      event_fire(EVENT_CODE_RESET_SOUND, event_context((i32)SOUND_ID_BUTTON_ON_CLICK));
    }
    else {
      draw_sprite_on_site_by_id(_btn->btn_type.ss_type, WHITE, VECTOR2(_btn->dest.x,_btn->dest.y), draw_sprite_scale, 0);
      if (_btn->state != BTN_STATE_HOVER) {
        draw_text(text, text_pos, font, font.baseSize * font_size_scale, BUTTON_TEXT_UP_COLOR, false, false, false, VECTOR2(0.f, 0.f));
      }
    }
  }
  return _btn->state == BTN_STATE_RELEASED;
}

void gui_progress_bar(progress_bar_id bar_id, Vector2 pos, bool _should_center) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::gui_player_experiance_process()::ui system didn't initialized");
    return;
  }
  progress_bar prg_bar = state->prg_bars.at(bar_id);

  if (!prg_bar.is_initialized) {
    TraceLog(LOG_ERROR, "user_interface::gui_player_experiance_process()::Player experiance process bar didn't initialized");
    return;
  }
  atlas_texture* inside_tex = ss_get_atlas_texture_by_enum(prg_bar.type.body_inside);
  Texture2D* atlas = ss_get_texture_by_enum(ATLAS_TEXTURE_ID);
  if (!inside_tex) {
    TraceLog(LOG_ERROR, "user_interface::gui_player_experiance_process()::progress bar atlas is null");
    return;
  }
  f32 start_uv = inside_tex->source.x / atlas->width;
  f32 end_uv = (inside_tex->source.x + inside_tex->source.width) / atlas->width;

  f32 process = start_uv + (end_uv - start_uv) * prg_bar.progress;

  draw_atlas_texture_stretch(
    prg_bar.type.body_outside, 
    pos, 
    prg_bar.scale, 
    Rectangle {.x = 27, .y = 0, .width = 10, .height = 9},
    prg_bar.width_multiply,
    _should_center
  );

  BeginShaderMode(get_shader_by_enum(prg_bar.type.mask_shader_id)->handle);
  set_shader_uniform(prg_bar.type.mask_shader_id, 0, data128(process));
  draw_atlas_texture_stretch(
    prg_bar.type.body_inside, 
    pos,
    prg_bar.scale, 
    Rectangle {.x = 27, .y = 0, .width = 10, .height = 9},
    prg_bar.width_multiply,
    _should_center
  );
  EndShaderMode();
}

void draw_atlas_texture_stretch(atlas_texture_id body, Vector2 pos, Vector2 scale, Rectangle stretch_part, u16 stretch_part_mltp, bool should_center) {
  if (body >= ATLAS_TEX_ID_MAX || body <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "user_interface::draw_repetitive_body_tex()::Recieved texture out of bound");
    return;
  }
  atlas_texture* body_tex = ss_get_atlas_texture_by_enum(body);
  if (!body_tex) {
    TraceLog(LOG_ERROR, "user_interface::draw_repetitive_body_tex()::Recieved texture returned NULL");
    return;
  }

  Rectangle first_source = Rectangle{
    .x = 0, .y = 0,
    .width = stretch_part.x, .height = stretch_part.height,
  };
  Rectangle first_dest = Rectangle{
    .x = pos.x, .y = pos.y,
    .width = first_source.width * scale.x, .height = first_source.height * scale.y,
  };
  Rectangle second_dest = Rectangle{
    .x = pos.x + first_dest.width, .y = pos.y,
    .width = stretch_part.width * scale.x * stretch_part_mltp, .height = stretch_part.height * scale.y,
  };
  Rectangle third_source = Rectangle{
    .x = stretch_part.x + stretch_part.width, .y = 0,
    .width = body_tex->source.width - (stretch_part.x + stretch_part.width), .height = stretch_part.height,
  };
  Rectangle third_dest = Rectangle{
    .x = pos.x + first_dest.width + second_dest.width, .y = pos.y,
    .width = third_source.width * scale.x, .height = third_source.height * scale.y,
  };
  if (should_center) {
    first_dest.x  -= first_dest.width + (second_dest.width / 2.f);
    second_dest.x -= first_dest.width + (second_dest.width / 2.f);
    third_dest.x  -= first_dest.width + (second_dest.width / 2.f);
  }
  gui_draw_atlas_texture_id_pro(body, first_source, first_dest, true, false);
  gui_draw_atlas_texture_id_pro(body, stretch_part, second_dest, true, false);
  gui_draw_atlas_texture_id_pro(body, third_source, third_dest, true, false);
}

void gui_slider(slider_id _id, Vector2 pos, Vector2 grid) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider()::One of recieved ids was out of bound");
    return;
  }
  slider* sdr = __builtin_addressof(state->sliders.at(_id));
  if (!sdr) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider()::Slider %d returned NULL", _id);
    return;
  }
  slider_type* sdr_type = __builtin_addressof(sdr->sdr_type);

  pos.x -= sdr_type->dest_frame_dim.x * .5f;
  pos.y -= sdr_type->dest_frame_dim.y * .5f;

  sdr->position = position_element_by_grid(pos, grid, SCREEN_OFFSET);
  if (!sdr->is_registered || (sdr_type->id == SDR_TYPE_OPTION && sdr->max_value <= 0)) return;
 
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
    else if (sdr->current_value > sdr->min_value) {
      sdr->current_value--;
    }
  }

  if(sdr_type->right_btn_id != 0) if (gui_slider_button(sdr_type->right_btn_id, btn_right_dest)) {
    if (sdr->on_right_button_trigger != nullptr) {
      sdr->on_right_button_trigger();
    }
    else if (sdr->current_value < sdr->max_value-1) {
      sdr->current_value++;
    }
  }
}

void draw_slider_body(slider* sdr) {
  slider_type sdr_type = sdr->sdr_type;

  switch (sdr->sdr_type.id) {
    case SDR_TYPE_PERCENT: {
      spritesheet * circle_sprite = ss_get_spritesheet_by_enum(SHEET_ID_SLIDER_PERCENT);
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
      u16 each_body_width = (total_body_width - ((sdr->max_value) * SCREEN_OFFSET.x)) / (sdr->max_value-1);
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = Vector2 {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = Vector2 {sdr->position.x + SCREEN_OFFSET.x, sdr->position.y};
      std::string text = std::string("");
      if (sdr->localize_text) {
        text = lc_txt(sdr->options.at(sdr->current_value).localization_symbol);
      } else {
        text = sdr->options.at(sdr->current_value).no_localized_text;
      }
      Vector2 text_measure = MeasureTextEx(UI_BOLD_FONT, text.c_str(), DEFAULT_SLIDER_FONT_SIZE, UI_FONT_SPACING);

      for (int i = 1; i < sdr->max_value; ++i) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + SCREEN_OFFSET.x) * (i-1); 
        draw_sprite_on_site_by_id(sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, (i == sdr->current_value) ? 1 : 0);
      }
      Vector2 text_pos = Vector2 {
        sdr->position.x + total_body_width/2.f - text_measure.x / 2.f,
        sdr->position.y + sdr_type.dest_frame_dim.y/2.f - text_measure.y / 2.f
      };
      draw_text(text.c_str(), text_pos, UI_BOLD_FONT, DEFAULT_SLIDER_FONT_SIZE, BUTTON_TEXT_UP_COLOR, false, false, false, VECTOR2(0.f, 0.f));
      break;
    }
    case SDR_TYPE_NUMBER: {
      u16 total_body_width = sdr_type.body_width * sdr_type.width_multiply;
      u16 each_body_width = (total_body_width - ((sdr->max_value) * SCREEN_OFFSET.x)) / (sdr->max_value-1);
      f32 each_body_scale = (float)each_body_width / sdr_type.origin_body_width;
      Vector2 draw_sprite_scale = Vector2 {each_body_scale, sdr_type.scale};
      Vector2 _pos_temp = Vector2 {sdr->position.x + SCREEN_OFFSET.x, sdr->position.y};
      std::string text = sdr->options.at(0).no_localized_text;
      
      Vector2 text_measure = MeasureTextEx(UI_BOLD_FONT, text.c_str(), DEFAULT_SLIDER_FONT_SIZE, UI_FONT_SPACING);

      for (int i = 1; i < sdr->max_value; ++i) {
        Vector2 _pos = _pos_temp;
        _pos.x += (each_body_width + SCREEN_OFFSET.x) * (i-1); 
        draw_sprite_on_site_by_id(sdr_type.ss_sdr_body, WHITE, _pos, draw_sprite_scale, (i == sdr->current_value) ? 1 : 0);
      }
      Vector2 text_pos = Vector2 {
        sdr->position.x + total_body_width/2.f - text_measure.x / 2.f,
        sdr->position.y + sdr_type.dest_frame_dim.y/2.f - text_measure.y / 2.f
      };
      draw_text(text.c_str(), text_pos, UI_BOLD_FONT, DEFAULT_SLIDER_FONT_SIZE, BUTTON_TEXT_UP_COLOR, false, false, false, VECTOR2(0.f, 0.f));
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::render_slider_body()::Unsupported slider type");
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
bool gui_panel_active(panel* pan, Rectangle dest, bool _should_center) {

  if (_should_center) {
    dest.x -= dest.width / 2.f;
    dest.y -= dest.height / 2.f;
  }

  if (CheckCollisionPointRec(state->mouse_pos, dest)) {
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

  (pan->current_state == BTN_STATE_HOVER) 
    ? DrawRectanglePro(dest, Vector2 {0, 0}, 0, pan->bg_hover_tint)  //draw_texture_regular(pan->bg_tex_id, dest, pan->bg_hover_tint, false)
    : DrawRectanglePro(dest, Vector2 {0, 0}, 0, pan->bg_tint); //draw_texture_regular(pan->bg_tex_id, dest, pan->bg_tint, false);

  draw_atlas_texture_npatch(pan->frame_tex_id, dest, pan->offsets, false);

  return pan->current_state == pan->signal_state;
}
void gui_label(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v) {
  switch (type) {
    case FONT_TYPE_MEDIUM: {
      draw_text(text, position, UI_MEDIUM_FONT, font_size * UI_MEDIUM_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_BOLD: {
      draw_text(text, position, UI_BOLD_FONT, font_size * UI_BOLD_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_LIGHT: {
      draw_text(text, position, UI_LIGHT_FONT, font_size * UI_LIGHT_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_ITALIC: {
      draw_text(text, position, UI_ITALIC_FONT, font_size * UI_ITALIC_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label()::Unsupported font type");
    break;
  }
}
void gui_label_shader(const char* text, shader_id sdr_id, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v) {
  switch (type) {
    case FONT_TYPE_MEDIUM: {
      draw_text_shader(text, sdr_id, position, UI_MEDIUM_FONT, font_size * UI_MEDIUM_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_BOLD: {
      draw_text_shader(text, sdr_id, position, UI_BOLD_FONT, font_size * UI_BOLD_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_LIGHT: {
      draw_text_shader(text, sdr_id, position, UI_LIGHT_FONT, font_size * UI_LIGHT_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    case FONT_TYPE_ITALIC: {
      draw_text_shader(text, sdr_id, position, UI_ITALIC_FONT, font_size * UI_ITALIC_FONT_SIZE, tint, _center_h, _center_v, false, VECTOR2(0.f, 0.f));
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label_shader()::Unsupported font type");
    break;
  }
}
void gui_label_wrap(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center) {
  if (_should_center) {
    position.x -= (position.width / 2.f);
    position.y -= (position.height / 2.f);
  }
  switch (type) {
    case FONT_TYPE_MEDIUM: {
      DrawTextBoxed(UI_MEDIUM_FONT, text, position, font_size * UI_MEDIUM_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_BOLD: {
      DrawTextBoxed(UI_BOLD_FONT, text, position, font_size * UI_BOLD_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_LIGHT: {
      DrawTextBoxed(UI_LIGHT_FONT, text, position, font_size * UI_LIGHT_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_ITALIC: {
      DrawTextBoxed(UI_ITALIC_FONT, text, position, font_size * UI_ITALIC_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label()::Unsupported font type");
    break;
  }
}
void gui_label_grid(const char* text, font_type type, i32 font_size, Vector2 position, Color tint, bool _center_h, bool _center_v, Vector2 grid_coord) {
  switch (type) {
    case FONT_TYPE_MEDIUM: {
      draw_text(text, position, UI_MEDIUM_FONT, font_size * UI_MEDIUM_FONT_SIZE, tint, _center_h, _center_v, true, grid_coord);
      break;
    }
    case FONT_TYPE_BOLD: {
      draw_text(text, position, UI_BOLD_FONT, font_size * UI_BOLD_FONT_SIZE, tint, _center_h, _center_v, true, grid_coord);
      break;
    }
    case FONT_TYPE_LIGHT: {
      draw_text(text, position, UI_LIGHT_FONT, font_size * UI_LIGHT_FONT_SIZE, tint, _center_h, _center_v, true, grid_coord);
      break;
    }
    case FONT_TYPE_ITALIC: {
      draw_text(text, position, UI_ITALIC_FONT, font_size * UI_ITALIC_FONT_SIZE, tint, _center_h, _center_v, true, grid_coord);
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label()::Unsupported font type");
    break;
  }
}
void gui_label_wrap_grid(const char* text, font_type type, i32 font_size, Rectangle position, Color tint, bool _should_center, Vector2 grid_pos) {
  Vector2 _position = position_element_by_grid(grid_pos, VECTOR2(position.x, position.y), SCREEN_OFFSET);
  position.x = _position.x;
  position.y = _position.y;

  if (_should_center) {
    position.x -= (position.width / 2.f);
    position.y -= (position.height / 2.f);
  }
  switch (type) {
    case FONT_TYPE_MEDIUM: {
      DrawTextBoxed(UI_MEDIUM_FONT, text, position, font_size * UI_MEDIUM_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_BOLD: {
      DrawTextBoxed(UI_BOLD_FONT, text, position, font_size * UI_BOLD_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_LIGHT: {
      DrawTextBoxed(UI_LIGHT_FONT, text, position, font_size * UI_LIGHT_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    case FONT_TYPE_ITALIC: {
      DrawTextBoxed(UI_ITALIC_FONT, text, position, font_size * UI_ITALIC_FONT_SIZE, UI_FONT_SPACING, true, tint);
      break;
    }
    default: TraceLog(LOG_WARNING, "user_interface::gui_label()::Unsupported font type");
    break;
  }

}

void gui_draw_settings_screen(void) { // TODO: Return to settings later
  Rectangle settings_bg_pnl_dest = Rectangle{ BASE_RENDER_SCALE(.025f).x, BASE_RENDER_SCALE(.075f).y, BASE_RENDER_SCALE(.950f).x, BASE_RENDER_SCALE(.850f).y};
  Rectangle header_loc = {0, 0, BASE_RENDER_RES.x, BASE_RENDER_SCALE(.1f).y};
  Rectangle footer_loc = {0, BASE_RENDER_RES.y - BASE_RENDER_SCALE(.1f).y, BASE_RENDER_RES.x, BASE_RENDER_SCALE(.1f).y};
  DrawRectangleRec(header_loc, Color{0, 0, 0, 50});
  DrawRectangleRec(footer_loc, Color{0, 0, 0, 50});

  gui_panel(panel(), settings_bg_pnl_dest, false);

  gui_slider(SDR_ID_SETTINGS_SOUND_SLIDER, BASE_RENDER_SCALE(.5f), VECTOR2(0.f,-20.f));

  gui_slider(SDR_ID_SETTINGS_RES_SLIDER, BASE_RENDER_SCALE(.5f), VECTOR2(0.f, -10.f));

  gui_slider(SDR_ID_SETTINGS_WIN_MODE_SLIDER, BASE_RENDER_SCALE(.5f), VECTOR2(0.f, 0.f));

  gui_slider(SDR_ID_SETTINGS_LANGUAGE, BASE_RENDER_SCALE(.5f), VECTOR2(0.f, 10.f));

  if(gui_menu_button(lc_txt(LOC_TEXT_SETTINGS_BUTTON_APPLY), BTN_ID_SETTINGS_APPLY_SETTINGS_BUTTON, VECTOR2(50.f, 100.f), BASE_RENDER_RES_DIV2, true)) {
    slider sdr_win_mode = state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER);
    i32 window_mod = sdr_win_mode.options.at(sdr_win_mode.current_value).content.data.i32[0];
    
    if (window_mod == FLAG_BORDERLESS_WINDOWED_MODE && !IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
      event_fire(EVENT_CODE_TOGGLE_BORDERLESS, event_context());
      set_resolution_slider_native_res();
    }
    else if (window_mod == FLAG_FULLSCREEN_MODE && !IsWindowFullscreen()) {
      event_fire(EVENT_CODE_TOGGLE_FULLSCREEN, event_context());
      set_resolution_slider_native_res();
    }
    else if (window_mod == 0 && (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) || IsWindowFullscreen())) {
      Vector2 res = pVECTOR2(SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16);
      set_resolution(res.x, res.y);
      event_fire(EVENT_CODE_TOGGLE_WINDOWED, event_context());
    }
    
    u32 language_index = SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_LANGUAGE).content.data.u32[0];
    if (state->display_language->language_index != language_index) {
      if(loc_parser_set_active_language_by_index(language_index)) {
        loc_data* loc = loc_parser_get_active_language();
        state->display_language = ui_get_localization_by_index(loc->index);
        set_language(loc->language_name.c_str());
      }
      else {
        TraceLog(LOG_ERROR, "user_interface::gui_draw_settings_screen()::Language changing failed");
      }
    }

    ui_refresh_setting_sliders_to_default();
    save_ini_file();
  }
}
void ui_refresh_setting_sliders_to_default(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_refresh_setting_sliders_to_default()::State is not valid");
    return;
  }

  SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value = static_cast<u16>(state->in_app_settings->master_sound_volume * .1f);
  for (size_t iter = 0; iter < state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).max_value; ++iter) {
    if (state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).options.at(iter).content.data.i32[0] == state->in_app_settings->window_state && iter != 0) {
      state->sliders.at(SDR_ID_SETTINGS_WIN_MODE_SLIDER).current_value = iter;
      break;
    }
  }
  
  const std::vector<f32>& window_size = state->in_app_settings->window_size;
  for (size_t iter = 0; iter < state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).max_value; ++iter) {
    slider_option& option = state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).options.at(iter);
    if (option.content.data.u16[0] == window_size.at(0) && option.content.data.u16[1] == window_size.at(1)) {
      state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).current_value = iter;
      break;
    }
  }
  
  if (SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16[0] != window_size.at(0) || SDR_CURR_OPT_VAL(SDR_ID_SETTINGS_RES_SLIDER).content.data.u16[1] != window_size.at(1)) {
    TraceLog(LOG_ERROR, "user_interface::user_interface_system_initialize()::Unsupported resolution: {%d:%d}", window_size.at(0), window_size.at(1));
    set_resolution(1920, 1080); // TODO: Set default resolution based on aspect ratio
    for (size_t iter = 0; iter < state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).max_value; ++iter) {
      slider_option& option = state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).options.at(iter);
      if (option.content.data.u16[0] == window_size.at(0) && option.content.data.u16[1] == window_size.at(1)) {
        state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).current_value = iter;
        break;
      }
    }
  }

  u32 language_index = loc_parser_get_active_language()->index;
  for (size_t iter = 0; iter < SDR_AT(SDR_ID_SETTINGS_LANGUAGE).max_value; ++iter) {
    data_pack opt = SDR_AT(SDR_ID_SETTINGS_LANGUAGE).options.at(iter).content;
    if (opt.data.u32[0] == language_index && (opt.type_flag < DATA_TYPE_MAX && opt.type_flag > DATA_TYPE_UNRESERVED)) {
      SDR_AT(SDR_ID_SETTINGS_LANGUAGE).current_value = iter;
      break;
    }
  }
}
void gui_draw_pause_screen(bool in_game_play_state) {
  Rectangle dest = Rectangle {
    BASE_RENDER_SCALE(.5f).x,
    BASE_RENDER_SCALE(.5f).y,
    BASE_RENDER_SCALE(.2f).x,
    BASE_RENDER_SCALE(.6f).y
  };
  gui_panel(panel(), dest, true);

  if (in_game_play_state) {
    if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_PAUSE_BUTTON_TEXT_RESUME), BTN_ID_PAUSEMENU_BUTTON_RESUME, Vector2 {0.f, -15.f}, BASE_RENDER_RES_DIV2, true)) {
      event_fire(EVENT_CODE_RESUME_GAME, event_context());
    }
  }
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_BUTTON_TEXT_SETTINGS), BTN_ID_PAUSEMENU_BUTTON_SETTINGS, Vector2 {0.f, -5.f}, BASE_RENDER_RES_DIV2, true)) {}
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_PAUSE_BUTTON_TEXT_EXIT_TO_MAINMENU), BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_MAIN_MENU, Vector2 { 0.f, 5.f}, BASE_RENDER_RES_DIV2, true)) {
    event_fire(EVENT_CODE_SCENE_MAIN_MENU, event_context());
  }
  if (gui_menu_button(lc_txt(LOC_TEXT_MAINMENU_PAUSE_BUTTON_TEXT_EXIT_TO_DESKTOP), BTN_ID_PAUSEMENU_BUTTON_EXIT_TO_DESKTOP, Vector2 { 0.f, 15.f}, BASE_RENDER_RES_DIV2, true)) {
    event_fire(EVENT_CODE_APPLICATION_QUIT, event_context());
  }
}

bool gui_slider_add_option(slider_id _id, data_pack content, u32 _localization_symbol, std::string _no_localized_text) {
  if (_id >= SDR_ID_MAX || _id <= SDR_ID_UNDEFINED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider_add_option()::Slider ids was out of bound");
    return false;
  }
  slider* sdr = &state->sliders.at(_id);
  if (!sdr->is_registered) {
    TraceLog(LOG_WARNING, "user_interface::gui_slider_add_option()::Given slider didn't registered");
    return false;
  }
  if (sdr->max_value < MAX_SLIDER_OPTION_SLOT) {
    sdr->options.at(sdr->max_value) = slider_option(_no_localized_text.c_str(), _localization_symbol, content);
    sdr->max_value++;
    return true;
  }
  else {
    TraceLog(LOG_ERROR, "user_interface::gui_slider_add_option()::You've reached the maximum amouth of option slot");
    return false;
  }
}
void register_button_type(button_type_id _btn_type_id, spritesheet_id _ss_type, Vector2 frame_dim, f32 _scale, bool _should_center) {
  if (_ss_type     >= SHEET_ID_SPRITESHEET_TYPE_MAX || _ss_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED || 
      _btn_type_id >= BTN_TYPE_MAX         || _btn_type_id <= BTN_TYPE_UNDEFINED  ||
      !state) {
    TraceLog(LOG_WARNING, "user_interface::register_button_type()::Recieved id was out of bound");
    return;
  }
  button_type btn_type = button_type(_btn_type_id, _ss_type, frame_dim, VECTOR2(frame_dim.x * _scale, frame_dim.y * _scale), _scale, _should_center);
  state->button_types.at(_btn_type_id) = btn_type;
}
void register_button(button_id _btn_id, button_type_id _btn_type_id) {
  if (_btn_id      >= BTN_ID_MAX   || _btn_id      <= BTN_ID_UNDEFINED   || 
      _btn_type_id >= BTN_TYPE_MAX || _btn_type_id <= BTN_TYPE_UNDEFINED || !state) 
  {
    TraceLog(LOG_WARNING, "user_interface::register_button()::One of recieved ids was out of bound");
    return;
  }
  button_type* _btn_type = __builtin_addressof(state->button_types.at(_btn_type_id));
  button btn = button(_btn_id, state->button_types.at(_btn_type_id), BTN_STATE_UP, Rectangle{0.f, 0.f, _btn_type->dest_frame_dim.x, _btn_type->dest_frame_dim.y});

  state->buttons.at(_btn_id) = btn;
}
void register_progress_bar(progress_bar_id _id, progress_bar_type_id _type_id, f32 width_multiply, Vector2 scale) {
  if (_id     >= PRG_BAR_ID_MAX      || _id      <= PRG_BAR_ID_UNDEFINED      ||
      _type_id>= PRG_BAR_TYPE_ID_MAX || _type_id <= PRG_BAR_TYPE_ID_UNDEFINED ||
      !state) {
    TraceLog(LOG_WARNING, "user_interface::register_progress_bar()::Recieved id was out of bound");
    return;
  }
  progress_bar prg_bar = progress_bar();

  prg_bar.type = state->prg_bar_types.at(_type_id);
  prg_bar.id = _id;
  prg_bar.scale = scale;
  prg_bar.width_multiply = width_multiply;
  prg_bar.is_initialized = true;

  state->prg_bars.at(_id) = prg_bar;
}
void register_progress_bar_type(progress_bar_type_id _type_id, atlas_texture_id _body_inside, atlas_texture_id _body_outside, shader_id _mask_shader_id) {
  if (_type_id       >= PRG_BAR_TYPE_ID_MAX || _type_id       <= PRG_BAR_TYPE_ID_UNDEFINED ||
      _body_inside   >= ATLAS_TEX_ID_MAX          || _body_inside   <= ATLAS_TEX_ID_UNSPECIFIED        ||
      _body_outside  >= ATLAS_TEX_ID_MAX          || _body_outside  <= ATLAS_TEX_ID_UNSPECIFIED        ||
      _mask_shader_id>= SHADER_ID_MAX       || _mask_shader_id<= SHADER_ID_UNSPECIFIED     ||
      !state) {
    TraceLog(LOG_WARNING, "user_interface::register_progress_bar_type()::Recieved id was out of bound");
    return;
  }
  progress_bar_type prg_type = progress_bar_type();

  prg_type.body_inside     = _body_inside;
  prg_type.body_outside    = _body_outside;
  prg_type.mask_shader_id  = _mask_shader_id;

  state->prg_bar_types.at(_type_id) = prg_type;
}
void register_slider_type(
  slider_type_id _sdr_type_id, spritesheet_id _ss_sdr_body_type, f32 _scale, u16 _width_multiply,
  button_type_id _left_btn_type_id, button_type_id _right_btn_type_id, u16 _char_limit) {
    if (_ss_sdr_body_type >= SHEET_ID_SPRITESHEET_TYPE_MAX || _ss_sdr_body_type <= SHEET_ID_SPRITESHEET_UNSPECIFIED || 
        _sdr_type_id      >= SDR_TYPE_MAX         || _sdr_type_id      <= SDR_TYPE_UNDEFINED      ||
        _left_btn_type_id >= BTN_TYPE_MAX         || _left_btn_type_id <= BTN_TYPE_UNDEFINED      ||
        _right_btn_type_id>= BTN_TYPE_MAX         || _right_btn_type_id<= BTN_TYPE_UNDEFINED      ||
        !state) {
      TraceLog(LOG_WARNING, "WARNING::user_interface::register_slider_type()::One of recieved ids was out of bound");
      return;
    }

  spritesheet ss_body = *ss_get_spritesheet_by_enum(_ss_sdr_body_type);
  button_type* left_btn_type = __builtin_addressof(state->button_types.at(_left_btn_type_id)); 
  button_type* right_btn_type = __builtin_addressof(state->button_types.at(_right_btn_type_id)); 

  slider_type sdr_type = slider_type(_sdr_type_id,_ss_sdr_body_type,
    VECTOR2(ss_body.current_frame_rect.width, ss_body.current_frame_rect.height),
    _scale, _width_multiply, (u16)(_scale * _char_limit), 
    _left_btn_type_id, _right_btn_type_id,
    (u16) (left_btn_type->source_frame_dim.x * left_btn_type->scale), (u16) (right_btn_type->source_frame_dim.x * right_btn_type->scale),
    (u16) ss_body.current_frame_rect.width, (u16) (ss_body.current_frame_rect.width * _scale)
  );

  sdr_type.dest_frame_dim = Vector2 { (f32) sdr_type.body_width * sdr_type.width_multiply, ss_body.current_frame_rect.height * _scale };

  state->slider_types.at(_sdr_type_id) = sdr_type;
}
/**
 * @param _is_clickable for SDR_TYPE_PERCENT type sliders. Does not affect others
 */
void register_slider(
  slider_id _sdr_id, slider_type_id _sdr_type_id, 
  button_id _left_btn_id, button_id _right_btn_id, bool _is_clickable, bool _localize_text) {
  if (_sdr_id       >= SDR_ID_MAX   || _sdr_id      <= SDR_ID_UNDEFINED  || 
      _left_btn_id  >= BTN_ID_MAX   || _left_btn_id  < BTN_ID_UNDEFINED  || 
      _right_btn_id >= BTN_ID_MAX   || _right_btn_id < BTN_ID_UNDEFINED  || 
      _sdr_type_id  >= SDR_TYPE_MAX || _sdr_type_id <= SDR_TYPE_UNDEFINED||
      !state) 
  {
    TraceLog(LOG_WARNING, "user_interface::register_slider()::One of recieved ids was out of bound");
    return;
  }

  slider_type* _sdr_type = __builtin_addressof(state->slider_types.at(_sdr_type_id));
  
  //slider_id _id, slider_type _type, u16 current_value, u16 max_value, u16 min_value, bool _localized_text, bool _is_clickable
  slider sdr = slider(
    _sdr_id, *_sdr_type, 
    static_cast<u16>(_sdr_type_id == SDR_TYPE_PERCENT ? 7 : 1 ), static_cast<u16>(_sdr_type_id == SDR_TYPE_PERCENT ? DEFAULT_PERCENT_SLIDER_CIRCLE_AMOUTH : 1), 1,
    _localize_text,
    _is_clickable
  );

  if(_left_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.left_btn_id = _left_btn_id;

    register_button(
      sdr.sdr_type.left_btn_id, sdr.sdr_type.left_btn_type_id
    );
  }
  if(_right_btn_id != BTN_ID_UNDEFINED){
    sdr.sdr_type.right_btn_id = _right_btn_id;
    register_button(
      sdr.sdr_type.right_btn_id, sdr.sdr_type.right_btn_type_id
    );
  }

  state->sliders.at(_sdr_id) = sdr;
}
void gui_draw_atlas_texture_to_background(atlas_texture_id _id) {
  draw_atlas_texture_regular(_id, Rectangle {0, 0, BASE_RENDER_RES.x, BASE_RENDER_RES.y}, WHITE, false);
}
void gui_draw_spritesheet_to_background(spritesheet_id _id, Color _tint) {
  if (_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED || !state) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_spritesheet_to_background()::Sprite type out of bound");
    return;
  }
  if (state->ss_to_draw_bg.sheet_id != _id) {
    state->ss_to_draw_bg = *ss_get_spritesheet_by_enum(_id);
    set_sprite(&state->ss_to_draw_bg, true, false);
  }
  Rectangle dest = Rectangle {0, 0, BASE_RENDER_RES.x, BASE_RENDER_RES.y};
  play_sprite_on_site(&state->ss_to_draw_bg, _tint, dest);
}
/**
 * @note  function, returns "Rectangle {}" if texture type returns null pointer
 * @return Rectangle { .x = 0, .y = 0, .width = tex->width, .height = tex->height}; 
 */
 Rectangle get_atlas_texture_source_rect(atlas_texture_id _id) {
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::get_atlas_texture_source_rect()::Requested type was null");
    return ZERORECT; 
  }
  
  return tex->source;
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
 void draw_atlas_texture_regular(atlas_texture_id _id, Rectangle dest, Color tint, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, 
    "user_interface::draw_texture_regular()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { TraceLog(
  LOG_WARNING, "user_interface::draw_texture_regular()::Tex was null");
    return; 
  }
  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }
  DrawTexturePro(*tex->atlas_handle, 
  tex->source, 
  dest, 
  ZEROVEC2, 0, tint);
}
/**
 * @brief Centers over -> dest.x -= dest.width / 2.f; 
 */
 void draw_atlas_texture_npatch(atlas_texture_id _id, Rectangle dest, Vector4 offsets, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::draw_atlas_texture_npatch()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::draw_atlas_texture_npatch()::Tex was null"); 
    return; 
  }

  if (should_center) {
    dest.x -= dest.width / 2.f; 
    dest.y -= dest.height / 2.f; 
  }

  NPatchInfo npatch = {
    tex->source,
    (i32) offsets.x,
    (i32)offsets.y,
    (i32)offsets.z,
    (i32)offsets.w,
    NPATCH_NINE_PATCH
  };

  DrawTextureNPatch(*tex->atlas_handle, npatch, dest, ZEROVEC2, 0, WHITE);
}
 Vector2 make_vector(f32 x, f32 y) {
  return Vector2 {x,y};
}
 void gui_draw_map_stage_pin(bool have_hovered, Vector2 screen_loc) {
  const Vector2 icon_size = NORMALIZE_VEC2(32.f, 32.f, 1280, 720);
  Rectangle icon_loc = Rectangle {screen_loc.x, screen_loc.y, icon_size.x * BASE_RENDER_RES.x, icon_size.y * BASE_RENDER_RES.y}; 
  icon_loc.x -= icon_loc.width  * .5f;
  icon_loc.y -= icon_loc.height * .5f;
  
  if(have_hovered) {
    gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, Rectangle{1632, 928, 32, 32}, icon_loc); // INFO: MAP PIN TEXTURES
  }
  else {
    gui_draw_texture_id_pro(TEX_ID_ASSET_ATLAS, Rectangle{1600, 928, 32, 32}, icon_loc);
  }
}
/**
 * @brief grid_pos, grid_coord, grid_dim 
 */
Vector2 position_element_by_grid(Vector2 grid_pos, Vector2 grid_coord, Vector2 grid_dim) {
  return Vector2{
      .x = grid_pos.x + (grid_coord.x * grid_dim.x) ,
      .y = grid_pos.y + (grid_coord.y * grid_dim.y) 
  };
}

constexpr void draw_text_shader(const char *text, shader_id sdr_id, Vector2 position, Font font, float fontsize, Color tint, bool center_horizontal, bool center_vertical, bool use_grid_align, Vector2 grid_coord) {
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
  int length = TextLength(text);

  float textOffsetX = 0.0f;
  float scaleFactor = fontsize/(float)font.baseSize;

  for (int i = 0, k = 0; i < length; i++, k++)
  {
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[i], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);
    float glyphWidth = 0;

    glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;
    if (i + 1 < length) glyphWidth = glyphWidth + UI_FONT_SPACING;
    
    if ((codepoint != ' ') && (codepoint != '\t')) {
      f32 letter_width  = (font.recs[index].width + 2.0f*font.glyphPadding) * scaleFactor;
      f32 letter_height = (font.recs[index].height + 2.0f*font.glyphPadding) * scaleFactor;

      switch (sdr_id) {
        case SHADER_ID_FONT_OUTLINE: {
          BeginShaderMode(get_shader_by_enum(SHADER_ID_FONT_OUTLINE)->handle);
            i32 uni_tex_size_loc = GetShaderLocation(get_shader_by_enum(SHADER_ID_FONT_OUTLINE)->handle, "texture_size");
            set_shader_uniform(SHADER_ID_FONT_OUTLINE, uni_tex_size_loc, data128(letter_width, letter_height));
            DrawTextCodepoint(font, codepoint, Vector2{ text_position.x + textOffsetX, text_position.y }, fontsize, tint);
          EndShaderMode();
          break;
        }
        default: {
          TraceLog(LOG_WARNING, "user_interface::draw_text_shader()::Unsupported shader id");
          break;
        }
      }
    }
    if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;
  }
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
            //DrawTextCodepoint(font, codepoint, Vector2{ rec.x + textOffsetX, rec.y + textOffsetY}, fontSize+5, TEXT_SHADOW_COLOR);
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
}

void draw_fade_effect() {
  if (!state) {
    TraceLog(LOG_WARNING, "user_interface::draw_fade_effect()::User interface didn't initialized");
    return;
  }
  if (!state->fade_animation_playing) {
    TraceLog(LOG_WARNING, "user_interface::draw_fade_effect()::Funtions called without starting animation");
    return;
  }
  f32 process = state->fadein 
    ? EaseQuadIn(state->fade_animation_timer,  0.f, 1.f, state->fade_animation_duration)
    : EaseQuadOut(state->fade_animation_timer, 1.f,-1.f, state->fade_animation_duration);
  BeginShaderMode(get_shader_by_enum(SHADER_ID_FADE_TRANSITION)->handle);
  set_shader_uniform(SHADER_ID_FADE_TRANSITION, 0, data128(process));
  draw_atlas_texture_regular(ATLAS_TEX_ID_BG_BLACK, Rectangle {0, 0, BASE_RENDER_RES.x, BASE_RENDER_RES.y}, WHITE, false);
  EndShaderMode();
}
/**
 * @brief relative if you want to draw from another atlas.
 */
void gui_draw_atlas_texture_id_pro(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_pro()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_pro()::Tex was null");
    return; 
  }
  if (relative) {
    src.x += tex->source.x;
    src.y += tex->source.y;
  }
  if (should_center) {
    dest.x -= dest.width / 2.f;
    dest.y -= dest.height / 2.f;
  }
  DrawTexturePro(*tex->atlas_handle, src, dest, ZEROVEC2, 0, WHITE);
}
void gui_draw_atlas_texture_id(atlas_texture_id _id, Rectangle dest, Vector2 origin, f32 rotation) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id()::Tex was null");
    return; 
  }

  DrawTexturePro(*tex->atlas_handle, tex->source, dest, origin, rotation, WHITE);
}
void gui_draw_atlas_texture_id_pro_grid(atlas_texture_id _id, Rectangle src, Rectangle dest, bool relative) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_pro_grid()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_pro_grid()::Tex was null");
    return; 
  }
  if (relative) {
    src.x += tex->source.x;
    src.y += tex->source.y;
  }
  Vector2 pos = position_element_by_grid(BASE_RENDER_SCALE(.5f), VECTOR2(dest.x, dest.y), SCREEN_OFFSET);
  dest.x = pos.x;
  dest.y = pos.y;
  DrawTexturePro(*tex->atlas_handle, src, dest, Vector2 { tex->source.width * .5f, tex->source.height *.5f }, 0, WHITE);
}
void gui_draw_atlas_texture_id_grid(atlas_texture_id _id, Rectangle dest) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_grid()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_grid()::Tex was null");
    return; 
  }
  Vector2 pos = position_element_by_grid(BASE_RENDER_SCALE(.5f), VECTOR2(dest.x, dest.y), SCREEN_OFFSET);
  dest.x = pos.x;
  dest.y = pos.y;
  DrawTexturePro(*tex->atlas_handle, tex->source, dest, ZEROVEC2, 0, WHITE);
}
void gui_draw_spritesheet_id(spritesheet_id _id, Color _tint, Vector2 pos, Vector2 scale, u16 frame) {
  if (_id >= SHEET_ID_SPRITESHEET_TYPE_MAX || _id <= SHEET_ID_SPRITESHEET_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_spritesheet_id()::ID was out of bound"); 
    return; 
  }
  draw_sprite_on_site_by_id(_id, _tint, pos, scale, frame);
}
void gui_draw_texture_id_pro(texture_id _id, Rectangle src, Rectangle dest, Color tint, Vector2 origin) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::ID was out of bound"); 
    return; 
  }
  Texture2D* tex = ss_get_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id_pro()::Tex was null");
    return; 
  }
  DrawTexturePro(*tex, src, dest, origin, 0, tint);
}
void gui_draw_texture_id(texture_id _id, Rectangle dest) {
  if (_id >= TEX_ID_MAX || _id <= TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id()::ID was out of bound"); 
    return; 
  }
  Texture2D* tex = ss_get_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_texture_id()::Tex was null");
    return; 
  }
  DrawTexturePro(*tex, 
  Rectangle{0.f, 0.f, (f32) tex->width, (f32) tex->height},
  dest, 
  ZEROVEC2, 0.f, WHITE);
}
void gui_draw_atlas_texture_id_scale(atlas_texture_id _id, Vector2 position, f32 scale, Color tint, bool should_center) {
  if (_id >= ATLAS_TEX_ID_MAX || _id <= ATLAS_TEX_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_scale()::ID was out of bound"); 
    return; 
  }
  atlas_texture* tex = ss_get_atlas_texture_by_enum(_id);
  if (!tex) { 
    TraceLog(LOG_WARNING, "user_interface::gui_draw_atlas_texture_id_scale()::Tex was null");
    return; 
  }
  if (should_center) {
    position.x -= tex->source.width / 2.f; 
    position.y -= tex->source.height / 2.f; 
  }

  DrawTexturePro(*tex->atlas_handle, 
  tex->source, 
  Rectangle {position.x, position.y, tex->source.width * scale, tex->source.height * scale}, 
  ZEROVEC2, 0, tint);
}

Font* ui_get_font(font_type font) {
  if (!state) {
    TraceLog(LOG_WARNING, "user_interface::ui_get_font()::State is not valid");
    return nullptr;
  }
  switch (font) {
  case FONT_TYPE_MEDIUM: return &UI_MEDIUM_FONT;
  case FONT_TYPE_BOLD:   return &UI_BOLD_FONT;
  case FONT_TYPE_LIGHT:  return &UI_LIGHT_FONT;
  case FONT_TYPE_ITALIC: return &UI_ITALIC_FONT;
  default: TraceLog(LOG_WARNING, "user_interface::ui_get_font()::Unknown font type");
  }
  return nullptr;
}
Vector2* ui_get_mouse_pos(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_get_mouse_pos()::State is not valid");
    return nullptr;
  }
  return &state->mouse_pos;
}
slider* get_slider_by_id(slider_id sdr_id) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::get_slider_by_id()::State is not valid");
    return nullptr;
  }
  if (sdr_id <= SDR_ID_UNDEFINED || sdr_id >= SDR_ID_MAX) {
    TraceLog(LOG_ERROR, "user_interface::get_slider_by_id()::ID is out of bound");
    return nullptr;
  }

  return __builtin_addressof(state->sliders.at(sdr_id));
}
bool ui_set_slider_current_index(slider_id id, u16 index) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_set_slider_current_index()::State is not valid");
    return false;
  }
  if (id >= SDR_ID_MAX || id <= SDR_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "user_interface::ui_set_slider_current_index()::ID was out of bound"); 
    return false;
  }

  state->sliders.at(id).current_value = index;
  return true;
}
bool ui_set_slider_current_value(slider_id id, slider_option value) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_set_slider_current_value()::State is not valid");
    return false;
  }
  if (id >= SDR_ID_MAX || id <= SDR_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "user_interface::ui_set_slider_current_value()::ID was out of bound"); 
    return false;
  }

  SDR_CURR_OPT_VAL(id) = value;
  return true;
}
bool ui_sound_slider_on_left_button_trigger(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_sound_slider_on_left_button_trigger()::State is not valid");
    return false;
  }

  SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value--;
  if (SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value > 10) {
    SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value = 0;
  }
  set_master_sound(SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value * 10);
  return true;
}
bool ui_sound_slider_on_right_button_trigger(void) {  
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_sound_slider_on_right_button_trigger()::State is not valid");
    return false;
  }

  SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value++;
  if (SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value > 10) {
    SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value = 10;
  }
  set_master_sound(SDR_AT(SDR_ID_SETTINGS_SOUND_SLIDER).current_value * 10);
  return true;
}

data_pack* get_slider_current_value(slider_id id) {
  if (!state) {
    TraceLog(LOG_WARNING, "user_interface::get_slider_current_value()::State is not valid"); 
    return nullptr;
  }
  if (id >= SDR_ID_MAX || id <= SDR_ID_UNDEFINED) {
    TraceLog(LOG_WARNING, "user_interface::get_slider_current_value()::ID was out of bound"); 
    return nullptr;
  }
  return __builtin_addressof(state->sliders
    .at(id).options
    .at(state->sliders
      .at(id).current_value).content
  );
}
bool is_ui_fade_anim_complete(void) {
  return state->fade_animation_timer == 0;
}
bool is_ui_fade_anim_about_to_complete(void) {
  return state->fade_animation_timer == state->fade_animation_duration-1;
}
void set_resolution_slider_native_res(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::set_resolution_slider_native_res()::State is null");
    return;
  }
  
  i32 monitor = GetCurrentMonitor();
  Vector2 res = Vector2 { (f32) GetMonitorWidth(monitor), (f32) GetMonitorHeight(monitor)};

  for (int i=0; i<state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).max_value; ++i) {
    if (
      state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).options.at(i).content.data.u16[0] == res.x &&
      state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).options.at(i).content.data.u16[1] == res.y
    ) {
      state->sliders.at(SDR_ID_SETTINGS_RES_SLIDER).current_value = i;
      break;
    }
  }
}
Font load_font(const char* file_name, i32 font_size, i32* _codepoints, i32 _codepoint_count) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::load_font()::State is not valid");
    return {};
  }

  Font font = {};

  const char* path = TextFormat("%s%s", RESOURCE_PATH, file_name);
  font = LoadFontEx(path, font_size, _codepoints, _codepoint_count);

  if (font.baseSize == 0) { // If custom font load failed
    TraceLog(LOG_WARNING, "user_interface::load_font()::Font cannot loading, returning default");
    return GetFontDefault();
  }
  return font;
}

localization_package* load_localization(std::string language_name, u32 loc_index, std::string _codepoints, i32 font_size) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::load_localization()::State is not valid");
    return nullptr;
  }
  localization_package loc_pack = localization_package();

  i32 codepoint_count = 1;
  i32* codepoints = LoadCodepoints(_codepoints.c_str(), &codepoint_count);
  
  loc_pack.language_index = loc_index;
  loc_pack.language_name = language_name;
  loc_pack.codepoints = codepoints;
  loc_pack.light_font  = load_font("miosevka_light.ttf",        font_size, codepoints, codepoint_count);
  loc_pack.bold_font   = load_font("miosevka_bold.ttf",         font_size, codepoints, codepoint_count);
  loc_pack.medium_font = load_font("miosevka_medium.ttf",       font_size, codepoints, codepoint_count);
  loc_pack.italic_font = load_font("miosevka_medium_italic.ttf",font_size, codepoints, codepoint_count);

  state->localization_info.push_back(loc_pack);

  return &state->localization_info.at(
    state->localization_info.size()-1
  );
}
localization_package* ui_get_localization_by_name(std::string language_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_get_localization_by_name()::State is not valid");
    return nullptr;
  }

  for (size_t iter = 0; iter < state->localization_info.size(); iter++) {
    if (state->localization_info.at(iter).language_name == language_name) {
      return &state->localization_info.at(iter);
    }
  }

  return nullptr;
}
localization_package* ui_get_localization_by_index(u32 _language_index) {
  if (!state) {
    TraceLog(LOG_ERROR, "user_interface::ui_get_localization_by_index()::State is not valid");
    return nullptr;
  }

  for (size_t iter = 0; iter < state->localization_info.size(); iter++) {
    if (state->localization_info.at(iter).language_index == _language_index) {
      return &state->localization_info.at(iter);
    }
  }

  return nullptr;
}

// EXPOSED
void ui_play_sprite_on_site(spritesheet *sheet, Color _tint, Rectangle dest) {
  play_sprite_on_site(sheet, _tint, dest);
}
void ui_set_sprite(spritesheet *sheet, bool _play_looped, bool _play_once) {
  set_sprite(sheet, _play_looped, _play_once);
}
void ui_update_sprite(spritesheet *sheet) {
  update_sprite(sheet);
}
// EXPOSED

void user_interface_system_destroy(void) {}

bool user_interface_on_event(u16 code, event_context context) {
  switch (code) {
    case EVENT_CODE_UI_UPDATE_PROGRESS_BAR: {
      state->prg_bars.at((i32)context.data.f32[0]).progress = context.data.f32[1];
      return true;
    }
    case EVENT_CODE_UI_START_FADEIN_EFFECT: {
      state->fade_animation_duration = context.data.u16[0];
      state->fade_animation_playing = true;
      state->fade_animation_timer = 0;
      state->fadein = true;
      return true;
    }
    case EVENT_CODE_UI_START_FADEOUT_EFFECT: {
      state->fade_animation_duration = context.data.u16[0];
      state->fade_animation_playing = true;
      state->fade_animation_timer = 0;
      state->fadein = false;
      return true;
    }
  };

  return false;
}

#undef SPACE_BTW_V
#undef DEFAULT_MENU_BUTTON_SCALE
#undef MENU_BUTTON_FONT
#undef MENU_BUTTON_FONT_SIZE
#undef MINI_BUTTON_FONT
#undef MINI_BUTTON_FONT_SIZE
#undef LABEL_MOOD_FONT_SIZE
#undef LABEL_MOOD_OUTLINE_FONT_SIZE
#undef LABEL_MINI_FONT_SIZE
#undef LABEL_MINI_OUTLINE_FONT_SIZE
#undef draw_text
#undef SDR_CURR_VAL
#undef FADE_ANIMATION_DURATION
#undef SCREEN_POS
