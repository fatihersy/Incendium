#include "fshader.h"
#include <raylib.h>
#include <defines.h>

#include "tools/pak_parser.h"
#include "core/fmemory.h"

typedef struct shader_system_state {
  std::array<fshader, SHADER_ID_MAX> shaders;
  file_buffer null_file;

  shader_system_state(void) {
    this->shaders.fill(fshader());
    this->null_file = file_buffer();
  }
} shader_system_state;

static shader_system_state *state = nullptr;

bool load_shader(pak_file_id pak_id, i32 _vs_id, i32 _fs_id, shader_id _id);
void shader_add_uniform(shader_id _id, const char *_name, ShaderUniformDataType _data_id);

bool initialize_shader_system(void) {
  if (state and state != nullptr) {
    return true;
  }
  state = (shader_system_state *)allocate_memory_linear(sizeof(shader_system_state), true);
  if (not state or state == nullptr) {
    TraceLog(LOG_ERROR, "fshader::initialize_shader_system()::State allocation failed");
    return false;
  }
  *state = shader_system_state();

  // NOTE: _path = "%s%s", SHADER_PATH, _path
  if (not load_shader(PAK_FILE_ASSET2, PAK_FILE_ASSET2_UNDEFINED, PAK_FILE_ASSET2_PRG_BAR_MASK, SHADER_ID_PROGRESS_BAR_MASK)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::mask shader cannot loaded");
    return false;
  }
  if (not load_shader(PAK_FILE_ASSET2, PAK_FILE_ASSET2_UNDEFINED, PAK_FILE_ASSET2_FADE_TRANSITION, SHADER_ID_FADE_TRANSITION)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::fade shader cannot loaded");
    return false;
  }
  if(not load_shader(PAK_FILE_ASSET2, PAK_FILE_ASSET2_UNDEFINED, PAK_FILE_ASSET2_FONT_OUTLINE, SHADER_ID_FONT_OUTLINE)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::outline shader cannot loaded");
    return false;
  }
  if(not load_shader(PAK_FILE_ASSET2, PAK_FILE_ASSET2_UNDEFINED, PAK_FILE_ASSET2_POST_PROCESS, SHADER_ID_POST_PROCESS)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::post process shader cannot loaded");
    return false;
  }
  if(not load_shader(PAK_FILE_ASSET2, PAK_FILE_ASSET2_UNDEFINED, PAK_FILE_ASSET2_MAP_CHOICE_IMAGE, SHADER_ID_MAP_CHOICE_IMAGE)) {
    TraceLog(LOG_WARNING, "fshader::initialize_shader_system()::map choice image shader cannot loaded");
    return false;
  }

  shader_add_uniform(SHADER_ID_PROGRESS_BAR_MASK, "progress", SHADER_UNIFORM_FLOAT);
  shader_add_uniform(SHADER_ID_PROGRESS_BAR_MASK, "tint", SHADER_UNIFORM_VEC4);
  shader_add_uniform(SHADER_ID_FADE_TRANSITION, "process", SHADER_UNIFORM_FLOAT);
  //shader_add_uniform(SHADER_ID_FONT_OUTLINE, "texture_size", SHADER_UNIFORM_VEC2);
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

  return __builtin_addressof(state->shaders.at(_id));
}
i32 get_uniform_location(shader_id _id, const char * uni_name) {
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_WARNING, "fshader::get_uniform_location()::Shader type out of bound");
    return -1;
  }
  return GetShaderLocation(state->shaders.at(_id).handle, uni_name);
}

void set_shader_uniform(shader_id _id, const char* uni_name, data128 _data_pack) {
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "fshader::set_shader_uniform()::Shader type out of bound");
    return;
  }
  fshader& _shader = state->shaders.at(_id);
  i32 uni_loc = GetShaderLocation(state->shaders.at(_id).handle, uni_name);

  if (uni_loc < 0) {
    TraceLog(LOG_ERROR, "fshader::set_shader_uniform()::Shader index out of bound");
    return;
  }

  ShaderUniformDataType data_id = _shader.locations.at(uni_loc).uni_data_type;
  switch (data_id) {
  case SHADER_UNIFORM_FLOAT: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_VEC2: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_VEC3: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_VEC4: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.f32, data_id);
    break;
  }
  case SHADER_UNIFORM_INT: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.i32[0], data_id);
    break;
  }
  case SHADER_UNIFORM_IVEC2: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.i32, data_id);
    break;
  }
  case SHADER_UNIFORM_IVEC3: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.i32, data_id);
    break;
  }
  case SHADER_UNIFORM_IVEC4: {
    _shader.locations.at(uni_loc).data = _data_pack;
    SetShaderValue(state->shaders.at(_id).handle, uni_loc, &state->shaders.at(_id).locations.at(uni_loc).data.i32, data_id);
    break;
  }
  case SHADER_UNIFORM_SAMPLER2D: {
    _shader.locations.at(uni_loc).data.address[0] = _data_pack.address;
    SetShaderValue(state->shaders.at(_id).handle,  uni_loc, state->shaders.at(_id).locations.at(uni_loc).data.address, data_id);
    break;
  }
  default: {
    TraceLog(LOG_ERROR,"fshader::set_shader_uniform()::Error while setting shader value");
    return;
  }
  }
}

bool load_shader(pak_file_id pak_id, i32 _vs_id, i32 _fs_id, shader_id _id) {
  if (_id >= SHADER_ID_MAX or _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "fshader::load_shader()::Shader type out of bound");
    return false;
  }
  const file_buffer * vs_file = get_asset_file_buffer(pak_id, _vs_id);
  const file_buffer * fs_file = get_asset_file_buffer(pak_id, _fs_id);
  
  if (not vs_file or vs_file == nullptr) {
    state->shaders.at(_id).handle = LoadShaderFromMemory(0, fs_file->content.c_str());
    state->shaders.at(_id).total_locations = 0;
    return true;
  }
  else if (not fs_file or fs_file == nullptr) {
    state->shaders.at(_id).handle = LoadShaderFromMemory(vs_file->content.c_str(), 0);
    state->shaders.at(_id).total_locations = 0;
    return true;
  }

  return false;
}

void shader_add_uniform(shader_id _id, const char *_name, ShaderUniformDataType _data_id) {
  if (_id >= SHADER_ID_MAX || _id <= SHADER_ID_UNSPECIFIED) {
    TraceLog(LOG_ERROR, "fshader::shader_add_uniform()::Shader type out of bound");
    return;
  }
  const i32 uniform_loc = GetShaderLocation(state->shaders.at(_id).handle, _name);
  if (uniform_loc < 0) {
    TraceLog(LOG_ERROR, "fshader::shader_add_uniform()::Shader uniform cannot found");
    return;
  }

  state->shaders.at(_id).locations.at(uniform_loc).name = _name;
  state->shaders.at(_id).locations.at(uniform_loc).uni_data_type = _data_id;

  state->shaders.at(_id).total_locations++;
}
