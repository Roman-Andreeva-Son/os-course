#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* vtsh_errno_str(int err, char* buf, size_t size) {
#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE))
  if (strerror_r(err, buf, size) == 0) {
    return buf;
  }
  (void)snprintf(buf, size, "errno %d", err);
  return buf;
#else
  return strerror_r(err, buf, size);
#endif
}

void vtsh_trim_newline(char* str) {
  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }
}

int vtsh_is_blank(const char* str) {
  return *str == '\0' || strspn(str, " \t") == strlen(str);
}

char* vtsh_xstrndup(const char* src, size_t len) {
  char* res = (char*)malloc(len + 1U);
  if (res == NULL) {
    return NULL;
  }
  memcpy(res, src, len);
  res[len] = '\0';
  return res;
}

const char* vtsh_skip_spaces(const char* s) {
  while (*s == ' ' || *s == '\t') {
    ++s;
  }
  return s;
}
