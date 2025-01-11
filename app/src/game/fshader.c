#include "fshader.h"

#include "core/fmemory.h"
#include "defines.h"
#include "raylib.h"

typedef struct shader_system_state {
  u16 shader_amouth;
  fshader shaders[SHADER_TYPE_MAX];
} shader_system_state;

static shader_system_state *state;

void load_shader(const char *_vs_path, const char *_fs_path, shader_type _type);
void shader_add_uniform(shader_type _type, const char *_name, ShaderUniformDataType _data_type);

void initialize_shader_system() {
  if (state) {
    return;
  }

  state = (shader_system_state *)allocate_memory_linear(sizeof(shader_system_state), true);

  // NOTE: _path = "%s%s", SHADER_PATH, _path
  load_shader(0, "mask.fs", SHADER_TYPE_PROGRESS_BAR_MASK);
  shader_add_uniform(SHADER_TYPE_PROGRESS_BAR_MASK, "progress", SHADER_UNIFORM_FLOAT);
}

const char *shader_path(const char *_path) {
  return (_path) ? TextFormat("%s%s", SHADER_PATH, _path) : 0;
}

fshader *get_shader_by_enum(shader_type type) {
  if (type >= SHADER_TYPE_MAX || type <= SHADER_TYPE_UNSPECIFIED) {
    TraceLog(LOG_WARNING,
             "resource::get_shader_by_enum()::Shader type out of bound");
    return (fshader *){0};
  }

  return &state->shaders[type];
}

void set_shader_uniform(shader_type _type, i32 index, data_pack _data_pack) {
  if (_type >= SHADER_TYPE_MAX || _type <= SHADER_TYPE_UNSPECIFIED) {
    TraceLog(LOG_ERROR,
    "resource::shader_add_attribute()::Shader type out of bound");
    return;
  }
  fshader* shader = &state->shaders[_type];
  ShaderUniformDataType data_type = shader->locations[index].uni_data_type;

  switch (data_type) {
  case SHADER_UNIFORM_FLOAT: {

    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.f32[0],
                   data_type);
    break;
  }
  case SHADER_UNIFORM_VEC2: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.f32,
                   data_type);
    break;
  }
  case SHADER_UNIFORM_VEC3: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.f32,
                   data_type);
    break;
  }
  case SHADER_UNIFORM_VEC4: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.f32,
                   data_type);
    break;
  }
  case SHADER_UNIFORM_INT: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.i32[0],
                   data_type);
    break;
  }
  case SHADER_UNIFORM_IVEC2: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.i32,
                   data_type);
    break;
  }
  case SHADER_UNIFORM_IVEC3: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.i32,
                   data_type);
    break;
  }
  case SHADER_UNIFORM_IVEC4: {
    SetShaderValue(state->shaders[_type].handle, index,
                   &state->shaders[_type].locations[index].data.data.i32,
                   data_type);
    break;
  }
  case SHADER_UNIFORM_SAMPLER2D: {
    SetShaderValue(state->shaders[_type].handle, index, 
    state->shaders[_type].locations[index].data.sampler,
    data_type);
    break;
  }
  default: {
      TraceLog(LOG_ERROR,
      "shader_add_uniform()::Error while setting shader value");
      return;
    }
  }

  shader->locations[index].data = _data_pack;
}

void load_shader(const char *_vs_path, const char *_fs_path,
                 shader_type _type) {
  const char *vs_path = shader_path(_vs_path);
  if (!FileExists(vs_path) && vs_path) {
    TraceLog(LOG_ERROR, "resource::load_shader()::Vertex path does not exist");
    return;
  }
  const char *fs_path = shader_path(_fs_path);
  if (!FileExists(fs_path) && fs_path) {
    TraceLog(LOG_ERROR,
             "resource::load_shader()::Fragment path does not exist");
    return;
  }
  if (_type >= SHADER_TYPE_MAX || _type <= SHADER_TYPE_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "resource::load_shader()::Shader type out of bound");
    return;
  }

  state->shader_amouth++;
  state->shaders[_type].handle = LoadShader(vs_path, fs_path);
  state->shaders[_type].total_locations = 0;
}


void shader_add_uniform(shader_type _type, const char *_name, ShaderUniformDataType _data_type) {
  if (_type >= SHADER_TYPE_MAX || _type <= SHADER_TYPE_UNSPECIFIED) {
    TraceLog(LOG_ERROR,
    "resource::shader_add_attribute()::Shader type out of bound");
    return;
  }
  u16 total_loc = state->shaders[_type].total_locations;

  state->shaders[_type].locations[total_loc].index = GetShaderLocation(
    state->shaders[_type].handle, _name);
  TextCopy(state->shaders[_type].locations[total_loc].name, _name);
  state->shaders[_type].locations[total_loc].uni_data_type = _data_type;

  total_loc++;
  state->shaders[_type].total_locations = total_loc;
}
