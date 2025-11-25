#pragma once

// инфа о редиректах
typedef struct {
  char* input_file;
  char* output_file;
} vtsh_redirection_t;

// токенизация строки команд
int vtsh_tokenize(const char* line, char* tokens[], int max_tokens);

// освободить все токены (каждый выделен через malloc)
void vtsh_free_tokens(char* tokens[], int ntok);

// разобрать токены в argv + редиректы
// возвращает argc или -1 при синтаксической ошибке
int vtsh_parse_command_tokens(
    char* tokens[],
    int ntok,
    char* argv_out[],
    int max_args,
    vtsh_redirection_t* redir
);
