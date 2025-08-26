#include "loc_parser.h"
#include "raylib.h"

#include "core/fmemory.h"
#include "core/logger.h"

#include "loc_types.h"
#include "pak_parser.h"

typedef struct loc_content_text {
  std::string str;
  loc_content_text(void) {
    this->str = std::string("");
  }
} loc_content_text;

#define LOC_MAX_CODEPOINTS 128
#define SYMBOL_DIGITS_START_SYMBOL 0x21
#define LOC_FILE_TEXT_SYMBOL_LENGTH 3
#define LOC_FILE_EXTENSION "._loc_data"
#define LOC_FILE_PAK_FILE PAK_FILE_ASSET2

#define LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME "display_text"
#define LOC_FILE_CODEPOINTS_VARIABLE_NAME "codepoints"
#define LOC_FILE_MAP_VARIABLE_NAME "translates"

#define LOC_FILE_SYMBOL_VARIABLE_INITIALIZATION '='
#define LOC_FILE_SYMBOL_STATEMENT_PARSER ','
#define LOC_FILE_SYMBOL_SCOPE_START '{'
#define LOC_FILE_SYMBOL_SCOPE_END '}'

typedef struct loc_file_scope {
  size_t scope_start_offset;
  size_t scope_end_offset;
  loc_file_scope(void) {
    this->scope_start_offset = 0u;
    this->scope_end_offset = 0u;
  }
  loc_file_scope(size_t start, size_t end) {
    this->scope_start_offset = start;
    this->scope_end_offset = end;
  }
} loc_file_scope;

typedef enum loc_reading_order {
  LOC_READING_ORDER_UNDEFINED,
  LOC_READING_ORDER_CONTENT,
  LOC_READING_ORDER_MAX,
} loc_reading_order;

typedef struct loc_parser_system_state {
  std::vector<loc_data> lang_data;
  loc_data * active_loc;
  std::string default_language;
  
  loc_data _buffer;
  std::vector<u8> file_buffer;
  loc_parser_system_state(void) {
    this->default_language = std::string();
    this->lang_data = std::vector<loc_data>();
    this->active_loc = nullptr;
    this->_buffer = loc_data();
    this->file_buffer = std::vector<u8>();
  }
}loc_parser_system_state;

static loc_parser_system_state * state = nullptr;

loc_content_text loc_parser_get_next_content_text(size_t& offset);
std::string loc_parser_read_symbol(size_t& offset);
std::string loc_parser_read_language_name(void);
std::string loc_parser_read_codepoints(void);
std::array<std::string, LOC_TEXT_MAX> loc_parser_read_map(void);

size_t loc_parser_go_to_variable(std::string variable_name);
std::string loc_parser_get_text(size_t& offset, std::string text);
std::string loc_parser_get_next_text(size_t& offset);
size_t loc_parser_go_to_next_statement(size_t offset);
loc_file_scope loc_parser_get_scope_range(size_t offset);
i32 get_content_text_value(size_t& offset);
bool is_symbol_allowed(u8& c);
bool is_variable_allowed(u8& c);

bool loc_parser_system_initialize(void) {
  if (state and state != nullptr) {
    return true;
  }
  state = (loc_parser_system_state*)allocate_memory_linear(sizeof(loc_parser_system_state), true);
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_system_initialize()::loc parser state allocation failed");
    return false;
  }
  *state = loc_parser_system_state();

  // Default Language English
  {
    state->default_language = std::string
    (R"(
      {
        display_text = "English",
        codepoints = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-><'.:!,/",
        translates = {
          "::NULL",

          "Play"                          Main Menu,
          "Character",
          "Settings",
          "Exit",
          "Editor",
          "Inventory",
          "Stats",
          "Back",
          "Upgrade",
          "Total",
          "Resume",
          "Exit to Main Menu",
          "Exit to Desktop",
          "Back",
          "Traits",
          "Chosen Traits",
          "Starter Ability",
          "Back",
          "Accept",
          "Remaining Points",

          "Windowed"                      Settings ,
          "Borderless",
          "Full Screen",
          "Apply",
          "Cancel",
          "English",
          "Turkish",

          "TREE"                          Editor,
          "TOMBSTONE",
          "STONE",
          "SPIKE",
          "SKULL",
          "PILLAR",
          "LAMP",
          "FENCE",
          "DETAIL",
          "CANDLE",
          "BUILDING",
          "SPRITE",

          "Press Space To Start!"         In Game,
          "NEW!",
          "Damage:",
          "Amouth:",
          "Hitbox:",
          "Speed:",
          "Stage Cleared",
          "Dead",
          "Collected Souls:",
          "Accept",

          "Life Essence"                   Player,
          "Essential",
          "Bread",
          "Keeps you fed",
          "Cardinal Boots",
          "Increases the speed",
          "Blast Scroll",
          "Increases the Hitbox",
          "Heavy Cross",
          "Increases Damage",
          "Hourglass",
          "Reduce cooldown",
          "Second Hand",
          "Increases projectile amouth",
          "Seeing Eyes",
          "Increases experience gain",
          "Jack-Of-All-Traits",
          "Increases your total trait points",
          "Bullet",
          "Arcane Codex",
          "Comet",
          "Fireball",
          "Blazing Steps",
          "Radience",

          "Work in progress!",
          "See the light",

          "Please select a starter ability"           Display error,
          "You need more souls!",
          "This stage cannot playable right now",
        }
      }
      )"
    );
  }

  return true;
}

bool _loc_parser_parse_localization_data_from_file(i32 pak_id, i32 index) {
  return loc_parser_parse_localization_data_from_file(pak_id, index);
}
bool _loc_parser_parse_localization_data(void) {
  return loc_parser_parse_localization_data();
}

bool loc_parser_parse_localization_data_from_file(i32 pak_id, i32 index) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  loc_data data = loc_data();
  state->file_buffer.clear();
  state->_buffer = loc_data();

  const file_buffer * file = get_asset_file_buffer(static_cast<pak_file_id>(pak_id), index);
  if (not file or file == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::File %d:%d is invalid", pak_id, index);
    return false;
  }
  state->file_buffer.assign_range(file->content);

  data.language_name = loc_parser_read_language_name();
  data.codepoints = loc_parser_read_codepoints();
  data.content = loc_parser_read_map();

  state->lang_data.push_back(data);
  return true;
}

bool loc_parser_parse_localization_data(void) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  std::vector<i32> file_ids = std::vector<i32>();

  file_ids.push_back(PAK_FILE_ASSET2_LOC_FILE_ENGLISH);
  file_ids.push_back(PAK_FILE_ASSET2_LOC_FILE_TURKISH);

  state->lang_data.clear();
  state->file_buffer.clear();
  state->_buffer = loc_data();
  for (size_t itr_000 = 0u; itr_000 < file_ids.size(); itr_000++) {
    loc_data data = loc_data();
    const file_buffer * file = get_asset_file_buffer(LOC_FILE_PAK_FILE, file_ids.at(itr_000));
    state->file_buffer.assign_range(file->content);
    
    data.language_name = loc_parser_read_language_name();
    data.codepoints = loc_parser_read_codepoints();
    data.content = loc_parser_read_map();
    data.index = state->lang_data.size();
    state->lang_data.push_back(data);
    IINFO("loc_parser::loc_parser_parse_localization_data()::Language:%s installed ", data.language_name.c_str());
  }
  if (state->lang_data.empty()) {
    state->file_buffer.clear();
    state->file_buffer.assign_range(state->default_language);

    loc_data data = loc_data();
    data.language_name = loc_parser_read_language_name();
    data.codepoints = loc_parser_read_codepoints();
    data.content = loc_parser_read_map();
    data.index = state->lang_data.size();
    state->lang_data.push_back(data);
  }
  return true;
}

loc_content_text loc_parser_get_next_content_text(size_t& offset) {
  loc_content_text _loc_con = loc_content_text();

  size_t begin_quote = U32_MAX;
  size_t end_quote = U32_MAX;
  bool begin_quote_found = false;
  bool end_quote_found = false;

  for (; offset < state->file_buffer.size(); ++offset) {
    if (not begin_quote_found) {
      if (state->file_buffer.at(offset) == '"') {
        begin_quote = offset + 1;
        begin_quote_found = true;
      }
      continue;
    }
    else if (not end_quote_found) {
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

  if (begin_quote < state->file_buffer.size() && end_quote < state->file_buffer.size()) {
    _loc_con.str.assign(state->file_buffer.begin() + begin_quote, state->file_buffer.begin() + end_quote);
  }
  return _loc_con;
}
i32 get_content_text_value(size_t& offset) {
  i32 loc_con_value = 0;
  std::string value_str = std::string("");
  bool number_found = false;

  for (; offset < state->file_buffer.size(); ++offset) {
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

std::string loc_parser_get_text(size_t& offset, std::string text) {
  std::string _text = std::string("");

  bool quote_found = false;
  for (; offset < state->file_buffer.size(); ++offset) {
    if (not quote_found) {
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
std::string loc_parser_get_next_text(size_t& offset) {
  std::string text = std::string("");

  bool quote_found = false;
  for (; offset < state->file_buffer.size(); ++offset) {
    if (not quote_found) {
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
size_t loc_parser_go_to_next_statement(size_t offset) {
  
  for (; offset < state->file_buffer.size(); ++offset) {
    if(state->file_buffer.at(offset) == LOC_FILE_SYMBOL_STATEMENT_PARSER) {
      return offset;
    }
  }

  return U32_MAX;
}
loc_file_scope loc_parser_get_scope_range(size_t offset) {
  loc_file_scope _scope = loc_file_scope(U32_MAX, U32_MAX);
  bool scope_start_found = false;
  
  for (; offset < state->file_buffer.size(); ++offset) {
    if(not scope_start_found and state->file_buffer.at(offset) == LOC_FILE_SYMBOL_SCOPE_START) {
      scope_start_found = true;
      _scope.scope_start_offset = offset;
    }
    else if (scope_start_found and state->file_buffer.at(offset) == LOC_FILE_SYMBOL_SCOPE_END) {
      _scope.scope_end_offset = offset;
      return _scope;
    }
  }

  return _scope;
}

size_t loc_parser_go_to_variable(std::string variable_name) {
  std::string variable = std::string("");
  bool variable_found = false;
  bool in_variable_try = false;
  size_t variable_search_offset = 0u;
  do {
    variable.clear();
    in_variable_try = false;
    for (; variable_search_offset < state->file_buffer.size(); ++variable_search_offset) 
    {
      if( not variable_found and is_variable_allowed(state->file_buffer.at(variable_search_offset))) {
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
      else if(variable_found and state->file_buffer.at(variable_search_offset) == LOC_FILE_SYMBOL_VARIABLE_INITIALIZATION) {
        return variable_search_offset;
      }
    }
  } while(variable_search_offset < state->file_buffer.size() and not variable_found);

  if (variable_found) {
    return variable_search_offset;
  }
  else return U32_MAX;
}

std::string loc_parser_read_symbol(size_t& offset) {
  std::string symbol = std::string("");
  char parser = '=';

  for (; offset < state->file_buffer.size(); ++offset) {
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
std::string loc_parser_read_language_name(void) {
  size_t offset = loc_parser_go_to_variable(LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME);

  if (offset < state->file_buffer.size()) {
    return loc_parser_get_next_text(offset);
  }
  else return "";
}
std::string loc_parser_read_codepoints(void) {
  size_t offset = loc_parser_go_to_variable(LOC_FILE_CODEPOINTS_VARIABLE_NAME);

  if (offset < state->file_buffer.size()) {
    return loc_parser_get_next_text(offset);
  }
  else return "";
}
std::array<std::string, LOC_TEXT_MAX> loc_parser_read_map(void) {
  size_t offset = loc_parser_go_to_variable(LOC_FILE_MAP_VARIABLE_NAME);
  loc_file_scope scope = loc_parser_get_scope_range(offset);
  offset = scope.scope_start_offset;

  if (scope.scope_start_offset > state->file_buffer.size()) {
    return std::array<std::string, LOC_TEXT_MAX>();
  }
  loc_reading_order reading_order = static_cast<loc_reading_order>(LOC_READING_ORDER_UNDEFINED+1);
  loc_content_text text = loc_content_text();
  std::array<std::string, LOC_TEXT_MAX> content_map = std::array<std::string, LOC_TEXT_MAX>();
  size_t next_index = 0u;

  for (size_t offset = scope.scope_start_offset; offset < scope.scope_end_offset;) {
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
        IWARN("loc_parser::loc_parser_read_map()::Unsupported reading order stage");
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
  return localized_languages(state->lang_data);
}

bool loc_parser_set_active_language_by_name(std::string language_name) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_set_active_language_by_name()::State is not valid");
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
bool loc_parser_set_active_language_by_index(int _index) {  
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_set_active_language_by_index()::State is not valid");
    return false;
  }
  size_t index = static_cast<size_t>(_index);

  if ( index < 0 or index >= state->lang_data.size()) {
    IERROR("loc_parser::loc_parser_set_active_language_by_index()::Language index is out of bound");
    return false;
  }
  state->active_loc = __builtin_addressof(state->lang_data.at(index));
  return true;
}
loc_data* loc_parser_get_active_language(void) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_get_active_language()::State is not valid");
    return nullptr;
  }
  return state->active_loc;
}

const char* lc_txt(i32 txt_id) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::lc_txt()::State is not valid");
    return "\0";
  }
  if (txt_id >= LOC_TEXT_MAX or txt_id < LOC_TEXT_UNDEFINED ) {
    IWARN("loc_parser::lc_txt()::Out of range");
    return "\0";
  }
  return state->active_loc->content.at(static_cast<size_t>(txt_id)).c_str();
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

#undef LOC_FILE_LANGUAGE_NAME_VARIABLE_NAME
#undef LOC_FILE_CODEPOINTS_VARIABLE_NAME
#undef LOC_FILE_MAP_VARIABLE_NAME

#undef LOC_FILE_SYMBOL_VARIABLE_INITIALIZATION
#undef LOC_FILE_SYMBOL_STATEMENT_PARSER
#undef LOC_FILE_SYMBOL_SCOPE_START
#undef LOC_FILE_SYMBOL_SCOPE_END
