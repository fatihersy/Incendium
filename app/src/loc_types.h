#ifndef LOC_TYPES_H
#define LOC_TYPES_H

#define LOC_TEXT_SYMBOL_SIZE 3
#define LOC_TEXT_VARIABLE_SIZE 12

#define LOC_TEXT_MAINMENU_BUTTON_TEXT_PLAY 0
#define LOC_TEXT_MAINMENU_BUTTON_TEXT_UPGRADE 1
#define LOC_TEXT_MAINMENU_BUTTON_TEXT_SETTINGS 2
#define LOC_TEXT_MAINMENU_BUTTON_TEXT_EXIT 3

#include <string>
#include <vector>

typedef struct loc_data {
  std::string language_name;
  std::string codepoints;
  std::vector<std::string> content;
  unsigned int index;
} loc_data;

typedef struct loc_content {
 const char* symbol;
 const char* text;
}loc_content;

typedef struct localized_languages {
  std::vector<loc_data> lang;
} localized_languages;

const char* lc_txt(unsigned int symbol);

bool loc_parser_parse_localization_data_from_file(const char* file_name);
bool loc_parser_parse_localization_data(void);

localized_languages loc_parser_get_loc_langs(void);
loc_data* loc_parser_get_active_language(void);
bool loc_parser_set_active_language_by_name(std::string language_name);
bool loc_parser_set_active_language_by_index(unsigned int index);

unsigned int symbol_to_index(const char* symbol);

#endif