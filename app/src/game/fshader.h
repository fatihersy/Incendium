
#ifndef FSHADER_H
#define FSHADER_H

#include <defines.h>

typedef struct fshader_location {
  char name[MAX_SHADER_LOCATION_NAME_LENGTH];
  u16 index;
  data128 data;
  ShaderUniformDataType uni_data_type;
} fshader_location;

typedef struct fshader {
  u16 total_locations;
  Shader handle;
  fshader_location locations[MAX_SHADER_LOCATION_SLOT];
} fshader;

void initialize_shader_system(void);

const char* shader_path(const char* _path);
fshader* get_shader_by_enum(shader_id _id);
void set_shader_uniform(shader_id _id, i32 index, data128 _data_pack);


#endif
