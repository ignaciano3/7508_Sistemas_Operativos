#ifndef UTILS_H
#define UTILS_H

#include "defs.h"
#include "types.h"

void exit_if_syscall_failed(int syscall_return_value,
                            char *syscall_name,
                            struct cmd *command_to_free,
                            int fds_to_close[],
                            int amount_of_fds_to_close);

void report_and_clean_from_pipecmd_handler(char *syscall_name,
                                           int fds_to_close[],
                                           int amount_of_fds_to_close);

char *split_line(char *buf, char splitter);

int block_contains(char *buf, char c);

int printf_debug(char *format, ...);
int fprintf_debug(FILE *file, char *format, ...);

#endif  // UTILS_H
