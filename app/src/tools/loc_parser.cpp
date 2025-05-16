#include "loc_parser.h"

#include "raylib.h"

#include "core/fmemory.h"
#include "loc_types.h"

#define SYMBOL_DIGITS_START_SYMBOL 0x21
#define LOC_FILE_TEXT_SYMBOL_LENGTH 3
#define LOC_FILE_PATH_PREFIX "./loc/"
#define LOC_FILE_EXTENSION ".txt"
#define loc_content_map std::vector<std::string>
#define loc_content_symbol std::string
#define loc_content_text std::string

typedef enum loc_reading_order {
  LOC_READING_ORDER_UNDEFINED,
  LOC_READING_ORDER_SYMBOL,
  LOC_READING_ORDER_CONTENT,
  LOC_READING_ORDER_MAX,
} loc_reading_order;

typedef struct loc_data {
  std::vector<std::string> content;
}loc_data;

typedef struct loc_parser_system_state {
  std::vector<u8> file_data;
  u32 loc_data_size;

  loc_data lang_data;

  std::pair<std::string, std::string> US_en;
  std::pair<std::string, std::string> TR_tr;
}loc_parser_system_state;

static loc_parser_system_state * state;

loc_content_text loc_parser_get_text(u32& offset);
loc_content_symbol loc_parser_read_symbol(u32& offset);
bool is_symbol_allowed(u8& c);

bool loc_parser_system_initialize(void) {
  if (state) {
    TraceLog(LOG_WARNING, "loc_parser::loc_parser_system_initialize()::Init function called twice");
    return false;
  }
  state = (loc_parser_system_state*)allocate_memory_linear(sizeof(loc_parser_system_state), true);
  if (!state) {
    TraceLog(LOG_WARNING, "loc_parser::loc_parser_system_initialize()::loc parser state allocation failed");
    return false;
  }
  state->US_en.first  = "English";
  state->US_en.second = "US_en";

  state->TR_tr.first =  "Turkish";
  state->TR_tr.second = "TR_tr";

  return true;
}

bool _loc_parser_parse_localization_data_from_file(const char* file_name) {
  return loc_parser_parse_localization_data_from_file(file_name);
}

bool loc_parser_parse_localization_data_from_file(const char* file_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  u16 file_name_len = TextLength(file_name);

  if (file_name_len == 0 || file_name_len > 5) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::File name lenght out of bound");
    return false;
  }
  const char* file_path = TextFormat("%s%s%s", LOC_FILE_PATH_PREFIX, file_name, LOC_FILE_EXTENSION);
  {
    int dataSize = 1;
    u8* _str_tile = LoadFileData(file_path, &dataSize);
    if (dataSize <= 0 || dataSize == I32_MAX) {
      TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::Reading data returned null");
      return false;
    }
    state->loc_data_size = dataSize;
    state->file_data.assign(_str_tile, _str_tile + dataSize);
    UnloadFileData(_str_tile);
  }

  loc_reading_order reading_order = static_cast<loc_reading_order>(LOC_READING_ORDER_UNDEFINED+1);

  loc_content_symbol symbol = {};
  loc_content_text text = {};
  loc_content_map& content_map = state->lang_data.content;
  content_map.clear();
  for (u32 i=0; i < state->loc_data_size;) {
    switch (reading_order) {
      case LOC_READING_ORDER_SYMBOL: {
        symbol = loc_parser_read_symbol(i);
        break;
      }
      case LOC_READING_ORDER_CONTENT: {
        text = loc_parser_get_text(i);
        content_map.push_back(text);
        break;
      }
      default: {
        TraceLog(LOG_ERROR, "loc_parser::parse_loc()::Unsupported reading order stage");
        return false;
      }
    }
    
    reading_order = static_cast<loc_reading_order>((reading_order % (loc_reading_order::LOC_READING_ORDER_MAX)) + 1);
    if (reading_order == loc_reading_order::LOC_READING_ORDER_MAX) {
      reading_order = static_cast<loc_reading_order>((reading_order % (loc_reading_order::LOC_READING_ORDER_MAX)) + 1);
    }
  }

  return true;
}

loc_content_text loc_parser_get_text(u32& offset) {
  loc_content_text text = {};

  bool quote_found = false;
  for (; offset<state->loc_data_size; ++offset) {
    if (!quote_found) {
      if (state->file_data.at(offset) == '"') {
        quote_found = true;
      }
      continue;
    } 
    if (state->file_data.at(offset) == '"') {
      offset++;
      return text; 
    }

    text.push_back(state->file_data.at(offset));
  }

  return text;
}

loc_content_symbol loc_parser_read_symbol(u32& offset) {
  loc_content_symbol symbol = {};
  char parser = '=';

  for (; offset<state->loc_data_size; ++offset) {
    if(is_symbol_allowed(state->file_data.at(offset))) {
      if (symbol.size() < LOC_TEXT_SYMBOL_SIZE) {
        symbol.push_back(state->file_data.at(offset));
      }
      continue;
    }
    else if(state->file_data.at(offset) == parser) {
      offset++;
      return symbol;
    }
  }

  return symbol;
}

const char* loc_lang_get_file_name(supported_language lang) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_lang_display_name_to_file_name()::State is not valid");
    return "";
  }
  std::string display_name;
  switch (lang) {
    case supported_language::LANGUAGE_ENGLISH: {
      return state->US_en.second.c_str();
    }
    case supported_language::LANGUAGE_TURKISH: {
      return state->TR_tr.second.c_str();
    }
    default: {
      TraceLog(LOG_ERROR, "loc_parser::loc_lang_get_file_name()::Unsupported language");
      return "";
    }
  }

  TraceLog(LOG_WARNING, "loc_parser::loc_lang_get_file_name()::Function terminated unexpectedly");
  return "";
}
supported_language loc_lang_file_name_to_enum(const char* file_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_lang_file_name_to_enum()::State is not valid");
    return supported_language::LANGUAGE_UNSPECIFIED;
  }
  std::string display_name;
  if (TextIsEqual(file_name, state->US_en.second.c_str())) { return supported_language::LANGUAGE_ENGLISH; }
  else if (TextIsEqual(file_name, state->TR_tr.second.c_str())) { return supported_language::LANGUAGE_TURKISH; }
  else {
    TraceLog(LOG_ERROR, "loc_parser::loc_lang_file_name_to_enum()::Unsupported language");
    return supported_language::LANGUAGE_UNSPECIFIED;
  }

  TraceLog(LOG_WARNING, "loc_parser::loc_lang_file_name_to_enum()::Function terminated unexpectedly");
  return supported_language::LANGUAGE_UNSPECIFIED;
}

const char* lc_txt(u32 index) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::lc_txt()::State is not valid");
    return "";
  }
  if (index > state->lang_data.content.size()) {
    TraceLog(LOG_ERROR, "loc_parser::lc_txt()::Out of range");
    return "";
  }

  return state->lang_data.content.at(index).c_str();
}

u32 symbol_to_index(const char* symbol) {
  u32 first_digit  = static_cast<u32>(symbol[2] - SYMBOL_DIGITS_START_SYMBOL) * 1;
  u32 second_digit = static_cast<u32>(symbol[1] - SYMBOL_DIGITS_START_SYMBOL) * 10;
  u32 third_digit  = static_cast<u32>(symbol[0] - SYMBOL_DIGITS_START_SYMBOL) * 100;

  return first_digit + second_digit + third_digit;
}

bool is_symbol_allowed(u8& c) {
  if (c >= '!' && c <= '*') {
    return true;
  }
  return false;
}

#undef loc_content_map
#undef loc_content_symbol
#undef loc_content_text

