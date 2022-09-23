#include "utils.h"
#include "freecmd.h"
#include <stdarg.h>

void close_all_fds(int fds_to_close[], int amount_of_fds_to_close);

void
close_all_fds(int fds_to_close[], int amount_of_fds_to_close)
{
	if (fds_to_close == NULL) {
		return;
	}

	for (int i = 0; i < amount_of_fds_to_close; i++) {
		close(fds_to_close[i]);
	}
}


// Funcion auxiliar de manejo de errores de syscalls en general.
// Si hubo una falla en la syscall especificada se libera el
// struct cmd y se cierran todos los file descriptors especificados
// por los dos ultimos parametros.
// Luego realiza llamado a exit con id de error.
void
exit_if_syscall_failed(int syscall_return_value,
                       char *syscall_name,
                       struct cmd *command_to_free,
                       int fds_to_close[],
                       int amount_of_fds_to_close)
{
	if (syscall_return_value == ERROR_EXIT_ID) {
		fprintf(stderr, "Error en syscall [%s]\n", syscall_name);
		perror("");
		close_all_fds(fds_to_close, amount_of_fds_to_close);
		free_command(command_to_free);
		exit(ERROR_EXIT_ID);
	}
}

// Funcion auxiliar encargada de cerrar todos los fds brindados en el vector,
// ademas de reportar error de syscall.
// Se utiliza solo en la funcion handler de pipe commands debido
// a que dentro de la misma no se deben realizar llamados directos a exit
// para asegurar el completo manejo de errores sin problemas de memoria/fds
// (Esto debido al comportamiento recursivo sumado a los forks)
void
report_and_clean_from_pipecmd_handler(char *syscall_name,
                                      int fds_to_close[],
                                      int amount_of_fds_to_close)
{
	fprintf(stderr, "Error en syscall [%s]\n", syscall_name);
	perror("");
	close_all_fds(fds_to_close, amount_of_fds_to_close);
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
