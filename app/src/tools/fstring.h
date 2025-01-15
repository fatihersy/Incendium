
#ifndef FSTRING_H
#define FSTRING_H

#include "defines.h"

const char* stringtify_data(data_pack data, const char* parser, u16 character_limit);
bool append_text(const char* src, char* dest, u16 char_limit);

#endif