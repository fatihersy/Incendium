#include "loc_parser.h"

#include "raylib.h"

#include "core/fmemory.h"
#include "loc_types.h"

const char * default_language = R"(
{
  display_text = "English",
  codepoints = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
  translates = {\
    !!! = "Play"
    !!" = "Upgrade"
    !!# = "Settings"
    !!$ = "Exit"
  }
}
)";

#define LOC_MAX_CODEPOINTS 128
#define SYMBOL_DIGITS_START_SYMBOL 0x21
#define LOC_FILE_TEXT_SYMBOL_LENGTH 3
#define LOC_FILE_PATH_PREFIX "./loc/"
#define LOC_FILE_EXTENSION "._loc_data"
#define loc_content_map std::vector<std::string>
#define loc_content_symbol std::string
#define loc_content_text std::string
#define loc_content_lang_name std::string
#define loc_content_codepoints std::string

#define LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME "display_text"
#define LOC_FILE_CODEPOINTS_VARIABLE_NAME "codepoints"
#define LOC_FILE_MAP_VARIABLE_NAME "translates"

#define LOC_FILE_SYMBOL_VARIABLE_INITIALIZATION '='
#define LOC_FILE_SYMBOL_STATEMENT_PARSER ','
#define LOC_FILE_SYMBOL_SCOPE_START '{'
#define LOC_FILE_SYMBOL_SCOPE_END '}'

typedef struct loc_file_scope {
  u32 scope_start_offset;
  u32 scope_end_offset;
} loc_file_scope;

typedef enum loc_reading_order {
  LOC_READING_ORDER_UNDEFINED,
  LOC_READING_ORDER_SYMBOL,
  LOC_READING_ORDER_CONTENT,
  LOC_READING_ORDER_MAX,
} loc_reading_order;

typedef struct loc_parser_system_state {
  std::vector<loc_data> lang_data;
  loc_data * active_loc;
  
  loc_data _buffer;
  std::vector<u8> file_buffer;
  u32 file_size;
}loc_parser_system_state;

static loc_parser_system_state * state;

loc_content_text loc_parser_get_next_content_text(u32& offset);
loc_content_symbol loc_parser_read_symbol(u32& offset);
loc_content_lang_name loc_parser_read_language_name(void);
loc_content_codepoints loc_parser_read_codepoints(void);
loc_content_map loc_parser_read_map(void);

u32 loc_parser_go_to_variable(std::string variable_name);
void loc_parser_read_content(void);
std::string loc_parser_get_text(u32& offset, std::string text);
std::string loc_parser_get_next_text(u32& offset);
u32 loc_parser_go_to_next_statement(u32 offset);
loc_file_scope loc_parser_get_scope_range(u32 offset);
bool is_symbol_allowed(u8& c);
bool is_variable_allowed(u8& c);

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

  return true;
}

bool _loc_parser_parse_localization_data_from_file(const char* file_name) {
  return loc_parser_parse_localization_data_from_file(file_name);
}
bool _loc_parser_parse_localization_data(void) {
  return loc_parser_parse_localization_data();
}

bool loc_parser_parse_localization_data_from_file(const char* file_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  u16 file_name_len = TextLength(file_name);

  if (file_name_len <= 0 || file_name_len >= 64) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::File name lenght out of bound");
    return false;
  }
  const char* file_path = TextFormat("%s%s%s", LOC_FILE_PATH_PREFIX, file_name, LOC_FILE_EXTENSION);
  loc_data data = {};
  state->file_size = 0;
  state->file_buffer.clear();
  state->_buffer = {};
  {
    int dataSize = 1;
    u8* _str_tile = LoadFileData(file_path, &dataSize);
    if (dataSize <= 0 || dataSize == I32_MAX) {
      TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::Reading data returned null");
      return false;
    }
    state->file_size = dataSize;
    state->file_buffer.assign(_str_tile, _str_tile + dataSize);
    UnloadFileData(_str_tile);
  }

  data.language_name = loc_parser_read_language_name();
  data.codepoints = loc_parser_read_codepoints();
  data.content = loc_parser_read_map();

  state->lang_data.push_back(data);
  return true;
}

bool loc_parser_parse_localization_data(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  FilePathList file_names = LoadDirectoryFilesEx(LOC_FILE_PATH_PREFIX, LOC_FILE_EXTENSION, false);

  if (file_names.count <= 0 || file_names.count >= 10) { // TODO: Hardcoded upper language limit
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::No localization file loaded or too much");
    return false;
  }

  state->lang_data.clear();
  state->file_size = 0;
  state->file_buffer.clear();
  state->_buffer = {};
  for (size_t iter = 0; iter < file_names.count; iter++) {
    loc_data data = {};
    {
      int dataSize = 1;
      u8* _str_tile = LoadFileData(file_names.paths[iter], &dataSize);
      if (dataSize <= 0 || dataSize == I32_MAX) {
        TraceLog(LOG_ERROR, "loc_parser::loc_parser_parse_localization_data_from_file()::Reading data returned null");
        return false;
      }
      state->file_size = dataSize;
      state->file_buffer.assign(_str_tile, _str_tile + dataSize);
      UnloadFileData(_str_tile);
    }
    data.language_name = loc_parser_read_language_name();
    data.codepoints = loc_parser_read_codepoints();
    data.content = loc_parser_read_map();
    data.index = state->lang_data.size();
    state->lang_data.push_back(data);
    TraceLog(LOG_INFO, "loc_parser::loc_parser_parse_localization_data()::Language:%s installed ", data.language_name.c_str());
  }
  if (state->lang_data.size() == 0) {
    size_t def_lang_len = TextLength(default_language);
    state->file_buffer.clear();
    state->file_buffer.assign(default_language, default_language + def_lang_len);
    state->file_size = def_lang_len;

    loc_data data = {};
    data.language_name = loc_parser_read_language_name();
    data.codepoints = loc_parser_read_codepoints();
    data.content = loc_parser_read_map();
    data.index = state->lang_data.size();
    state->lang_data.push_back(data);
  }
  UnloadDirectoryFiles(file_names);
  return true;
}

loc_content_text loc_parser_get_next_content_text(u32& offset) {
  loc_content_text text = {};

  bool quote_found = false;
  for (; offset<state->file_size; ++offset) {
    if (!quote_found) {
      if (state->file_buffer.at(offset) == '"') {
        quote_found = true;
      }
      continue;
    } 
    if (state->file_buffer.at(offset) == '"') {
      offset++;
      return text; 
    }
    text.push_back(state->file_buffer.at(offset));
  }
  return text;
}
std::string loc_parser_get_text(u32& offset, std::string text) {
  std::string _text = {};

  bool quote_found = false;
  for (; offset<state->file_size; ++offset) {
    if (!quote_found) {
      if (state->file_buffer.at(offset) == '"') {
        quote_found = true;
      }
      continue;
    } 
    if (state->file_buffer.at(offset) == '"') {
      offset++;
      return _text; 
    }
    _text.push_back(state->file_buffer.at(offset));
  }

  if(TextIsEqual(_text.c_str(), text.c_str())) {
    return _text;
  }
  else return "";
}
std::string loc_parser_get_next_text(u32& offset) {
  std::string text = {};

  bool quote_found = false;
  for (; offset<state->file_size; ++offset) {
    if (!quote_found) {
      if (state->file_buffer.at(offset) == '"') {
        quote_found = true;
      }
      continue;
    } 
    if (state->file_buffer.at(offset) == '"') {
      offset++;
      return text; 
    }
    text.push_back(state->file_buffer.at(offset));
  }
  return text;
}
u32 loc_parser_go_to_next_statement(u32 offset) {
  
  for (; offset < state->file_size; ++offset) {
    if(state->file_buffer.at(offset) == LOC_FILE_SYMBOL_STATEMENT_PARSER) {
      return offset;
    }
  }

  return U32_MAX;
}
loc_file_scope loc_parser_get_scope_range(u32 offset) {
  loc_file_scope _scope = {
    .scope_start_offset = U32_MAX,
    .scope_end_offset = U32_MAX
  };
  bool scope_start_found = false;
  
  for (; offset < state->file_size; ++offset) {
    if(!scope_start_found && state->file_buffer.at(offset) == LOC_FILE_SYMBOL_SCOPE_START) {
      scope_start_found = true;
      _scope.scope_start_offset = offset;
    }
    else if (scope_start_found && state->file_buffer.at(offset) == LOC_FILE_SYMBOL_SCOPE_END) {
      _scope.scope_end_offset = offset;
      return _scope;
    }
  }

  return _scope;
}

u32 loc_parser_go_to_variable(std::string variable_name) {
  std::string variable = {};
  bool variable_found = false;
  bool in_variable_try = false;
  u32 variable_search_offset = 0;
  do {
    variable.clear();
    in_variable_try = false;
    for (; variable_search_offset < state->file_size; ++variable_search_offset) 
    {
      if( !variable_found && is_variable_allowed(state->file_buffer.at(variable_search_offset))) {
        if (variable.size() < LOC_TEXT_VARIABLE_SIZE) {
          variable.push_back(state->file_buffer.at(variable_search_offset));
          in_variable_try = true;
        }
      }
      else if(in_variable_try) {
        if (TextIsEqual(variable.c_str(), variable_name.c_str())) {
          variable_found = true;
          in_variable_try = false;
        }
        else {
          variable_search_offset = loc_parser_go_to_next_statement(variable_search_offset);
          break;
        }
      }
      else if(variable_found && state->file_buffer.at(variable_search_offset) == LOC_FILE_SYMBOL_VARIABLE_INITIALIZATION) {
        return variable_search_offset;
      }
    }
  } while(variable_search_offset < state->file_size && !variable_found);

  if (variable_found) {
    return variable_search_offset;
  }
  else return U32_MAX;
}

loc_content_symbol loc_parser_read_symbol(u32& offset) {
  loc_content_symbol symbol = {};
  char parser = '=';

  for (; offset<state->file_size; ++offset) {
    if(is_symbol_allowed(state->file_buffer.at(offset))) {
      if (symbol.size() < LOC_TEXT_SYMBOL_SIZE) {
        symbol.push_back(state->file_buffer.at(offset));
      }
      continue;
    }
    else if(state->file_buffer.at(offset) == parser) {
      offset++;
      return symbol;
    }
  }

  return symbol;
}
loc_content_lang_name loc_parser_read_language_name(void) {
  u32 offset = loc_parser_go_to_variable(LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME);

  if (offset < state->file_size) {
    return loc_parser_get_next_text(offset);
  }
  else return "";
}
loc_content_codepoints loc_parser_read_codepoints(void) {
  u32 offset = loc_parser_go_to_variable(LOC_FILE_CODEPOINTS_VARIABLE_NAME);

  if (offset < state->file_size) {
    return loc_parser_get_next_text(offset);
  }
  else return "";
}
loc_content_map loc_parser_read_map(void) {

  u32 offset = loc_parser_go_to_variable(LOC_FILE_MAP_VARIABLE_NAME);

  loc_file_scope scope = loc_parser_get_scope_range(offset);
  offset = scope.scope_start_offset;

  if (scope.scope_start_offset > state->file_size) {
    return {};
  }

  loc_reading_order reading_order = static_cast<loc_reading_order>(LOC_READING_ORDER_UNDEFINED+1);

  loc_content_symbol symbol = {};
  loc_content_text text = {};
  loc_content_map content_map = {};

  for (u32 offset=scope.scope_start_offset; offset < scope.scope_end_offset;) {
    switch (reading_order) {
      case LOC_READING_ORDER_SYMBOL: {
        symbol = loc_parser_read_symbol(offset);
        break;
      }
      case LOC_READING_ORDER_CONTENT: {
        text = loc_parser_get_next_content_text(offset);
        content_map.push_back(text);
        break;
      }
      default: {
        TraceLog(LOG_ERROR, "loc_parser::loc_parser_read_map()::Unsupported reading order stage");
        return content_map;
      }
    }
    
    reading_order = static_cast<loc_reading_order>((reading_order % (loc_reading_order::LOC_READING_ORDER_MAX)) + 1);
    if (reading_order == loc_reading_order::LOC_READING_ORDER_MAX) {
      reading_order = static_cast<loc_reading_order>((reading_order % (loc_reading_order::LOC_READING_ORDER_MAX)) + 1);
    }
  }

  return content_map;
}


localized_languages loc_parser_get_loc_langs(void) {
  return localized_languages { .lang = state->lang_data };
}
bool loc_parser_set_active_language_by_name(std::string language_name) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_set_active_language_by_name()::State is not valid");
    return false;
  }

  for (size_t iter = 0; iter < state->lang_data.size(); iter++) {
    std::string _str = state->lang_data.at(iter).language_name;
    if(_str == language_name) {
      state->active_loc = &state->lang_data.at(iter);
      return true;
    }
  }

  return false;
}
bool loc_parser_set_active_language_by_index(unsigned int index) {  
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_set_active_language_by_index()::State is not valid");
    return false;
  }
  if (index < 0 || index >= state->lang_data.size()) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_set_active_language_by_index()::Language index is out of bound");
    return false;
  }

  state->active_loc = &state->lang_data.at(index);

  return true;
}
loc_data* loc_parser_get_active_language(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_get_active_language()::State is not valid");
    return {};
  }
  
  return state->active_loc;
}

const char* lc_txt(u32 index) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::lc_txt()::State is not valid");
    return "";
  }
  if (index > state->active_loc->content.size()) {
    TraceLog(LOG_ERROR, "loc_parser::lc_txt()::Out of range");
    return "";
  }
  return state->active_loc->content.at(index).c_str();
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
bool is_variable_allowed(u8& c) {
  if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_')) {
    return true;
  }
  return false;
}

#undef loc_content_map
#undef loc_content_symbol
#undef loc_content_text

