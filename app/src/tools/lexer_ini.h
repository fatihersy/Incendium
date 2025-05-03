#ifndef LEXER_H
#define LEXER_H

#include "defines.h"

#define INI_FILE_MAX_FILE_SIZE 32000
#define INI_FILE_MAX_SECTION_NAME_LENGTH 32
#define INI_FILE_MAX_SECTION_LENGTH 512
#define INI_FILE_MAX_VARIABLE_NAME_LENGTH 32
#define INI_FILE_MAX_VARIABLE_VALUE_LENGTH 32

bool parse_app_settings_ini(const char* filename, app_settings* out_settings);

#endif
