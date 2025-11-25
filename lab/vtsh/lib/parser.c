#include "parser.h"

#include <stdlib.h>
#include <string.h>

#include "util.h"

// ==================== токенизация ====================
//
// < и > считаются операторами только если стоят в начале
// "слова" после пробелов.

static int is_redirect_token(const char* token) {
  return strcmp(token, "<") == 0 || strcmp(token, ">") == 0 ||
         strcmp(token, ">>") == 0;
}

int vtsh_tokenize(const char* line, char* tokens[], int max_tokens) {
  int ntok = 0;
  const char* cursor = line;

  while (*cursor != '\0') {
    const char* before_spaces = cursor;
    cursor = vtsh_skip_spaces(cursor);
    if (*cursor == '\0') {
      break;
    }
    if (ntok >= max_tokens) {
      break;
    }

    int at_token_start = (cursor == line) || (cursor != before_spaces);

    if (at_token_start && (*cursor == '<' || *cursor == '>')) {
      if (*cursor == '>' && cursor[1] == '>') {
        tokens[ntok] = vtsh_xstrndup(cursor, 2U);
        if (tokens[ntok] == NULL) {
          return -1;
        }
        ++ntok;
        cursor += 2;
        continue;
      }

      tokens[ntok] = vtsh_xstrndup(cursor, 1U);
      if (tokens[ntok] == NULL) {
        return -1;
      }
      ++ntok;
      ++cursor;
      continue;
    }

    const char* start = cursor;
    while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t') {
      ++cursor;
    }

    size_t len = (size_t)(cursor - start);
    if (len > 0U) {
      tokens[ntok] = vtsh_xstrndup(start, len);
      if (tokens[ntok] == NULL) {
        return -1;
      }
      ++ntok;
    }
  }

  return ntok;
}

void vtsh_free_tokens(char* tokens[], int ntok) {
  for (int i = 0; i < ntok; ++i) {
    free(tokens[i]);
  }
}

// ==================== разбор редиректов ====================

static int handle_redirect(
    char* tokens[], int ntok, int* idx, vtsh_redirection_t* redir
) {
  char* tok = tokens[*idx];
  int is_input = (tok[0] == '<');

  // ">>" считаем синтаксической ошибкой
  if (strcmp(tok, ">>") == 0) {
    return -1;
  }

  // после < или > обязательно должен быть файл
  if (*idx + 1 >= ntok) {
    return -1;
  }

  char* fname = tokens[*idx + 1];
  if (is_redirect_token(fname)) {
    return -1;
  }

  char** slot = is_input ? &redir->input_file : &redir->output_file;
  if (*slot != NULL) {
    // второй < или >
    return -1;
  }

  *slot = fname;
  ++(*idx);
  return 0;
}

int vtsh_parse_command_tokens(
    char* tokens[],
    int ntok,
    char* argv_out[],
    int max_args,
    vtsh_redirection_t* redir
) {
  redir->input_file = NULL;
  redir->output_file = NULL;

  int argc = 0;

  for (int i = 0; i < ntok; ++i) {
    char* tok = tokens[i];

    if (is_redirect_token(tok)) {
      if (handle_redirect(tokens, ntok, &i, redir) != 0) {
        return -1;
      }
      continue;
    }

    if (argc >= max_args - 1) {
      break;
    }
    argv_out[argc++] = tok;
  }

  argv_out[argc] = NULL;
  return argc;
}
