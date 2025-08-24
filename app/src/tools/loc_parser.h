
#ifndef LOC_PARSER_H
#define LOC_PARSER_H

bool loc_parser_system_initialize(void);

bool _loc_parser_parse_localization_data_from_file(int pak_id, int index);

bool _loc_parser_parse_localization_data(void);

#endif
