#include "fshader.h"
#include <raylib.h>
#include <defines.h>

#include "core/fmemory.h"

typedef struct shader_system_state {
  u16 shader_amouth;
  fshader shaders[SHADER_ID_MAX];
} shader_system_state;

static shader_system_state *state;

bool load_shader(const char *_vs_path, const char *_fs_path, shader_id _id);
void shader_add_uniform(shader_id _id, const char *_name, ShaderUniformDataType _data_id);

bool initialize_shader_system(void) {
  if (state) {
    return true;
  }

  state = (shader_system_state *)allocate_memory_linear(sizeof(shader_system_state), true);
  if (!state || state == nullptr) {
    TraceLog(LOG_ERROR, "fshader::initialize_shader_system()::State allocation failed");
    return false;
  }

  // NOTE: _path = "%s%s", SHADER_PATH, _path
  if (!load_shader(0, "prg_bar_mask.fs",    SHADER_ID_PROGRESS_BAR_MASK)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::mask shader cannot loaded");
    return false;
  }
  if (!load_shader(0, "fade_transition.fs", SHADER_ID_FADE_TRANSITION)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::fade shader cannot loaded");
    return false;
  }
  if(!load_shader(0, "font_outline.fs",    SHADER_ID_FONT_OUTLINE)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::outline shader cannot loaded");
    return false;
  }
  if(!load_shader(0, "post_process.fs",    SHADER_ID_POST_PROCESS)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::post process shader cannot loaded");
    return false;
  }

  shader_add_uniform(SHADER_ID_PROGRESS_BAR_MASK, "progress", SHADER_UNIFORM_FLOAT);
  shader_add_uniform(SHADER_ID_FADE_TRANSITION, "process", SHADER_UNIFORM_FLOAT);
  shader_add_uniform(SHADER_ID_FONT_OUTLINE, "texture_size", SHADER_UNIFORM_VEC2);
  shader_add_uniform(SHADER_ID_POST_PROCESS, "process", SHADER_UNIFORM_FLOAT);

  return true;
}

const char *shader_path(const char *_path) {
  return (_path) ? TextFormat("%s%s", SHADER_PATH, _path) : 0;
}

fshader *get_shader_by_enum(shader_id _id) {
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "fshader::get_shader_by_enum()::Shader type out of bound");
    return nullptr;
  }

  return &state->shaders[_id];
}

void set_shader_uniform(shader_id _id, i32 index, data128 _data_pack) {
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR,
    "fshader::shader_add_attribute()::Shader type out of bound");
    return;
  }
  fshader* shader = &state->shaders[_id];
  ShaderUniformDataType data_id = shader->locations[index].uni_data_type;

  switch (data_id) {
  case SHADER_UNIFORM_FLOAT: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_VEC2: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_VEC3: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_VEC4: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_INT: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.i32[0], data_id);
    break;
  }
  case SHADER_UNIFORM_IVEC2: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.i32, data_id);
    break;
  }
  case SHADER_UNIFORM_IVEC3: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.i32, data_id);
    break;
  }
  case SHADER_UNIFORM_IVEC4: {
    shader->locations[index].data = _data_pack;
    SetShaderValue(state->shaders[_id].handle, index, &state->shaders[_id].locations[index].data.i32, data_id);
    break;
  }
  case SHADER_UNIFORM_SAMPLER2D: {
    shader->locations[index].data.address[0] = _data_pack.address;
    SetShaderValue(state->shaders[_id].handle,  index, state->shaders[_id].locations[index].data.address, data_id);
    break;
  }
  default: {
    TraceLog(LOG_ERROR,"fshader::shader_add_uniform()::Error while setting shader value");
    return;
  }
  }
}

bool load_shader(const char *_vs_path, const char *_fs_path, shader_id _id) {
  const char *vs_path = shader_path(_vs_path);
  if (!FileExists(vs_path) && vs_path) {
    TraceLog(LOG_ERROR, "fshader::load_shader()::Vertex path does not exist");
    return false;
  }
  const char *fs_path = shader_path(_fs_path);
  if (!FileExists(fs_path) && fs_path) {
    TraceLog(LOG_ERROR, "fshader::load_shader()::Fragment path does not exist");
    return false;
  }
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "fshader::load_shader()::Shader type out of bound");
    return false;
  }

  state->shader_amouth++;
  state->shaders[_id].handle = LoadShader(vs_path, fs_path);
  state->shaders[_id].total_locations = 0;

  return true;
}

void shader_add_uniform(shader_id _id, const char *_name, ShaderUniformDataType _data_id) {
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR,
    "fshader::shader_add_attribute()::Shader type out of bound");
    return;
  }
  u16 total_loc = state->shaders[_id].total_locations;

  state->shaders[_id].locations[total_loc].index = GetShaderLocation(state->shaders[_id].handle, _name);
  TextCopy(state->shaders[_id].locations[total_loc].name, _name);
  state->shaders[_id].locations[total_loc].uni_data_type = _data_id;

  total_loc++;
  state->shaders[_id].total_locations = total_loc;
}
