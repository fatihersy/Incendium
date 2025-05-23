
#ifndef LOC_PARSER_H
#define LOC_PARSER_H

bool loc_parser_system_initialize(void);

bool _loc_parser_parse_localization_data_from_file(const char* file_name);

bool _loc_parser_parse_localization_data(void);

#endif
