#include "utils.h"
#include "freecmd.h"
#include <stdarg.h>


void
exit_check_for_generic_syscall_error(int syscall_return_value,
                                     char *syscall_name,
                                     struct cmd *command_to_free)
{
	if (syscall_return_value == ERROR_EXIT_ID) {
		fprintf(stderr, "Error en syscall %s", syscall_name);
		perror("\n");
		free_command(command_to_free);
		exit(ERROR_EXIT_ID);
	}
}

// splits a string line in two
// according to the splitter character
char *
split_line(char *buf, char splitter)
{
	int i = 0;

	while (buf[i] != splitter && buf[i] != END_STRING)
		i++;

	buf[i++] = END_STRING;

	while (buf[i] == SPACE)
		i++;

	return &buf[i];
}

// looks in a block for the 'c' character
// and returns the index in which it is, or -1
// in other case
int
block_contains(char *buf, char c)
{
	for (size_t i = 0; i < strlen(buf); i++)
		if (buf[i] == c)
			return i;

	return -1;
}

// Printf wrappers for debug purposes so that they don't
// show when shell is compiled in non-interactive way
int
printf_debug(char *format, ...)
{
#ifndef SHELL_NO_INTERACTIVE
	va_list args;
	va_start(args, format);
	int ret = vprintf(format, args);
	va_end(args);

	return ret;
#else
	return 0;
#endif
}

int
fprintf_debug(FILE *file, char *format, ...)
{
#ifndef SHELL_NO_INTERACTIVE
	va_list args;
	va_start(args, format);
	int ret = vfprintf(file, format, args);
	va_end(args);

	return ret;
#else
	return 0;
#endif
}
