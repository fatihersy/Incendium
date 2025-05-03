#include "fstring.h"
#include "stdlib.h"
#include "errno.h"
#include "string.h"

#include "core/fmemory.h"

string_parse_result parse_string(const char* str, const char parser, u16 parse_count, u16 delimiter) {
  string_parse_result result = {};
  u16 string_itr_len = 0;
  u16 actual_count = 0;
  if (!str) {
    return result;
  }

  for (int i = 0; i < parse_count && i < MAX_PARSED_TEXT_ARR_LEN && string_itr_len < delimiter; ++i) {
    int j = 0;
    while (str[string_itr_len] != parser && 
           str[string_itr_len] != '\0' && 
           string_itr_len < delimiter && 
           j < MAX_PARSED_TEXT_TEXT_LEN - 1) {
      result.buffer[i][j++] = str[string_itr_len++];
    }
    result.buffer[i][j] = '\0';
    if (str[string_itr_len] == '\0') {
      actual_count = i + 1;
      break;
    }
    string_itr_len++;
    actual_count = i + 1;
  }
  result.count = actual_count;
  return result;
}

bool append_text(const char* src, char* dest, u16 char_limit) {
  if(strlen(src) + strlen(dest) > char_limit || char_limit == INVALID_ID16) {
    return false;
  }
  copy_memory(dest + strlen(dest), src, strlen(src)); 
  return true;
}

i64 str_to_I64(const char* str){
  i16 value = strtoll(str, NULL, 10);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }

  return value;
}
u64 str_to_U64(const char* str){
  u64 value = strtoull(str, NULL, 10);
  if (errno == ERANGE) {
      errno = 0;
      return 0; 
  }

  return value;
}
f64 str_to_F64(const char* str){
  f64 value = strtod(str, NULL);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
i32 str_to_I32(const char* str){
  i32 value = atoi(str);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
u32 str_to_U32(const char* str){
  u32 value = atoi(str);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
f32 str_to_F32(const char* str){
  f32 value = strtof(str, NULL);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
i16 str_to_I16(const char* str){
  i16 value = atoi(str);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
u16 str_to_U16(const char* str){
  u16 value = atoi(str);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
i8  str_to_I8 (const char* str){
  i8 value = atoi(str);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
u8  str_to_U8 (const char* str){
  u8 value = atoi(str);
  if (errno == ERANGE) {
    errno = 0;
    return 0; 
  }
  return value;
}
