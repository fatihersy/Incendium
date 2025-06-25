
#ifndef FSTRING_H
#define FSTRING_H

#include "defines.h"

typedef struct string_parse_result {
  std::vector<std::string> buffer;
  string_parse_result(void) {
    this->buffer.clear();
  }
} string_parse_result;

i64 str_to_I64(const char* str);
u64 str_to_U64(const char* str);
f64 str_to_F64(const char* str);
i32 str_to_I32(const char* str);
u32 str_to_U32(const char* str);
f32 str_to_F32(const char* str);
i16 str_to_I16(const char* str);
u16 str_to_U16(const char* str);
i8  str_to_I8 (const char* str);
u8  str_to_U8 (const char* str);

string_parse_result parse_string(std::string str, const char parser, u16 delimiter);

bool append_text(const char* src, char* dest, u16 char_limit);

#endif
