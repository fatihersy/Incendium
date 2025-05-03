#include "logger.h"

#ifdef _WIN32
  #include <windows.h>
#endif

int alert(const char* caption, const char* message) {
  #ifndef _WIN32
    fprintf( stderr, "Message: '%s', Detail: '%s'\n", caption, message );
	  return 0;
  #else
    return ::MessageBox( NULL, caption, message, MB_OK );
  #endif
}