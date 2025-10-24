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
  std::array<loc_data, LANGUAGE_INDEX_MAX> lang_data;
  loc_data * active_loc;
  std::string default_language;
  
  std::vector<u8> file_buffer;
  loc_parser_system_state(void) {
    this->default_language = std::string();
    this->lang_data.fill(loc_data());
    this->active_loc = nullptr;
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
        display_text = "Builtin",
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
          "Collected Coins:",
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
          "You need more coin!",
          "This stage cannot playable right now",
        }
      }
      )"
    );
  }

  return true;
}

bool _loc_parser_parse_localization_data_from_file(int pak_id, int file_index, int lang_index) {
  return loc_parser_parse_localization_data_from_file(pak_id, file_index, static_cast<language_index>(lang_index));
}
bool _loc_parser_parse_localization_data(void) {
  return loc_parser_parse_localization_data();
}

bool loc_parser_parse_localization_data_from_file(int pak_id, int file_index, language_index lang_index) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  loc_data data = loc_data();
  state->file_buffer.clear();

  const file_buffer * file = get_asset_file_buffer(static_cast<pak_file_id>(pak_id), file_index);
  if (not file or file == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::File %d:%d is invalid", pak_id, file_index);
    return false;
  }
  state->file_buffer.assign_range(file->content);

  data.language_name = loc_parser_read_language_name();
  data.codepoints = loc_parser_read_codepoints();
  if (data.language_name.empty() or data.codepoints.empty()) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::File parse is failed");
    return false;
  }
  data.content = loc_parser_read_map();
  data.index = lang_index;

  state->lang_data.at(lang_index) = data;
  return true;
}

bool loc_parser_parse_localization_data(void) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data()::State is invalid");
    return false;
  }
  
  if (not loc_parser_parse_localization_data_from_file(LOC_FILE_PAK_FILE, PAK_FILE_ASSET2_LOC_FILE_ENGLISH, LANGUAGE_INDEX_ENGLISH)) {
    IWARN("loc_parser::loc_parser_parse_localization_data()::Parsing English file is failed");
    return false;
  }

  if (not loc_parser_parse_localization_data_from_file(LOC_FILE_PAK_FILE, PAK_FILE_ASSET2_LOC_FILE_TURKISH, LANGUAGE_INDEX_TURKISH)) {
    IWARN("loc_parser::loc_parser_parse_localization_data()::Parsing Turkish file is failed");
    return false;
  }

  if (not loc_parser_parse_builtin_localization_data()) {
    IWARN("loc_parser::loc_parser_parse_localization_data()::Parsing builtin language is failed");
    return false;
  }

  return true;
}
bool loc_parser_parse_builtin_localization_data(void) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_parse_localization_data_from_file()::Loc parser state is not valid");
    return false;
  }
  state->file_buffer.clear();

  loc_data data = loc_data();
  state->file_buffer.assign_range(state->default_language);
  
  data.language_name = loc_parser_read_language_name();
  data.codepoints = loc_parser_read_codepoints();
  data.content = loc_parser_read_map();
  data.index = LANGUAGE_INDEX_BUILTIN;

  state->lang_data.at(LANGUAGE_INDEX_BUILTIN) = data;

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
language_index loc_parser_lang_name_to_index(const char * name) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_lang_name_to_index()::State is not valid");
    return LANGUAGE_INDEX_UNDEFINED;
  }
  if (not name or name == nullptr) {
    IERROR("loc_parser::loc_parser_lang_name_to_index()::Name is not valid");
    return LANGUAGE_INDEX_UNDEFINED;
  }

  if(TextIsEqual(name, "Builtin")) {
    return LANGUAGE_INDEX_BUILTIN;
  }
  else if(TextIsEqual(name, "English")) {
    return LANGUAGE_INDEX_ENGLISH;
  }
  else if(TextIsEqual(name, "Turkish")) {
    return LANGUAGE_INDEX_TURKISH;
  }
  else {
    IWARN("loc_parser::loc_parser_lang_name_to_index()::Unsupported name");
    return LANGUAGE_INDEX_UNDEFINED;
  }
}
const char * loc_parser_lang_index_to_name(language_index index) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_lang_name_to_index()::State is not valid");
    return nullptr;
  }
  if (index <= LANGUAGE_INDEX_UNDEFINED or index >= LANGUAGE_INDEX_MAX) {
    IERROR("loc_parser::loc_parser_lang_name_to_index()::Index is out of bound");
    return nullptr;
  }

  if(index == LANGUAGE_INDEX_BUILTIN) {
    return "Builtin";
  }
  else if(index == LANGUAGE_INDEX_ENGLISH) {
    return "English";
  }
  else if(index == LANGUAGE_INDEX_TURKISH) {
    return "Turkish";
  }
  else {
    IWARN("loc_parser::loc_parser_lang_name_to_index()::Unsupported name");
    return nullptr;
  }
}

const std::array<loc_data, LANGUAGE_INDEX_MAX> * loc_parser_get_loc_langs(void) {
  return __builtin_addressof(state->lang_data);
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
bool loc_parser_set_active_language_by_index(language_index _index) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_set_active_language_by_index()::State is not valid");
    return false;
  }
  if (_index < LANGUAGE_INDEX_UNDEFINED or _index > LANGUAGE_INDEX_MAX) {
    IERROR("loc_parser::loc_parser_set_active_language_by_index()::Language index is out of bound");
    return false;
  }
  loc_data& loc = state->lang_data.at(_index);
  if (loc.index > LANGUAGE_INDEX_UNDEFINED and loc.index < LANGUAGE_INDEX_MAX) {
    state->active_loc = __builtin_addressof(loc);
    return true;
  }
  return false;
}
bool loc_parser_set_active_language_builtin(void) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_set_active_language_by_index()::State is not valid");
    return false;
  }
  if (loc_parser_set_active_language_by_index(LANGUAGE_INDEX_BUILTIN)) {
    state->active_loc = loc_parser_get_active_language();
    return true;
  }
  return false;
}
loc_data* loc_parser_get_active_language(void) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_get_active_language()::State is not valid");
    return nullptr;
  }
  return state->active_loc;
}
const loc_data * loc_parser_get_language_by_index(language_index index) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_get_language_by_index()::State is not valid");
    return nullptr;
  }
  if (index < LANGUAGE_INDEX_UNDEFINED or index > LANGUAGE_INDEX_MAX) {
    IERROR("loc_parser::loc_parser_get_language_by_index()::Language index is out of bound");
    return __builtin_addressof(state->lang_data.at(LANGUAGE_INDEX_BUILTIN));
  }
  
  return __builtin_addressof(state->lang_data.at(index));
}
const loc_data*  loc_parser_get_language_by_name(const char * name) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::loc_parser_get_language_by_name()::State is not valid");
    return nullptr;
  }
  if (not name or name == nullptr) {
    IERROR("loc_parser::loc_parser_get_language_by_name()::Name is invalid");
    return __builtin_addressof(state->lang_data.at(LANGUAGE_INDEX_BUILTIN));
  }
  
  for (const loc_data& loc : state->lang_data) {
    if (loc.language_name == name) {
    return __builtin_addressof(loc);
    }
  }
  return nullptr;
}

const char* lc_txt(i32 txt_id) {
  if (not state or state == nullptr) {
    IERROR("loc_parser::lc_txt()::State is invalid");
    return "NULL";
  }
  if (not state->active_loc or state->active_loc == nullptr) {
    IERROR("loc_parser::lc_txt()::Localization is invalid");
    return "NULL";
  }
  if (txt_id >= LOC_TEXT_MAX or txt_id <= LOC_TEXT_UNDEFINED ) {
    IWARN("loc_parser::lc_txt()::Out of range");
    return "NULL";
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
