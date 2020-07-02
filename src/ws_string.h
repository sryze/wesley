#ifndef WS_STRING_H
#define WS_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if defined _WIN32 && (defined _MSC_VER || !defined __GNUC__)
  #if !defined _MSC_VER || _MSC_VER <= 1900
    #define snprintf _snprintf
  #endif
  #define strcasecmp _stricmp
  #define strncasecmp _strnicmp
  #define strtok_r strtok_s
#endif

#ifndef __linux__
  #define strndup ws_strndup
#endif
#ifndef __APPLE__
  #define strnstr ws_strnstr
#endif
#define atoin ws_atoin

#ifndef __linux__
  char *ws_strndup(const char *str, size_t len);
#endif
#ifndef __APPLE__
  const char *ws_strnstr(const char *str, const char *substr, size_t len);
#endif
int ws_atoin(const char *str, size_t len);

#endif /* WS_STRING_H */
