
#ifndef PAK_PARSER_H
#define PAK_PARSER_H

#include "defines.h"

void pak_parser_system_initialize(void);

void parse_pak(const char* path);

file_data get_file_data(const char* file_name);

#endif
