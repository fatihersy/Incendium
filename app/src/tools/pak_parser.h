
#ifndef PAK_PARSER_H
#define PAK_PARSER_H

#include "defines.h"

[[__nodiscard__]] bool pak_parser_system_initialize(void);

bool parse_asset_pak(pak_file_id id);

const file_buffer *  get_asset_file_buffer(pak_file_id id, i32 index);
worldmap_stage_file get_map_file_data(i32 index);

const file_buffer *  fetch_asset_file_buffer(pak_file_id pak_id, i32 index);
worldmap_stage_file fetch_map_file_data(i32 index);

#endif
