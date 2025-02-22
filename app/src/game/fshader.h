
#ifndef FSHADER_H
#define FSHADER_H

#include <defines.h>

void initialize_shader_system(void);

const char* shader_path(const char* _path);
fshader* get_shader_by_enum(shader_id _id);
void set_shader_uniform(shader_id _id, i32 index, data_pack _data_pack);


#endif
