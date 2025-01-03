#include "lexer.h"
#include "stdlib.h"

#include "defines.h"
#include "raylib.h"

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
void get_variable_C (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable, char* out_var[INI_FILE_MAX_VARIABLE_STRING_LENGTH]);

const char* get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section);
const char* get_value_number(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name, data_type type);

bool is_alpha(char *c);
bool is_letter(char *c);
bool is_signed_number(char *c);
bool is_unsigned_number(char *c);
void move_next_alpha(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);
void move_next_letter(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);
void move_next_signed_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);
void move_next_unsigned_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);

bool parse_app_settings_ini(const char* filename, app_settings* out_settings) {

    i32 size = 0;
    char hello_str[INI_FILE_MAX_SECTION_LENGTH] = "";
    char file_str[INI_FILE_MAX_SECTION_LENGTH] = "";
    u8* _str = LoadFileData(filename, &size);
    if (size > INI_FILE_MAX_FILE_SIZE) {
        return false;
    }
    TextCopy(file_str, (char*)_str);
    TextCopy(hello_str, get_section(file_str, "hello"));

    u64 value = get_variable_U64(hello_str, "hello");

    return true;
}

const char* get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section) {
    u16 counter = 0;
    char section_name[INI_FILE_MAX_SECTION_NAME_LENGTH];
    char _section_str[INI_FILE_MAX_SECTION_LENGTH];
    while(_str[counter] != '\0') {
        if (_str[counter] == '[') {
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
                u16 section_str_counter = 0;
                while (_str[counter] != '[' && _str[counter] != '\0') {
                    if(_str[counter] != '\r' && _str[counter] != '\n') {                   
                        _section_str[section_str_counter] = _str[counter];
                        section_str_counter++;
                    }
                    counter++;
                }
                break;
            }
        }
    }

    return TextFormat("%s", _section_str);
}

const char* get_value_number(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name, data_type type) {
    u16 i = 0;
    char arr_val_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_I64_LENGTH] = "";
    u16 delimiter = 0;
    bool is_unsigned = false;
    switch (type) {
        case DATA_TYPE_I64:{ delimiter = 19;                   break;}
        case DATA_TYPE_U64:{ delimiter = 20; is_unsigned=true; break;}
        case DATA_TYPE_F64:{ delimiter = 20;                   break;}
        case DATA_TYPE_I32:{ delimiter = 10;                   break;}
        case DATA_TYPE_U32:{ delimiter = 10; is_unsigned=true; break;}
        case DATA_TYPE_F32:{ delimiter = 20;                   break;}
        case DATA_TYPE_I16:{ delimiter = 5;                    break;}
        case DATA_TYPE_U16:{ delimiter = 5;  is_unsigned=true; break;}
        case DATA_TYPE_I8: { delimiter = 3;                    break;}
        case DATA_TYPE_U8: { delimiter = 3;  is_unsigned=true; break;}
        case DATA_TYPE_C: {
            TraceLog(LOG_WARNING, "WARNING:lexer::get_value_number()::");
            return "";
        }
        default: {
            TraceLog(LOG_WARNING, "WARNING:lexer::get_value_number()::");
            return "";
        }
    }

    while(i <= INI_FILE_MAX_SECTION_LENGTH) {
        if (is_letter(&_section[i])) {
            u16 variable_name_counter = 0;

            while (is_alpha(&_section[i])) { 
                arr_val_name[variable_name_counter] = _section[i];
                variable_name_counter++;
                i++;
            }
            if(TextIsEqual(arr_val_name, variable_name)){
                u16 variable_value_counter = 0;
                if(is_unsigned) move_next_unsigned_number(_section, &i);
                else move_next_signed_number(_section, &i);
                if(i >= INI_FILE_MAX_SECTION_LENGTH) return "";
                while (is_unsigned ? is_unsigned_number(&_section[i]) : is_signed_number(&_section[i])) {     
                    if (variable_value_counter > delimiter) {
                        TraceLog(LOG_WARNING, "WARNING:lexer::get_value_number()::variable:%s ", variable_name);
                        return TextFormat("%s", value_str);
                    }         
                    value_str[variable_value_counter] = _section[i];
                    variable_value_counter++;
                    i++;
                }
                if(TextLength(value_str) > 0) {
                    
                    return TextFormat("%s", value_str);
                }
                else {
                    TraceLog(LOG_WARNING, "WARNING:lexer::get_value_number()::value:%s cannot found", variable_name);
                    return TextFormat("%s", value_str);
                }
                break;
            }
        }
        move_next_letter(_section, &i);
    }

    TraceLog(LOG_WARNING, "WARNING:lexer::get_value_number()::Cannot found the variable %s ", variable_name);
    return "";
}

i64 get_variable_I64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_I64_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_I64));

    if (TextLength(value) > 0) {
        return strtoll(value, NULL, 10);
    }

    return I64_MAX;
}
u64 get_variable_U64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_U64_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_U64));

    if (TextLength(value) > 0) {
        return strtoull(value, NULL, 10);
    }

    return U64_MAX;
}
f64 get_variable_F64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_F64_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_F64));

    if (TextLength(value) > 0) {
        return strtod(value, NULL);
    }

    return F64_MAX;
}
i32 get_variable_I32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_I32_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_I32));

    if (TextLength(value) > 0) {
        return atoi(value);
    }

    return I32_MAX;
}
u32 get_variable_U32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_U32_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_U32));

    if (TextLength(value) > 0) {
        return strtoul(value, NULL, 10);
    }

    return U32_MAX;
}
f32 get_variable_F32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_F32_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_F32));

    if (TextLength(value) > 0) {
        return strtof(value, NULL);
    }

    return F32_MAX;
}
i16 get_variable_I16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_I16_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_I16));

    if (TextLength(value) > 0) {
        return strtol(value, NULL,10);
    }

    return I16_MAX;
}
u16 get_variable_U16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_U16_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_U16));

    if (TextLength(value) > 0) {
        return strtoul(value, NULL,10);
    }

    return U16_MAX;
}
i8  get_variable_I8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_I8_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_I8));

    if (TextLength(value) > 0) {
        return strtol(value, NULL,10);
    }

    return I8_MAX;
}
u8  get_variable_U8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_U8_LENGTH] = "";

    TextCopy(value, get_value_number(_section, variable, DATA_TYPE_U8));

    if (TextLength(value) > 0) {
        return strtoul(value, NULL,10);
    }

    return U8_MAX;
}
void get_variable_C (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable, char* out_var[INI_FILE_MAX_VARIABLE_STRING_LENGTH]) {

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
bool is_signed_number(char *c) {
    
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

    return false;
}



void move_next_alpha(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_alpha(&_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
void move_next_letter(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_letter(&_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
void move_next_signed_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_signed_number(&_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
void move_next_unsigned_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if(_str[*counter] == '-') {
            *counter = INVALID_ID16;
            return;
        }
        if(is_unsigned_number(&_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
