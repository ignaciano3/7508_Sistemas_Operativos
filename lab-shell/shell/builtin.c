#include "builtin.h"

extern int status;
extern char prompt[PRMTLEN];

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return (strcmp(cmd, "exit") == 0);
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
	if (strcmp(cmd, "cd\0") == 0) {
		dest_directory = getenv("HOME");
	} else if (strncmp(cmd, "cd ", 3) == 0) {
		dest_directory = cmd + 3;
	} else {
		return 0;
	}

	int chdir_result = chdir(dest_directory);
	if (chdir_result < 0) {
		char buf[BUFLEN];
		snprintf(buf, strlen(buf), "cannot cd to %s ", dest_directory);
		status = 1;
		perror(buf);
	} else {
		snprintf(prompt,
		         strlen(prompt),
		         "%s",
		         getcwd(dest_directory, PRMTLEN));
	}

	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char buf[PRMTLEN];
		printf("%s\n", getcwd(buf, PRMTLEN));
		status = 1;
		return 1;
	}
	return 0;
}
