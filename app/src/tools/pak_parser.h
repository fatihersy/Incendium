
#ifndef PAK_PARSER_H
#define PAK_PARSER_H

#include "defines.h"

void pak_parser_system_initialize(const char* path);

void parse_pak(void);

file_data get_file_data(const char* file_name);

file_data fetch_file_data(const char* file_name);

#endif
