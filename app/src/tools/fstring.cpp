#include "fstring.h"
#include "stdlib.h"
#include "errno.h"

#include "core/fmemory.h"

const char* stringtify_data(data_pack data, const char* parser, u16 character_limit) {
  u16 length = data.array_lenght;
  char _temp[MAX_SLIDER_OPTION_TEXT_SLOT+1] = "";

  switch (data.type_flag) {
    case DATA_TYPE_I32: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", data.data.i32[i]);
          if(!append_text(c, _temp, character_limit)) break;
          if(!append_text(parser, _temp, character_limit)) break;
        } 
        else { // Last
          const char* c = TextFormat("%u", data.data.i32[i]);
          append_text(c, _temp, character_limit);
        }
      }
      break;
    }
    case DATA_TYPE_U64: { 
      const char* c = TextFormat("%u", data.data.u64);
      append_text(c, _temp, character_limit);
      break;
    }
    case DATA_TYPE_U32: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", data.data.u32[i]);
          if(!append_text(c, _temp, character_limit)) break;
          if(!append_text(parser, _temp, character_limit)) break;
        } 
        else { // Last
          const char* c = TextFormat("%u", data.data.u32[i]);
          append_text(c, _temp, character_limit);
        }
      }
      break;
    }
    case DATA_TYPE_U16: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", data.data.u16[i]);
          if(!append_text(c, _temp, character_limit)) break;
          if(!append_text(parser, _temp, character_limit)) break;
        } 
        else { // Last
          const char* c = TextFormat("%u", data.data.u16[i]);
          append_text(c, _temp, character_limit);
        }
      }
      break;
    }
    case DATA_TYPE_F32: { 
      for (int i=0; i<length; ++i) {
        if(i != length-1) { // Unless the last
          const char* c = TextFormat("%u", data.data.f32[i]);
          if(!append_text(c, _temp, character_limit)) break;
          if(!append_text(parser, _temp, character_limit)) break;
        } 
        else { // Last
          const char* c = TextFormat("%u", data.data.f32[i]);
          append_text(c, _temp, character_limit);
        }
      }
      break;
    }
    case DATA_TYPE_C: { 
      append_text(data.data.c, _temp, character_limit);
      break;
    }
    default: TraceLog(
      LOG_WARNING, "WARNING::user_interface::stringtify_options()::Unsupported data type");
  }
  const char* text = _temp;
  return text;
}

string_parse_result parse_string(const char* str, const char parser, u16 parse_count, u16 delimiter) {
  string_parse_result result = {};
  u16 string_itr_len = 0;
  u16 actual_count = 0;
  if (!str) {
    TraceLog(LOG_ERROR, "parse_string: Received NULL string pointer");
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
  if(TextLength(src) + TextLength(dest) > char_limit || char_limit == INVALID_ID16) {
    TraceLog(LOG_WARNING, 
    "WARNING::user_interface::stringtify_options()::Array length exceed. ArrLen:%d, Needed:%d", 
        MAX_SLIDER_OPTION_TEXT_SLOT, TextLength(src) + TextLength(dest));
    return false;
  }
  copy_memory(dest + TextLength(dest), src, TextLength(src)); 
  return true;
}

i64 str_to_I64(const char* str){
  i16 value = strtoll(str, NULL, 10);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_I64()::For i64 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }

  return value;
}
u64 str_to_U64(const char* str){
  u64 value = strtoull(str, NULL, 10);
  if (errno == ERANGE) {
      TraceLog(LOG_WARNING, "fstring::str_to_U64()::For u64 string:%s value out of range", str);
      errno = 0;
      return 0; 
  }

  return value;
}
f64 str_to_F64(const char* str){
  f64 value = strtod(str, NULL);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_F64()::For f64 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
i32 str_to_I32(const char* str){
  i32 value = atoi(str);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_I32()::For i32 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
u32 str_to_U32(const char* str){
  u32 value = atoi(str);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_U32()::For u32 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
f32 str_to_F32(const char* str){
  f32 value = strtof(str, NULL);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_U32()::For f32 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
i16 str_to_I16(const char* str){
  i16 value = atoi(str);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_I16()::For i16 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
u16 str_to_U16(const char* str){
  u16 value = atoi(str);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_U16()::For u16 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
i8  str_to_I8 (const char* str){
  i8 value = atoi(str);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_I8()::For i8 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
u8  str_to_U8 (const char* str){
  u8 value = atoi(str);
  if (errno == ERANGE) {
    TraceLog(LOG_WARNING, "fstring::str_to_U8()::For u8 string:%s value out of range", str);
    errno = 0;
    return 0; 
  }
  return value;
}
