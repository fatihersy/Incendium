#include "fstring.h"

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
