#include "logger.h"
#include <raylib.h>
#include <vector>

#include "core/fmemory.h"

#ifdef _RELEASE
  #define LOGGING_SEVERITY LOG_SEV_ERROR
#else
  #define LOGGING_SEVERITY LOG_SEV_WARNING
#endif

#define LOG_FILE_DIRECTORY "logs"

typedef struct logging_system_state {
  int build_id;
  std::vector<std::string> logs;
  std::string log_file_name;
  std::string last_writed;
  std::string dump_string;
  logging_system_state(void) {
    this->logs = std::vector<std::string>();
    this->log_file_name = std::string();
    this->last_writed = std::string();
    this->dump_string = std::string();
  }
} logging_system_state;

static logging_system_state * state = nullptr;

TraceLogLevel to_rl_log_level(logging_severity sev) {
  switch (sev) {
    case LOG_SEV_TRACE: return TraceLogLevel::LOG_TRACE;
    case LOG_SEV_DEBUG: return TraceLogLevel::LOG_DEBUG;
    case LOG_SEV_INFO: return TraceLogLevel::LOG_INFO;
    case LOG_SEV_WARNING: return TraceLogLevel::LOG_WARNING;
    case LOG_SEV_ERROR: return TraceLogLevel::LOG_ERROR;
    case LOG_SEV_FATAL: return TraceLogLevel::LOG_FATAL;
    default: {
      return TraceLogLevel::LOG_NONE;
    }
  }
}

bool logging_system_initialize(int build_id) {
  if (state and state != nullptr) {
    return false;
  }
  state = (logging_system_state*)allocate_memory_linear(sizeof(logging_system_state), true);
  if (not state or state == nullptr) {
    return false;
  }
  *state = logging_system_state();
  state->build_id = build_id;

  char timeStr[11] = { 0 };
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(timeStr, sizeof(timeStr), "%d_%m_%Y", tm_info);

  if (not DirectoryExists(LOG_FILE_DIRECTORY)) {
    MakeDirectory(LOG_FILE_DIRECTORY);
  }
  state->log_file_name = TextFormat("%s/%s.txt", LOG_FILE_DIRECTORY, timeStr);

  if (not FileExists(state->log_file_name.c_str())) {
    SaveFileText(state->log_file_name.c_str(), " ");
  }
  return true;
}

void logging_system_shutdown(void) {
  if (not state or state == nullptr) {
    return;
  }
  for (std::string str : state->logs) {
		str.clear();
		str.shrink_to_fit();
	}
  state->logs.clear();
	state->logs.shrink_to_fit();
	
  free(state);
  state = nullptr;
}

void inc_logging(logging_severity ls, const char* fmt, ...) {
  if(not state or state == nullptr) {
		return;
  }
  std::string out_log = std::string();
  char timeStr[64] = { 0 };
  time_t now = time(NULL);

  struct tm *tm_info = localtime(&now);
  strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S", tm_info);
  out_log.append(timeStr);

  switch (ls)
  {
    case LOG_SEV_INFO :    out_log.append("::INFO] :"); break;
    case LOG_SEV_WARNING : out_log.append("::WARN] :"); break;
    case LOG_SEV_ERROR :   out_log.append("::ERROR]:"); break;
    case LOG_SEV_FATAL :   out_log.append("::FATAL]:"); break;
    default: break;
  }
  out_log.append(TextFormat("bID: %d -- ", state->build_id));

  __builtin_va_list arg_ptr;
  va_start(arg_ptr, fmt); 

  // https://stackoverflow.com/questions/19009094/c-variable-arguments-with-stdstring-only
  // reliably acquire the size from a copy of
  // the variable argument array
  // and a functionally reliable call
  // to mock the formatting
  va_list arg_ptr_copy;
  va_copy(arg_ptr_copy, arg_ptr);
  const int iLen = std::vsnprintf(NULL, 0, fmt, arg_ptr_copy);
  va_end(arg_ptr_copy);

  // return a formatted string without
  // risking memory mismanagement
  // and without assuming any compiler
  // or platform specific behavior
  std::vector<char> zc(iLen + 1);
  std::vsnprintf(zc.data(), zc.size(), fmt, arg_ptr);
  va_end(arg_ptr);

  out_log.append(std::string(zc.data(), zc.size()-1));
	std::string& log = state->logs.emplace_back(out_log);

  if (log == state->last_writed) {
    return;
  }
	if (ls >= LOGGING_SEVERITY) {
    log.append("\n");
    if (FileExists(state->log_file_name.c_str())) {
      char * logged_text = LoadFileText(state->log_file_name.c_str());
      if (TextLength(logged_text) <= 1) {
        SaveFileText(state->log_file_name.c_str(), log.c_str());
      }
      else {
        SaveFileText(state->log_file_name.c_str(), TextFormat("%s%s", logged_text, log.c_str()));
      }
      UnloadFileText(logged_text);
    }
    else {
      SaveFileText(state->log_file_name.c_str(), TextFormat("%s", log.c_str()));
    }
	} else {
    #ifndef _RELEASE 
    TraceLog(to_rl_log_level(ls), log.c_str());
    #endif
  }
}

const char * get_last_log(void) {
  if (not state or state == nullptr) {
    return nullptr;
  }
  state->dump_string = std::string();

  if (not state->logs.empty()) {
    state->dump_string = state->logs.back();
  }

  return state->dump_string.c_str();
}

