#include "tools/lexer_ini.h"
#include "defines.h"

#include "tools/fstring.h"

#define DATA_TYPE_C_DELIMITER 20
#define DATA_TYPE_I64_DELIMITER 19
#define DATA_TYPE_U64_DELIMITER 20
#define DATA_TYPE_F64_DELIMITER 20
#define DATA_TYPE_I32_DELIMITER 10
#define DATA_TYPE_U32_DELIMITER 10
#define DATA_TYPE_F32_DELIMITER 20
#define DATA_TYPE_I16_DELIMITER 5
#define DATA_TYPE_U16_DELIMITER 5
#define DATA_TYPE_I8_DELIMITER 3 
#define DATA_TYPE_U8_DELIMITER 3

i64 get_variable_I64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
u64 get_variable_U64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
f64 get_variable_F64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
i32 get_variable_I32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
u32 get_variable_U32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
f32 get_variable_F32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
i16 get_variable_I16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
u16 get_variable_U16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
i8  get_variable_I8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);
u8  get_variable_U8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable);

const char* get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section);
const char* get_value_number(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name, data_type type);
const char* get_value_string(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name);

bool is_alpha(char *c);
bool is_letter(char *c);
bool is_digit(char *c);
bool is_number(char *c);
bool is_unsigned_number(char *c);
bool is_variable_allowed(char *c);
bool is_string_allowed(char *c);
void move_next_alpha(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line);
void move_next_letter(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line);
void move_next_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line);
void move_next_string(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line);

bool parse_app_settings_ini(const char* filename, app_settings* out_settings) {

  i32 size = 0;
  char file_str[INI_FILE_MAX_FILE_SIZE] = "";
  char section_resolution[INI_FILE_MAX_SECTION_LENGTH] = "";
  char section_sound[INI_FILE_MAX_SECTION_LENGTH] = "";
  char section_title[INI_FILE_MAX_SECTION_LENGTH] = "";
  char section_window[INI_FILE_MAX_SECTION_LENGTH] = "";
  u8* _str = LoadFileData(filename, &size);
  if (size > INI_FILE_MAX_FILE_SIZE) {
    TraceLog(LOG_ERROR, "lexer_ini::parse_app_settings_ini()::Ini file size exceeded.");
    return false;
  }
  TextCopy(file_str, (char*)_str);
  TextCopy(section_resolution, get_section(file_str, "resolution"));
  TextCopy(section_sound,      get_section(file_str, "sound"));
  TextCopy(section_title,      get_section(file_str, "title"));
  TextCopy(section_window,      get_section(file_str, "window"));
  out_settings->resolution[0] = get_variable_I32(section_resolution, "width");
  out_settings->resolution[1] = get_variable_I32(section_resolution, "height");
  out_settings->master_sound_volume = get_variable_U16(section_sound, "master");
  const char* str_win_mode = get_value_string(section_window, "mode");
  if (TextIsEqual(str_win_mode, "borderless")) {
    out_settings->window_state = FLAG_BORDERLESS_WINDOWED_MODE;
  }
  else if (TextIsEqual(str_win_mode, "fullscreen")) {
    out_settings->window_state = FLAG_FULLSCREEN_MODE;
  }
  else if (TextIsEqual(str_win_mode, "windowed")) {
    out_settings->window_state = 0;
  }
  TextCopy(out_settings->title, get_value_string(section_title, "title"));
  return true;
}

const char* get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section) {
    u16 counter = 0;
    while(_str[counter] != '\0') {
        if (_str[counter] == '[') {
            char section_name[INI_FILE_MAX_SECTION_NAME_LENGTH] = "";
            u16 section_name_counter = 0;
            counter++;
            while (_str[counter] != ']' && _str[counter] != '\0') {
                if(_str[counter] != '\r' && _str[counter] != '\n') {     
                    section_name[section_name_counter] = _str[counter];
                    section_name_counter++;
                }
                counter++;
            }
            counter++;
            if(TextIsEqual(section_name, section)){
                char _section_str[INI_FILE_MAX_SECTION_LENGTH] = "";
                u16 section_str_counter = 0;
                while (_str[counter] != '[' && _str[counter] != '\0') {
                    if(_str[counter] != '\r' && _str[counter] != '\n') {                   
                        _section_str[section_str_counter] = _str[counter];
                        section_str_counter++;
                    }
                    counter++;
                }
                return TextFormat("%s", _section_str);
                break;
            }
        }
        counter++;
    }
    
    TraceLog(LOG_WARNING, "lexer::get_section()::Cannot found the section %s ", section);
    return "";
}
const char* get_value_number(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name, data_type type) {
    u16 i = 0;
    u16 delimiter = 0;
    switch (type) {
        case DATA_TYPE_I64:{ delimiter = DATA_TYPE_I64_DELIMITER; break;}
        case DATA_TYPE_U64:{ delimiter = DATA_TYPE_U64_DELIMITER; break;}
        case DATA_TYPE_F64:{ delimiter = DATA_TYPE_F64_DELIMITER; break;}
        case DATA_TYPE_I32:{ delimiter = DATA_TYPE_I32_DELIMITER; break;}
        case DATA_TYPE_U32:{ delimiter = DATA_TYPE_U32_DELIMITER; break;}
        case DATA_TYPE_F32:{ delimiter = DATA_TYPE_F32_DELIMITER; break;}
        case DATA_TYPE_I16:{ delimiter = DATA_TYPE_I16_DELIMITER; break;}
        case DATA_TYPE_U16:{ delimiter = DATA_TYPE_U16_DELIMITER; break;}
        case DATA_TYPE_I8: { delimiter = DATA_TYPE_I8_DELIMITER;  break;}
        case DATA_TYPE_U8: { delimiter = DATA_TYPE_U8_DELIMITER;  break;}
        default: {
            TraceLog(LOG_WARNING, "lexer::get_value_number()::Unsupported type of data");
            return "";
        }
    }

    while(i <= INI_FILE_MAX_SECTION_LENGTH) {
        if (is_letter(&_section[i])) {
            u16 variable_name_counter = 0;
            char arr_val_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
            while (is_variable_allowed(&_section[i])) { 
                arr_val_name[variable_name_counter] = _section[i];
                variable_name_counter++;
                i++;
            }
            if(TextIsEqual(arr_val_name, variable_name)){
                char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
                u16 variable_value_counter = 0;
                move_next_number(_section, &i, true);
                if(i >= INI_FILE_MAX_SECTION_LENGTH) return "";
                while (is_number(&_section[i])) {     
                    if (variable_value_counter > delimiter) {
                        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s's value. Value out of range", variable_name);
                        return TextFormat("%s", value_str);
                    }         
                    value_str[variable_value_counter] = _section[i];
                    variable_value_counter++;
                    i++;
                }
                return TextFormat("%s", value_str);
                break;
            }
        }
        move_next_letter(_section, &i, false);
    }

    TraceLog(LOG_WARNING, "lexer::get_value_number()::Cannot found the variable %s ", variable_name);
    return "";
}
const char* get_value_string(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name) {
    u16 i = 0;
    u16 delimiter = DATA_TYPE_C_DELIMITER;

    while(i <= INI_FILE_MAX_SECTION_LENGTH) {
        if (is_letter(&_section[i])) {
            u16 variable_name_counter = 0;
            char arr_val_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
            while (is_variable_allowed(&_section[i])) { 
                arr_val_name[variable_name_counter] = _section[i];
                variable_name_counter++;
                i++;
            }
            if(TextIsEqual(arr_val_name, variable_name)){
                char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
                u16 variable_value_counter = 0;
                move_next_string(_section, &i, true);
                if(i >= INI_FILE_MAX_SECTION_LENGTH){ 
                    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s has no value", variable_name);
                    return "";
                }
                while (_section[i] != '"') {     
                    if (variable_value_counter > delimiter) {
                        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s's value. Value out of range", variable_name);
                        return TextFormat("%s", value_str);
                    }         
                    value_str[variable_value_counter] = _section[i];
                    variable_value_counter++;
                    i++;
                }
                return TextFormat("%s", value_str);
                break;
            }
        }
        move_next_letter(_section, &i, false);
    }

    TraceLog(LOG_WARNING, "lexer::get_value_number()::Cannot found the variable %s ", variable_name);
    return "";
}
i64 get_variable_I64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I64));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  return str_to_I64(value_str);
}
u64 get_variable_U64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U64));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  if(is_unsigned_number(value_str) == false) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
    return 0;
  }
  return str_to_U64(value_str);
}
f64 get_variable_F64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_F64));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  return str_to_F64(value_str);
}
i32 get_variable_I32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I32));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  return str_to_I32(value_str);
}
u32 get_variable_U32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U32));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  if(is_unsigned_number(value_str) == false) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
    return 0;
  }
  return str_to_U32(value_str);
}
f32 get_variable_F32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_F32));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  return str_to_F32(value_str);
}
i16 get_variable_I16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I16));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  return str_to_I16(value_str);
}
u16 get_variable_U16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U16));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  if(is_unsigned_number(value_str) == false) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
    return 0;
  }
  return str_to_U16(value_str);
}
i8  get_variable_I8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I8));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }

  return str_to_I8(value_str);
}
u8  get_variable_U8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
  char value_str[INI_FILE_MAX_VARIABLE_VALUE_LENGTH] = "";
  TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U8));
  if (TextLength(value_str) == 0) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
    return 0;
  }
  if(is_unsigned_number(value_str) == false) {
    TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
    return 0;
  }
  return str_to_U8(value_str);
}

bool is_alpha(char *c) {
  
  if (*c >= '0' && *c <= '9') {
    return true;
  }
  
  if (*c >= 'A' && *c <= 'Z') {
    return true;
  }
  
  if (*c >= 'a' && *c <= 'z') {
    return true;
  }
  
  return false;
}
bool is_letter(char *c) { 
  if (*c >= 'A' && *c <= 'Z') {
    return true;
  }
  if (*c >= 'a' && *c <= 'z') {
    return true;
  }
  return false;
}
bool is_number(char *c) {
  if (*c >= '0' && *c <= '9') {
    return true;
  }
  else if (*c == '-' && (*(c+1) >= '0' && *(c+1) <= '9')) {
    return true;
  }
  return false;
}
bool is_unsigned_number(char *c) {
  if (*c >= '0' && *c <= '9') {
    return true;
  }
  else if (*c == '-' && (*(c+1) >= '0' && *(c+1) <= '9')) {
    return false;
  }
  return false;
}
bool is_digit(char *c) {
  if (*c >= '0' && *c <= '9') {
    return true;
  }
  return false;
}
bool is_eol(char *c) {
  if (*c >= 10 && *(c+1) <= 13) {
    return true;
  }
  return false;
}
bool is_variable_allowed(char *c) {
  if (is_alpha(c) || *c == '_') return true;

  return false;
}
bool is_string_allowed(char *c) {
  if (is_alpha(c) || *c == '_' || *c == ' ') return true;
  return false;
}

void move_next_alpha(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line) {
  while(_str[(*counter)++] != '\0') {
    if (is_alpha(&_str[*counter])) return;
    if (stay_the_line && is_eol(&_str[*counter])) break;
  }
  *counter = INVALID_ID16;
}
void move_next_letter(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line) {
  while(_str[(*counter)++] != '\0') {
    if (is_letter(&_str[*counter])) return;
    if (stay_the_line && is_eol(&_str[*counter])) break;
  }
  *counter = INVALID_ID16;
}
void move_next_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line) {
  while(_str[(*counter)++] != '\0') {
    if (is_number(&_str[*counter])) return;
    if (stay_the_line && is_eol(&_str[*counter])) break;
  }
  *counter = INVALID_ID16;
}
void move_next_string(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter, bool stay_the_line) {
  while(_str[(*counter)++] != '\0') {
    if (_str[*counter] == '"') {
      ++(*counter);
      return;
    }
    if (stay_the_line && is_eol(&_str[*counter])) break;
  }
  *counter = INVALID_ID16;
}

