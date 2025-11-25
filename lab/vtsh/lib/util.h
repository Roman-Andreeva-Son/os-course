#pragma once

#include <stddef.h>

// errno -> строка
const char* vtsh_errno_str(int err, char* buf, size_t size);

// убрать \n / \r в конце строки
void vtsh_trim_newline(char* str);

// пустая строка или только пробелы/табуляции?
int vtsh_is_blank(const char* str);

// безопасный strndup
char* vtsh_xstrndup(const char* src, size_t len);

// пропустить пробелы и табы
const char* vtsh_skip_spaces(const char* s);
