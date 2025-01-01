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
void get_section     (char _str[INI_FILE_MAX_FILE_SIZE], const char* section,  char section_str[INI_FILE_MAX_SECTION_LENGTH]);

bool is_alpha(char c);
bool is_letter(char c);
bool is_number(char c);
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

    get_section(file_str, "hello", hello_str);

    i64 value = get_variable_I64(hello_str, "hello");

    return true;
}

void get_section(char _str[INI_FILE_MAX_FILE_SIZE], const char* section, char section_str[INI_FILE_MAX_SECTION_LENGTH]) {
    u16 counter = 0;
    char section_name[INI_FILE_MAX_SECTION_NAME_LENGTH];
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
                        section_str[section_str_counter] = _str[counter];
                        section_str_counter++;
                    }
                    counter++;
                }
                break;
            }
        }
    }
}

i64 get_variable_I64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {
    u16 i = 0;
    char variable_name[INI_FILE_MAX_VARIABLE_NAME_LENGTH] = "";
    char value[INI_FILE_MAX_VARIABLE_I64_LENGTH] = "";
    bool found = false;

    while( i != INVALID_ID16 || found == false) {
        if (is_letter(_section[i])) {
            u16 variable_name_counter = 0;

            while (is_alpha(_section[i])) { 
                variable_name[variable_name_counter] = _section[i];
                variable_name_counter++;
                i++;
            }
            if(TextIsEqual(variable_name, variable)){
                u16 variable_value_counter = 0;
                move_next_number(_section, &i);

                while (is_number(_section[i])) {                 
                    value[variable_value_counter] = _section[i];
                    variable_value_counter++;
                    i++;
                }
                found = true;
                break;
            }
        }
        move_next_letter(_section, &i);
    }
    if (found) {
        return strtoll(value, NULL, 10);
    }

    return 0;
}
u64 get_variable_U64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
f64 get_variable_F64(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
i32 get_variable_I32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
u32 get_variable_U32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
f32 get_variable_F32(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
i16 get_variable_I16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
u16 get_variable_U16(char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
i8  get_variable_I8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
u8  get_variable_U8 (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable) {

    return 0;
}
void get_variable_C (char _section[INI_FILE_MAX_SECTION_LENGTH], const char* variable, char* out_var[INI_FILE_MAX_VARIABLE_STRING_LENGTH]) {

}

bool is_alpha(char c) {
    
    if (c >= '0' && c <= '9') {
        return 1;
    }
   
    if (c >= 'A' && c <= 'Z') {
        return 1;
    }
   
    if (c >= 'a' && c <= 'z') {
        return 1;
    }
   
    return 0;
}
bool is_letter(char c) {
    
    if (c >= 'A' && c <= 'Z') {
        return 1;
    }
   
    if (c >= 'a' && c <= 'z') {
        return 1;
    }
   
    return 0;
}
bool is_number(char c) {
    
    if (c >= '0' && c <= '9') {
        return 1;
    }
     
    return 0;
}


void move_next_alpha(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_alpha(_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
void move_next_letter(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_letter(_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
void move_next_number(char _str[INI_FILE_MAX_FILE_SIZE], u16* counter) {
    while(_str[(*counter)++] != '\0') {
        if (is_number(_str[*counter])) return;
    }
    *counter = INVALID_ID16;
}
