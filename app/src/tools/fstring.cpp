#include "fstring.h"
#include "stdlib.h"
#include "errno.h"
#include "string.h"

#include "core/fmemory.h"

string_parse_result parse_string(std::string str, const char parser, u16 delimiter) {
  string_parse_result result = string_parse_result();
  std::string::size_type string_in_readed = 0;

  for (size_t iter = 0; iter < delimiter && iter < str.size(); ++iter) {
    std::string::size_type found_at = str.find(parser, string_in_readed);
    if (std::string::npos == found_at || found_at == 0) {
      break;
    }

    result.buffer.push_back(str.substr(string_in_readed, found_at - string_in_readed));
    
    string_in_readed = found_at+1;
  }

  return result;
}

bool append_text(const char* src, char* dest, u16 char_limit) {
  if(strlen(src) + strlen(dest) > char_limit || char_limit == INVALID_IDU16) {
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
