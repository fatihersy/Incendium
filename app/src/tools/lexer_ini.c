#include "lexer.h"
#include "stdlib.h"
#include "errno.h"

#include "defines.h"
#include "raylib.h"

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
void get_variable_C (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable, char* out_var[INI_FILE_MAX_VARIABLE_STRING_LENGTH]);

const char* get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section);
const char* get_value_number(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable_name, data_type type);

bool is_alpha(char *c);
bool is_letter(char *c);
bool is_digit(char *c);
bool is_number(char *c);
bool is_unsigned_number(char *c);
bool is_variable_allowed(char *c);
void move_next_alpha(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);
void move_next_letter(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);
void move_next_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter);

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

    u64 value_u64 = get_variable_U64(hello_str, "value_u64");
    i64 value_i64 = get_variable_I64(hello_str, "value_i64");
    f64 value_f64 = get_variable_F64(hello_str, "value_f64");
    u32 value_u32 = get_variable_U32(hello_str, "value_u32");
    i32 value_i32 = get_variable_I32(hello_str, "value_i32");
    f32 value_f32 = get_variable_F32(hello_str, "value_f32");
    u16 value_u16 = get_variable_U16(hello_str, "value_u16");
    i16 value_i16 = get_variable_I16(hello_str, "value_i16");
    u8  value_u8  = get_variable_U8 (hello_str, "value_u8");
    i8  value_i8  = get_variable_I8 (hello_str, "value_i8");

    return true;
}

const char* get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section) {
    u16 counter = 0;
    char section_name[INI_FILE_MAX_SECTION_NAME_LENGTH] = "";
    char _section_str[INI_FILE_MAX_SECTION_LENGTH] = "";
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
        counter++;
    }

    return TextFormat("%s", _section_str);
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
                char value_str[INI_FILE_MAX_VARIABLE_I64_LENGTH] = "";
                u16 variable_value_counter = 0;
                move_next_number(_section, &i);
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
        move_next_letter(_section, &i);
    }

    TraceLog(LOG_WARNING, "lexer::get_value_number()::Cannot found the variable %s ", variable_name);
    return "";
}

i64 get_variable_I64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_I64_LENGTH] = "";
    i64 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I64));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    value = strtoll(value_str, NULL, 10);
    if (errno == ERANGE) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For i64 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
u64 get_variable_U64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_U64_LENGTH] = "";
    u64 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U64));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    if(is_unsigned_number(value_str) == false) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
        return 0;
    }
    value = strtoull(value_str, NULL, 10);
    if (errno == ERANGE) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For u64 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
f64 get_variable_F64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_F64_LENGTH] = "";
    f64 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_F64));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    value = strtod(value_str, NULL);
    if (errno == ERANGE) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For f64 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
i32 get_variable_I32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_I32_LENGTH] = "";
    i32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I32));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    value = atoi(value_str);
    if (errno == ERANGE) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For i32 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
u32 get_variable_U32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_U32_LENGTH] = "";
    u32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U32));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    if(is_unsigned_number(value_str) == false) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
        return 0;
    }
    value = atoi(value_str);
    if (errno == ERANGE) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For u32 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
f32 get_variable_F32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_F32_LENGTH] = "";
    f32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_F32));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    value = strtof(value_str, NULL);
    if (errno == ERANGE) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For f32 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
i16 get_variable_I16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_I16_LENGTH] = "";
    i32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I16));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    value = atoi(value_str);
    if (errno == ERANGE || value > I16_MAX) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For i16 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
u16 get_variable_U16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_U16_LENGTH] = "";
    u32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U16));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    if(is_unsigned_number(value_str) == false) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
        return 0;
    }
    value = atoi(value_str);
    if (errno == ERANGE || value > U16_MAX) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For u16 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
i8  get_variable_I8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_I8_LENGTH] = "";
    i32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_I8));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    value = atoi(value_str);
    if (errno == ERANGE || value > I8_MAX) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For i8 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
}
u8  get_variable_U8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value_str[INI_FILE_MAX_VARIABLE_U8_LENGTH] = "";
    u32 value = 0;
    TextCopy(value_str, get_value_number(_section, variable, DATA_TYPE_U8));
    if (TextLength(value_str) == 0) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For variable:%s no value recieved", variable);
        return 0;
    }
    if(is_unsigned_number(value_str) == false) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For unsigned variable:%s value was minus 0", variable);
        return 0;
    }
    value = atoi(value_str);
    if (errno == ERANGE || value > U8_MAX) {
        TraceLog(LOG_WARNING, "lexer::get_value_number()::For u8 variable:%s value out of range", variable);
        errno = 0;
        return 0; 
    }

    return value;
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
bool is_variable_allowed(char *c) {
    
    if (is_alpha(c)) return true;
    if (*c == '_') return true;

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
void move_next_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_number(&_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}

