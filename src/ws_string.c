#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ws_string.h"

bool ws_strcasebegin(const char *str, const char *substr)
{
  return strncasecmp(str, substr, strlen(substr)) == 0;
}

#ifndef __linux__

char *ws_strndup(const char *str, size_t len)
{
  char *str_dup;

  str_dup = malloc(sizeof(*str_dup) * (len + 1));
  if (str_dup == NULL) {
    return NULL;
  }

  memcpy(str_dup, str, len * sizeof(char));
  str_dup[len] = '\0';

  return str_dup;
}

#endif

#ifndef __APPLE__

const char *ws_strnstr(const char *str, const char *substr, size_t len)
{
  size_t i;
  size_t sub_len = strlen(substr);

  if (len < sub_len) {
    return NULL;
  }

  for (i = 0; i <= len - sub_len; i++) {
    if (strncmp(str + i, substr, sub_len) == 0) {
      return str + i;
    }
  }

  return NULL;
}

#endif /* __APPLE__ */

int ws_atoin(const char *str, size_t len)
{
  size_t i;
  int value = 0;
  int m = 1;

  if (str == NULL || len == 0) {
    return 0;
  }

  for (i = len; i > 0; i--) {
    char c = str[i - 1];

    if (c < '0' || c > '9') {
      return 0;
    }

    value += m * (c - '0');
    m *= 10;
  }

  return value;
}
