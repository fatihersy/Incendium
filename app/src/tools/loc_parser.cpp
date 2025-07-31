#include "loc_parser.h"

#include "raylib.h"

#include "core/fmemory.h"
#include "loc_types.h"

const char * default_language = R"(
{
  display_text = "English",
  codepoints = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
  translates = {\
    "Play"
    "Upgrade"
    "Settings"
    "Exit"
  }
}
)";

typedef struct loc_content_text {
  std::string str;
  loc_content_text(void) {
    this->str = std::string("");
  }
}loc_content_text;

#define LOC_MAX_CODEPOINTS 128
#define SYMBOL_DIGITS_START_SYMBOL 0x21
#define LOC_FILE_TEXT_SYMBOL_LENGTH 3
#define LOC_FILE_PATH_PREFIX "./loc/"
#define LOC_FILE_EXTENSION "._loc_data"
#define loc_content_map std::array<std::string, LOC_TEXT_MAX>
#define loc_content_symbol std::string
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
  i32 scope_start_offset;
  i32 scope_end_offset;
} loc_file_scope;

typedef enum loc_reading_order {
  LOC_READING_ORDER_UNDEFINED,
  LOC_READING_ORDER_CONTENT,
  LOC_READING_ORDER_MAX,
} loc_reading_order;

typedef struct loc_parser_system_state {
  std::vector<loc_data> lang_data;
  loc_data * active_loc;
  
  loc_data _buffer;
  std::vector<u8> file_buffer;
  i32 file_size;
}loc_parser_system_state;

static loc_parser_system_state * state;

loc_content_text loc_parser_get_next_content_text(u32& offset);
loc_content_symbol loc_parser_read_symbol(u32& offset);
loc_content_lang_name loc_parser_read_language_name(void);
loc_content_codepoints loc_parser_read_codepoints(void);
loc_content_map loc_parser_read_map(void);

i32 loc_parser_go_to_variable(std::string variable_name);
void loc_parser_read_content(void);
std::string loc_parser_get_text(i32& offset, std::string text);
std::string loc_parser_get_next_text(i32& offset);
i32 loc_parser_go_to_next_statement(i32 offset);
loc_file_scope loc_parser_get_scope_range(i32 offset);
i32 get_content_text_value(i32& offset);
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
  loc_data data = loc_data();
  state->file_size = 0u;
  state->file_buffer.clear();
  state->_buffer = loc_data();
  {
    int dataSize = 1;
    u8* _str_tile = LoadFileData(file_path, __builtin_addressof(dataSize));
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
  state->_buffer = loc_data();
  for (size_t iter = 0; iter < file_names.count; iter++) {
    loc_data data = loc_data();
    {
      int dataSize = 1;
      u8* _str_tile = LoadFileData(file_names.paths[iter], __builtin_addressof(dataSize));
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

    loc_data data = loc_data();
    data.language_name = loc_parser_read_language_name();
    data.codepoints = loc_parser_read_codepoints();
    data.content = loc_parser_read_map();
    data.index = state->lang_data.size();
    state->lang_data.push_back(data);
  }
  UnloadDirectoryFiles(file_names);
  return true;
}

loc_content_text loc_parser_get_next_content_text(i32& offset) {
  loc_content_text _loc_con = loc_content_text();

  i32 begin_quote = I32_MAX;
  i32 end_quote = I32_MAX;
  bool begin_quote_found = false;
  bool end_quote_found = false;

  for (; offset < state->file_size; ++offset) {
    if (!begin_quote_found) {
      if (state->file_buffer.at(offset) == '"') {
        begin_quote = offset + 1;
        begin_quote_found = true;
      }
      continue;
    }
    else if (!end_quote_found) {
      if (state->file_buffer.at(offset) == '"') {
        end_quote = offset;
        end_quote_found = true;
      }
      continue;
    }
    else {
      break;
    }
  }

  if (begin_quote < state->file_size && end_quote < state->file_size) {
    _loc_con.str.assign(state->file_buffer.begin() + begin_quote, state->file_buffer.begin() + end_quote);
  }
  return _loc_con;
}
i32 get_content_text_value(i32& offset) {
  i32 loc_con_value = I32_MAX;
  std::string value_str = std::string("");
  bool number_found = false;

  for (; offset < state->file_size; ++offset) {
    u8 c = state->file_buffer.at(offset); 

    if (c >= '0' && c <= '9') {
      number_found = true;
      value_str.push_back(c);
    }
    else if(number_found) break;
  }
  loc_con_value = TextToInteger(value_str.c_str());

  return loc_con_value;
}

std::string loc_parser_get_text(i32& offset, std::string text) {
  std::string _text = std::string("");

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
std::string loc_parser_get_next_text(i32& offset) {
  std::string text = std::string("");

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
i32 loc_parser_go_to_next_statement(i32 offset) {
  
  for (; offset < state->file_size; ++offset) {
    if(state->file_buffer.at(offset) == LOC_FILE_SYMBOL_STATEMENT_PARSER) {
      return offset;
    }
  }

  return U32_MAX;
}
loc_file_scope loc_parser_get_scope_range(i32 offset) {
  loc_file_scope _scope = {
    .scope_start_offset = I32_MAX,
    .scope_end_offset = I32_MAX
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

i32 loc_parser_go_to_variable(std::string variable_name) {
  std::string variable = std::string("");
  bool variable_found = false;
  bool in_variable_try = false;
  i32 variable_search_offset = 0;
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

loc_content_symbol loc_parser_read_symbol(i32& offset) {
  loc_content_symbol symbol = std::string("");
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
  i32 offset = loc_parser_go_to_variable(LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME);

  if (offset < state->file_size) {
    return loc_parser_get_next_text(offset);
  }
  else return "";
}
loc_content_codepoints loc_parser_read_codepoints(void) {
  i32 offset = loc_parser_go_to_variable(LOC_FILE_CODEPOINTS_VARIABLE_NAME);

  if (offset < state->file_size) {
    return loc_parser_get_next_text(offset);
  }
  else return "";
}
loc_content_map loc_parser_read_map(void) {

  i32 offset = loc_parser_go_to_variable(LOC_FILE_MAP_VARIABLE_NAME);

  loc_file_scope scope = loc_parser_get_scope_range(offset);
  offset = scope.scope_start_offset;

  if (scope.scope_start_offset > state->file_size) {
    return {};
  }

  loc_reading_order reading_order = static_cast<loc_reading_order>(LOC_READING_ORDER_UNDEFINED+1);

  loc_content_text text = loc_content_text();
  loc_content_map content_map = {};
  i32 next_index = 0;

  for (i32 offset=scope.scope_start_offset; offset < scope.scope_end_offset;) {
    switch (reading_order) {
      case LOC_READING_ORDER_CONTENT: {
        text = loc_parser_get_next_content_text(offset);
        content_map.at(next_index) = text.str;
        next_index++;
        
        if (static_cast<size_t>(next_index) < content_map.size()) {
          break;
        }
        else {
          return content_map;
        }
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
      state->active_loc = __builtin_addressof(state->lang_data.at(iter));
      return true;
    }
  }

  return false;
}
bool loc_parser_set_active_language_by_index(int index) {  
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_set_active_language_by_index()::State is not valid");
    return false;
  }
  if ( index < 0 || static_cast<size_t>(index) > state->lang_data.size()) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_set_active_language_by_index()::Language index is out of bound");
    return false;
  }

  state->active_loc = __builtin_addressof(state->lang_data.at(index));
  return true;
}
loc_data* loc_parser_get_active_language(void) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::loc_parser_get_active_language()::State is not valid");
    return nullptr;
  }
  
  return state->active_loc;
}

const char* lc_txt(i32 index) {
  if (!state) {
    TraceLog(LOG_ERROR, "loc_parser::lc_txt()::State is not valid");
    return "";
  }
  if (static_cast<size_t>(index) > state->active_loc->content.size()) {
    TraceLog(LOG_ERROR, "loc_parser::lc_txt()::Out of range");
    return "";
  }
  return state->active_loc->content.at(index).c_str();
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

#undef LOC_MAX_CODEPOINTS
#undef SYMBOL_DIGITS_START_SYMBOL
#undef LOC_FILE_TEXT_SYMBOL_LENGTH
#undef LOC_FILE_PATH_PREFIX
#undef LOC_FILE_EXTENSION
#undef loc_content_map
#undef loc_content_symbol
#undef loc_content_text
#undef loc_content_lang_name
#undef loc_content_codepoints

#undef LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME
#undef LOC_FILE_CODEPOINTS_VARIABLE_NAME
#undef LOC_FILE_MAP_VARIABLE_NAME

#undef LOC_FILE_SYMBOL_VARIABLE_INITIALIZATION
#undef LOC_FILE_SYMBOL_STATEMENT_PARSER
#undef LOC_FILE_SYMBOL_SCOPE_START
#undef LOC_FILE_SYMBOL_SCOPE_END
