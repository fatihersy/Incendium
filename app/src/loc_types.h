#ifndef LOC_TYPES_H
#define LOC_TYPES_H

typedef enum supported_language {
  LANGUAGE_UNSPECIFIED,
  LANGUAGE_ENGLISH,
  LANGUAGE_TURKISH,
  LANGUAGE_MAX
} supported_language;

typedef struct loc_content {
 const char* symbol;
 const char* text;
}loc_content;

const char* lc_txt(unsigned int symbol);

bool loc_parser_parse_localization_data_from_file(const char* file_name);

const char* loc_lang_get_file_name(supported_language lang);
supported_language loc_lang_file_name_to_enum(const char* file_name);

unsigned int symbol_to_index(const char* symbol);

#endif