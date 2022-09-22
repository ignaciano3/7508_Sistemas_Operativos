#include "exec.h"
#include <stdbool.h>

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int idx_previous_to_value = block_contains(eargv[i], '=');
		if (idx_previous_to_value > 0) {
			char env_var_key[BUFLEN], env_var_value[BUFLEN];
			get_environ_key(eargv[i], env_var_key);
			get_environ_value(eargv[i],
			                  env_var_value,
			                  idx_previous_to_value);

			int setenv_result = setenv(env_var_key,
			                           env_var_value,
			                           REPLACE_OLD_VALUE);
			if (setenv_result == ERROR_EXIT_ID) {
				perror("Error seteando env vars");
			}
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int opening_result;
	if (flags & O_CREAT) {
		opening_result = open(file, flags, S_IWUSR | S_IRUSR);
	} else {
		opening_result = open(file, flags);
	}

	return opening_result;
}

bool is_stdout_redir_required(struct execcmd *redir_command);

bool is_stdin_redir_required(struct execcmd *redir_command);

bool is_stderr_redir_required(struct execcmd *redir_command);

void redirect_stdout(struct execcmd *redir_command);

void redirect_stdin(struct execcmd *redir_command);

void redirect_stderr(struct execcmd *redir_command);

void redirect_fds_according_to_command(struct execcmd *redir_command);

void handle_all_pipe_cmds_execution(struct pipecmd *pipe_command);

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *exec_command;
	struct backcmd *back_command;
	struct execcmd *redir_command;
	struct pipecmd *pipe_command;

	switch (cmd->type) {
	case EXEC: {
		// spawns a command

		exec_command = (struct execcmd *) cmd;
		set_environ_vars(exec_command->eargv, exec_command->eargc);
		int exec_result =
		        execvp(exec_command->argv[0], exec_command->argv);
		exit_if_syscall_failed(exec_result, SYSCALL_EXEC, cmd, NULL, 0);

		break;
	}
	case BACK: {
		// runs a command in background

		back_command = (struct backcmd *) cmd;
		exec_cmd(back_command->c);

		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow

		redir_command = (struct execcmd *) cmd;
		redirect_fds_according_to_command(redir_command);

		redir_command->type = EXEC;
		exec_cmd((struct cmd *) redir_command);

		break;
	}

	case PIPE: {
		// pipes two commands

		pipe_command = (struct pipecmd *) cmd;
		handle_all_pipe_cmds_execution(pipe_command);

		// free the memory allocated
		// for the pipe tree structure
		free_command(parsed_pipe);
		exit(OK_EXIT_ID);

		break;
	}
	}
}


bool
is_stdout_redir_required(struct execcmd *redir_command)
{
	return (strlen(redir_command->out_file) > 0);
}

bool
is_stdin_redir_required(struct execcmd *redir_command)
{
	return (strlen(redir_command->in_file) > 0);
}

bool
is_stderr_redir_required(struct execcmd *redir_command)
{
	return (strlen(redir_command->err_file) > 0);
}

void
redirect_stdout(struct execcmd *redir_command)
{
	int opening_result = open_redir_fd(redir_command->out_file,
	                                   O_CREAT | O_TRUNC | O_RDWR);
	exit_if_syscall_failed(opening_result,
	                       SYSCALL_OPEN,
	                       (struct cmd *) redir_command,
	                       NULL,
	                       0);
	int fd_to_replace_stdout = opening_result;

	int dup2_result = dup2(fd_to_replace_stdout, STDOUT_FILENO);
	close(fd_to_replace_stdout);
	exit_if_syscall_failed(
	        dup2_result, SYSCALL_DUP2, (struct cmd *) redir_command, NULL, 0);
}

void
redirect_stdin(struct execcmd *redir_command)
{
	int opening_result = open_redir_fd(redir_command->in_file, O_RDONLY);
	exit_if_syscall_failed(opening_result,
	                       SYSCALL_OPEN,
	                       (struct cmd *) redir_command,
	                       NULL,
	                       0);
	int fd_to_replace_stdin = opening_result;

	int dup2_result = dup2(fd_to_replace_stdin, STDIN_FILENO);
	close(fd_to_replace_stdin);
	exit_if_syscall_failed(
	        dup2_result, SYSCALL_DUP2, (struct cmd *) redir_command, NULL, 0);
}

void
redirect_stderr(struct execcmd *redir_command)
{
	bool must_redir_to_current_fd_1 =
	        (strcmp(redir_command->err_file, "&1") == 0);
	if (must_redir_to_current_fd_1) {  //  2>&1
		int dup2_result = dup2(STDOUT_FILENO, STDERR_FILENO);
		exit_if_syscall_failed(dup2_result,
		                       SYSCALL_DUP2,
		                       (struct cmd *) redir_command,
		                       NULL,
		                       0);
	} else {
		int opening_result =
		        open_redir_fd(redir_command->err_file, O_CREAT | O_RDWR);
		exit_if_syscall_failed(opening_result,
		                       SYSCALL_OPEN,
		                       (struct cmd *) redir_command,
		                       NULL,
		                       0);
		int fd_to_replace_stderr = opening_result;

		int dup2_result = dup2(fd_to_replace_stderr, STDERR_FILENO);
		close(fd_to_replace_stderr);
		exit_if_syscall_failed(dup2_result,
		                       SYSCALL_DUP2,
		                       (struct cmd *) redir_command,
		                       NULL,
		                       0);
	}
}


void
redirect_fds_according_to_command(struct execcmd *redir_command)
{
	if (is_stdout_redir_required(redir_command)) {
		redirect_stdout(redir_command);
	}
	if (is_stdin_redir_required(redir_command)) {
		redirect_stdin(redir_command);
	}
	if (is_stderr_redir_required(redir_command)) {
		redirect_stderr(redir_command);
	}
}


void
handle_all_pipe_cmds_execution(struct pipecmd *pipe_command)
{
	int fds[2];
	int pipe_result = pipe(fds);
	if (pipe_result == ERROR_EXIT_ID) {
		report_and_clean_from_pipecmd_handler(SYSCALL_PIPE, NULL, 0);
		return;
	}

	int fork_result_for_left_cmd = fork();
	if (fork_result_for_left_cmd == ERROR_EXIT_ID) {
		report_and_clean_from_pipecmd_handler(SYSCALL_FORK, fds, 2);
		return;
	}

	if (fork_result_for_left_cmd == 0) {  // hijo izquierdo
		close(fds[READ]);

		int dup2_result = dup2(fds[WRITE], STDOUT_FILENO);
		if (dup2_result == ERROR_EXIT_ID) {
			report_and_clean_from_pipecmd_handler(SYSCALL_DUP2,
			                                      &fds[WRITE],
			                                      1);
			return;
		}

		close(fds[WRITE]);
		exec_cmd(pipe_command->leftcmd);

	} else {
		close(fds[WRITE]);

		int fork_result_for_right_cmd = fork();
		if (fork_result_for_right_cmd == ERROR_EXIT_ID) {
			waitpid(fork_result_for_left_cmd, NULL, 0);

			report_and_clean_from_pipecmd_handler(SYSCALL_FORK,
			                                      &fds[READ],
			                                      1);
			return;
		}

		if (fork_result_for_right_cmd == 0) {  // hijo derecho
			int dup2_result = dup2(fds[READ], STDIN_FILENO);
			if (dup2_result == ERROR_EXIT_ID) {
				report_and_clean_from_pipecmd_handler(
				        SYSCALL_DUP2, &fds[READ], 1);
				return;
			}

			close(fds[READ]);
			exec_cmd(pipe_command->rightcmd);


		} else {
			close(fds[READ]);
			waitpid(fork_result_for_left_cmd, NULL, 0);
			waitpid(fork_result_for_right_cmd, NULL, 0);
		}
	}
}
