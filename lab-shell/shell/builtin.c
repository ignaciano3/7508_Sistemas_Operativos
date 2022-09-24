#include "builtin.h"
#include <stdbool.h>

extern int status;
extern char prompt[PRMTLEN];

bool its_a_common_exit_cmd(char *cmd);

bool its_a_common_cd_cmd(char *cmd);

bool its_a_common_pwd_cmd(char *cmd);

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return its_a_common_exit_cmd(cmd);
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *dest_directory;

	if (its_a_common_cd_cmd(cmd)) {
		dest_directory = getenv("HOME");
	} else if (strncmp(cmd, "cd ", 3) == 0) {  // check solo de los 3 primeros
		dest_directory = cmd + 3;
	} else {
		return false;
	}

	int chdir_result = chdir(dest_directory);
	if (chdir_result == ERROR_EXIT_ID) {
		char buf_for_error[BUFLEN];
		sprintf(buf_for_error, "cannot cd to %s", dest_directory);

		status = EXIT_FAILURE;  // Set de estado de error siguiendo convencion de bash
		perror(buf_for_error);

	} else {
		char *getcwd_result = getcwd(dest_directory, PRMTLEN);

		if (getcwd_result == NULL) {
			perror("Error en [getcwd]");
			status = EXIT_FAILURE;
		} else {
			sprintf(prompt, "(%s)", getcwd_result);
		}
	}

	return true;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (its_a_common_pwd_cmd(cmd)) {
		char buf_for_cwd[PRMTLEN];
		char *getcwd_result = getcwd(buf_for_cwd, PRMTLEN);

		if (getcwd_result == NULL) {
			perror("Error en [getcwd]");
			status = EXIT_FAILURE;  // Set de estado de error siguiendo convencion de bash
		} else {
			printf("%s\n", getcwd_result);
		}

		return true;
	}
	return false;
}


// Devuelve true si el comando se trata de
// un caso común de uso del builtin exit.
// Esta deteccion es para comodidad de uso
// exclusivamente.
// Por ejemplo, los comandos: "exit", " exit" "exit "
// deberian comportase de la misma manera.
bool
its_a_common_exit_cmd(char *cmd)
{
	return ((strcmp(cmd, "exit") == 0) || (strcmp(cmd, " exit") == 0) ||
	        (strcmp(cmd, "exit ") == 0));
}

// Devuelve true si el comando se trata de
// un caso común de uso del builtin cd.
// Esta deteccion es para comodidad de uso
// exclusivamente.
// Por ejemplo, los comandos: "cd", " cd" "cd "
bool
its_a_common_cd_cmd(char *cmd)
{
	return ((strcmp(cmd, "cd") == 0) || (strcmp(cmd, " cd") == 0) ||
	        (strcmp(cmd, "cd ") == 0));
}

// Devuelve true si el comando se trata de
// un caso común de uso del builtin pwd.
// Esta deteccion es para comodidad de uso
// exclusivamente.
// Por ejemplo, los comandos: "pwd", " pwd" "pwd "
bool
its_a_common_pwd_cmd(char *cmd)
{
	return ((strcmp(cmd, "pwd") == 0) || (strcmp(cmd, " pwd") == 0) ||
	        (strcmp(cmd, "pwd ") == 0));
}
