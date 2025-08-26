
#ifndef FSHADER_H
#define FSHADER_H

#include <game/game_types.h>

typedef struct fshader_location {
  std::string name;
  data128 data;
  ShaderUniformDataType uni_data_type;
  fshader_location(void) {
    this->name.clear();
    this->data = data128();
    this->uni_data_type = SHADER_UNIFORM_FLOAT;
  }
} fshader_location;

typedef struct fshader {
  i32 total_locations;
  Shader handle;
  std::array<fshader_location, MAX_SHADER_LOCATION_SLOT> locations;
  fshader(void) {
    this->total_locations = 0;
    this->handle = Shader { 0u, nullptr};
    this->locations.fill(fshader_location());
  }
} fshader;

[[__nodiscard__]] bool initialize_shader_system(void);

const char* shader_path(const char* _path);
const fshader* get_shader_by_enum(shader_id _id);
i32 get_uniform_location(shader_id _id, const char * uni_name);
void set_shader_uniform(shader_id _id, const char* uni_name, data128 _data_pack);


#endif
