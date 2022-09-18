#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

void check_for_syscall_error(int syscall_return_value);

char *split_line(char *buf, char splitter);

int block_contains(char *buf, char c);

int printf_debug(char *format, ...);
int fprintf_debug(FILE *file, char *format, ...);

#endif  // UTILS_H
