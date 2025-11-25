#include "shell.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"
#include "util.h"
#include "vtsh.h"

enum { MAX_LINE = 4096, MAX_ARGS = 128, MAX_TOKENS = 256 };

#define EXIT_CMD_NOT_FOUND 127
#define MS_PER_SEC 1000.0
#define US_PER_MS 1000.0
#define FILE_MODE_DEFAULT 0644
#define ERRBUF_SIZE 128

void run_shell(void) {
  (void)setvbuf(stdout, NULL, _IONBF, 0);
  (void)setvbuf(stderr, NULL, _IONBF, 0);
  (void)setvbuf(stdin, NULL, _IONBF, 0);

  char line[MAX_LINE];

  for (;;) {
    (void)fputs(vtsh_prompt(), stdout);

    if (fgets(line, sizeof(line), stdin) == NULL) {
      break;
    }

    vtsh_trim_newline(line);
    if (vtsh_is_blank(line)) {
      continue;
    }

    // 1) токены
    char* tokens[MAX_TOKENS];
    int ntok = vtsh_tokenize(line, tokens, MAX_TOKENS);
    if (ntok < 0) {
      break;
    }
    if (ntok == 0) {
      vtsh_free_tokens(tokens, ntok);
      continue;
    }

    // 2) редиректы + argv
    char* argv[MAX_ARGS];
    vtsh_redirection_t redir = {.input_file = NULL, .output_file = NULL};
    int argc = vtsh_parse_command_tokens(tokens, ntok, argv, MAX_ARGS, &redir);

    char* input_file = redir.input_file;
    char* output_file = redir.output_file;

    if (argc < 0) {
      (void)puts("Syntax error");
      vtsh_free_tokens(tokens, ntok);
      continue;
    }

    if (argc == 0) {
      vtsh_free_tokens(tokens, ntok);
      continue;
    }

    if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "quit") == 0) {
      vtsh_free_tokens(tokens, ntok);
      break;
    }

    // проверяем '&' в конце
    int run_in_background = 0;
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
      run_in_background = 1;
      argv[argc - 1] = NULL;
      --argc;
      if (argc == 0) {
        vtsh_free_tokens(tokens, ntok);
        continue;
      }
    }

    int interactive = isatty(STDIN_FILENO);

    if (!interactive && !run_in_background && argc == 1 &&
        strcmp(argv[0], "cat") == 0 && input_file == NULL &&
        output_file == NULL) {
      char buffer[MAX_LINE];
      while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        vtsh_trim_newline(buffer);
        (void)puts(buffer);
      }
      vtsh_free_tokens(tokens, ntok);
      break;
    }

    struct timeval t_start;
    if (!run_in_background) {
      (void)gettimeofday(&t_start, NULL);
    }

    char errbuf[ERRBUF_SIZE];
    pid_t pid = fork();
    if (pid < 0) {
      (void)fprintf(
          stderr,
          "fork failed: %s\n",
          vtsh_errno_str(errno, errbuf, sizeof errbuf)
      );
      vtsh_free_tokens(tokens, ntok);
      continue;
    }

    if (pid == 0) {
      // дочерний процесс

      if (input_file != NULL) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) {
          (void)puts("I/O error");
          _exit(EXIT_FAILURE);
        }
        if (dup2(fd_in, STDIN_FILENO) < 0) {
          (void)close(fd_in);
          (void)puts("I/O error");
          _exit(EXIT_FAILURE);
        }
        (void)close(fd_in);
      }

      if (output_file != NULL) {
        unsigned flags =
            (unsigned)O_WRONLY | (unsigned)O_CREAT | (unsigned)O_TRUNC;
        int fd_out = open(output_file, (int)flags, FILE_MODE_DEFAULT);

        if (fd_out < 0) {
          (void)puts("I/O error");
          _exit(EXIT_FAILURE);
        }
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
          (void)close(fd_out);
          (void)puts("I/O error");
          _exit(EXIT_FAILURE);
        }
        (void)close(fd_out);
      }

      execvp(argv[0], argv);
      (void)puts("Command not found");
      _exit(EXIT_CMD_NOT_FOUND);
    }

    if (run_in_background) {
      (void)printf("[%d]\n", (int)pid);
      vtsh_free_tokens(tokens, ntok);
      continue;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
      (void)fprintf(
          stderr,
          "waitpid failed: %s\n",
          vtsh_errno_str(errno, errbuf, sizeof errbuf)
      );
    }

    struct timeval t_end;
    (void)gettimeofday(&t_end, NULL);
    double secs = (double)(t_end.tv_sec - t_start.tv_sec);
    double usecs = (double)(t_end.tv_usec - t_start.tv_usec);
    double elapsed_ms = secs * MS_PER_SEC + usecs / US_PER_MS;

    (void)fprintf(stderr, "[time] %.2f ms\n", elapsed_ms);

    vtsh_free_tokens(tokens, ntok);
  }
}
