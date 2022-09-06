#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#ifndef NARGS
#define NARGS 4
#endif

const int ERROR_ID = -1;
const int OK_ID = 0;

// argv siempre va a venir con dos posiciones ocupadas: (argv[0] == "./xargs.o")
// y (argv[1] == "<comando a ejecutar>")
// Por lo tanto en main se almacena en args_for_the_required_command[0] dicho comando
// y a partir de ahi se accede al resto de buffers almacenados por getline con este OFFSET.
#define OFFSET_FOR_COMMAND 1
const int MIN_ARGS = 2;

#define OFFSET_FOR_NULL_TERMINATION 1

int collect_all_args_and_execute_commands(char **args_for_the_required_command,
                                          char *required_command);

int execute_command_with_args(char **args_for_the_required_command,
                              int amount_of_args,
                              char *required_command);

bool line_contains_newline_char(char **args, int position_of_line, int line_length);

void replace_newline_to_null(char **args, int position_of_line, int line_length);

bool stored_enough_args_for_execution(int amount_of_args);

void free_last_getline_buf_only(char **args, int amount_of_args);

void free_all_stored_args(char **args, int amount_of_args);

int
main(int argc, char *argv[])
{
	if (argc < MIN_ARGS) {
		printf("Error: se necesita al menos un argumento (el comando a "
		       "ejecutar)\n");
		return ERROR_ID;
	}

	char *required_command = argv[1];

	// +OFFSET para el comando como tal y +1 para que termine en null siempre para asegurar uso correcto de memoria
	char *args_for_the_required_command[NARGS + OFFSET_FOR_COMMAND +
	                                    OFFSET_FOR_NULL_TERMINATION] = { NULL };
	args_for_the_required_command[0] = required_command;


	int execution_result = collect_all_args_and_execute_commands(
	        args_for_the_required_command, required_command);
	if (execution_result == ERROR_ID) {
		return ERROR_ID;
	}

	return OK_ID;
}


int
collect_all_args_and_execute_commands(char **args_for_the_required_command,
                                      char *required_command)
{
	size_t allocated_bytes_for_line = 0;
	int amount_of_stored_lines = 0;

	int line_length =
	        getline(&args_for_the_required_command[amount_of_stored_lines +
	                                               OFFSET_FOR_COMMAND],
	                &allocated_bytes_for_line,
	                stdin);

	while (line_length != ERROR_ID) {
		amount_of_stored_lines++;

		if (line_contains_newline_char(args_for_the_required_command,
		                               amount_of_stored_lines,
		                               line_length)) {
			replace_newline_to_null(args_for_the_required_command,
			                        amount_of_stored_lines,
			                        line_length);
		}

		if (stored_enough_args_for_execution(amount_of_stored_lines)) {
			int execution_result = execute_command_with_args(
			        args_for_the_required_command,
			        amount_of_stored_lines,
			        required_command);
			free_all_stored_args(args_for_the_required_command,
			                     amount_of_stored_lines);
			if (execution_result == ERROR_ID) {
				return ERROR_ID;
			}
			amount_of_stored_lines = 0;
		}

		line_length = getline(
		        &args_for_the_required_command[amount_of_stored_lines +
		                                       OFFSET_FOR_COMMAND],
		        &allocated_bytes_for_line,
		        stdin);
		if (line_length == ERROR_ID) {
			free_last_getline_buf_only(args_for_the_required_command,
			                           amount_of_stored_lines +
			                                   OFFSET_FOR_COMMAND);  // Por documentacion, aún cuando getline falla hace alloc de un buffer entonces hay que liberarlo
		}
	}

	if (amount_of_stored_lines >
	    0) {  // Si se finalizó pero quedaron argumentos leidos, falta ejecutar el comando una ultima vez
		int execution_result =
		        execute_command_with_args(args_for_the_required_command,
		                                  amount_of_stored_lines,
		                                  required_command);
		free_all_stored_args(args_for_the_required_command,
		                     amount_of_stored_lines);
		if (execution_result == ERROR_ID) {
			return ERROR_ID;
		}
	}
	return OK_ID;
}

int
execute_command_with_args(char **args_for_the_required_command,
                          int amount_of_args,
                          char *required_command)
{
	int forking_result = fork();
	if (forking_result < 0) {
		perror("Error en fork");
		return ERROR_ID;
	}

	if (forking_result > 0) {
		int waiting_result = wait(NULL);
		free_all_stored_args(args_for_the_required_command,
		                     amount_of_args);
		if (waiting_result == ERROR_ID) {
			perror("Error en wait");
			return ERROR_ID;
		}
		return OK_ID;
	} else {
		int exec_result =
		        execvp(required_command, args_for_the_required_command);
		if (exec_result == ERROR_ID) {
			perror("Error en exec");
			return ERROR_ID;
		}
		return OK_ID;
	}
}

bool
line_contains_newline_char(char **args, int position_of_line, int line_length)
{
	int position_of_newline_char = line_length - 1;
	return (args[position_of_line][position_of_newline_char] == '\n');
}

void
replace_newline_to_null(char **args, int position_of_line, int line_length)
{
	int position_of_newline_char = line_length - 1;
	args[position_of_line][position_of_newline_char] = '\0';
}

bool
stored_enough_args_for_execution(int amount_of_args)
{
	return (amount_of_args == NARGS);
}

void
free_last_getline_buf_only(char **args, int amount_of_args)
{
	free(args[amount_of_args]);
	args[amount_of_args] = NULL;
}

void
free_all_stored_args(char **args, int amount_of_args)
{
	int total_to_free = amount_of_args + OFFSET_FOR_COMMAND +
	                    OFFSET_FOR_NULL_TERMINATION;
	for (int j = 1; j < total_to_free; j++) {
		free(args[j]);
		args[j] = NULL;
	}
}