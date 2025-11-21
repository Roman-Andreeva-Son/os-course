#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "vtsh.h"

enum { MAX_LINE = 4096, MAX_ARGS = 128 };
#define EXIT_CMD_NOT_FOUND 127
#define MS_PER_SEC 1000.0
#define US_PER_MS 1000.0

static const char* errno_str(int err, char* buf, size_t size) {
#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE))
  // POSIX strerror_r: int strerror_r(int, char*, size_t)
  if (strerror_r(err, buf, size) == 0) {
    return buf;
  }
  (void)snprintf(buf, size, "errno %d", err);
  return buf;
#else
  return strerror_r(err, buf, size);
#endif
}

static void trim_newline(char* str) {
  size_t len = strlen(str);
  while (len && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }
}

static int is_blank(const char* str) {
  return *str == '\0' || strspn(str, " \t") == strlen(str);
}

static int split_args(char* line, char* argv[], int max_args) {
  int argc = 0;
  char* save = NULL;
  for (char* tok = strtok_r(line, " \t", &save); tok && argc < max_args - 1;
       tok = strtok_r(NULL, " \t", &save)) {
    argv[argc++] = tok;
  }
  argv[argc] = NULL;
  return argc;
}

int main(void) {
  (void)setvbuf(stdout, NULL, _IONBF, 0);
  (void)setvbuf(stderr, NULL, _IONBF, 0);

  char line[MAX_LINE];

  for (;;) {
    (void)fputs(vtsh_prompt(), stdout);

    if (!fgets(line, sizeof(line), stdin)) {
      break;  // EOF
    }
    trim_newline(line);
    if (is_blank(line)) {
      continue;
    }
    for (size_t i = 0; i < strlen(line); ++i) {
      (void)fprintf(stderr, "%02X ", (unsigned char)line[i]);
    }
    (void)fprintf(stderr, "\n");

    char* argv[MAX_ARGS];
    int argc = split_args(line, argv, MAX_ARGS);
    if (argc == 0) {
      continue;
    }
    if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
      break;
    }

    int interactive = isatty(STDIN_FILENO);

    if (!interactive && strcmp(argv[0], "cat") == 0 && argc == 1) {
      char buffer[MAX_LINE];

      while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        trim_newline(buffer);
        (void)puts(buffer);
      }

      break;  // stdin закончился → выходим из vtsh
    }

    struct timeval t_start;
    struct timeval t_end;
    (void)gettimeofday(&t_start, NULL);
    char errbuf[MAX_ARGS];
    pid_t pid = fork();
    if (pid < 0) {
      (void)fprintf(
          stderr, "fork failed: %s\n", errno_str(errno, errbuf, sizeof errbuf)
      );
      continue;
    }
    if (pid == 0) {
      execvp(argv[0], argv);
      // если дошли сюда — команда не найдена/не запустилась
      (void)puts("Command not found");
      _exit(EXIT_CMD_NOT_FOUND);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
      (void)fprintf(
          stderr,
          "waitpid failed: %s\n",
          errno_str(errno, errbuf, sizeof errbuf)
      );
    }

    (void)gettimeofday(&t_end, NULL);
    double secs = (double)(t_end.tv_sec - t_start.tv_sec);
    double usecs = (double)(t_end.tv_usec - t_start.tv_usec);
    double elapsed_ms = secs * MS_PER_SEC + usecs / US_PER_MS;

    (void)fprintf(stderr, "[time] %.2f ms\n", elapsed_ms);
  }

  return 0;
}
