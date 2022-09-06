#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>

const int ERROR_ID = -1;
const int OK_ID = 0;

int create_dual_pipes(int fds_1[], int fds_2[]);

void greet_before_fork(int fds_1[], int fds_2[]);

bool am_the_parent_process(int forking_result);

int handle_interaction_as_parent(int forking_result, int fds_1[], int fds_2[]);

int handle_interaction_as_child(int forking_result, int fds_1[], int fds_2[]);

int handle_pingpong_interactions(int forking_result, int fds_1[], int fds_2[]);


int
main(void)
{
	srand(time(NULL));

	int fds_1[2];
	int fds_2[2];

	int pipes_creation_result = create_dual_pipes(fds_1, fds_2);
	if (pipes_creation_result < OK_ID) {
		return ERROR_ID;
	};

	greet_before_fork(fds_1, fds_2);

	int forking_result = fork();
	if (forking_result < OK_ID) {
		perror("Error del fork");
		close(fds_1[0]);
		close(fds_1[1]);
		close(fds_2[0]);
		close(fds_2[1]);
		return ERROR_ID;
	};

	return handle_pingpong_interactions(forking_result, fds_1, fds_2);
}


// ================================================================================

int
create_dual_pipes(int fds_1[], int fds_2[])
{
	int result_pipe = pipe(fds_1);
	if (result_pipe < OK_ID) {
		perror("Error de creacion de fds_1 por pipe");
		return ERROR_ID;
	};
	result_pipe = pipe(fds_2);
	if (result_pipe < OK_ID) {
		perror("Error de creacion de fds_2 por pipe");
		close(fds_1[0]);
		close(fds_1[1]);
		return ERROR_ID;
	};

	return OK_ID;
}

void
greet_before_fork(int fds_1[], int fds_2[])
{
	printf("Hola, soy PID %i:", getpid());
	printf("\n");
	printf("  - primer pipe me devuelve: [%i, %i]\n", fds_1[0], fds_1[1]);
	printf("  - segundo pipe me devuelve: [%i, %i]\n", fds_2[0], fds_2[1]);
}

bool
am_the_parent_process(int forking_result)
{
	return forking_result != 0;
}

int
handle_interaction_as_parent(int forking_result, int fds_1[], int fds_2[])
{
	// cierro los fds que no voy a usar en este proceso (los ends del hijo)
	close(fds_1[0]);
	close(fds_2[1]);

	long msg_to_send = rand();

	printf("\n");
	printf("Donde fork me devuelve %i:\n", forking_result);
	printf("  - getpid me devuelve: %i\n", getpid());
	printf("  - getppid me devuelve: %i\n", getppid());
	printf("  - random me devuelve: %li\n", msg_to_send);
	printf("  - envío valor %li a través de fd=%i\n", msg_to_send, fds_1[1]);

	int writing_result = write(fds_1[1], &msg_to_send, sizeof(long));
	if (writing_result < OK_ID) {
		perror("Error writing: ");
		close(fds_2[0]);
		close(fds_1[1]);
		return ERROR_ID;
	};

	// Ahora se lee y se bloquea hasta que responda el hijo

	long received_msg;
	int reading_result = read(fds_2[0], &received_msg, sizeof(long));
	if (reading_result < OK_ID) {
		perror("Error reading: ");
		close(fds_2[0]);
		close(fds_1[1]);
		return ERROR_ID;
	} else if (reading_result == OK_ID) {
		close(fds_2[0]);
		close(fds_1[1]);
		return ERROR_ID;
	}

	printf("\n");
	printf("Hola, de nuevo PID %i:\n", getpid());
	printf("  - recibí valor %li vía fd=%i\n", received_msg, fds_2[0]);

	close(fds_2[0]);
	close(fds_1[1]);

	int waiting_result = wait(NULL);
	if (waiting_result == ERROR_ID) {
		perror("Error waiting for child: ");
		return ERROR_ID;
	}

	return OK_ID;
}

int
handle_interaction_as_child(int forking_result, int fds_1[], int fds_2[])
{
	// cierro los fds que no voy a usar en este proceso (los ends del padre)
	close(fds_2[0]);
	close(fds_1[1]);

	// recibo el mensaje (bloqueo aca para que esto espere hasta despues de que el padre me contacte)
	long received_msg;
	int reading_result = read(fds_1[0], &received_msg, sizeof(long));
	if (reading_result < OK_ID) {
		perror("Error reading: ");
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	} else if (reading_result == OK_ID) {
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	}

	printf("\n");
	printf("Donde fork me devuelve %i:\n", forking_result);
	printf("  - getpid me devuelve: %i\n", getpid());
	printf("  - getppid me devuelve: %i\n", getppid());
	printf("  - recibo valor %li vía fd=%i\n", received_msg, fds_1[0]);
	printf("  - reenvío valor en fd=%i y termino\n", fds_2[1]);

	int writing_result = write(fds_2[1], &received_msg, sizeof(long));
	if (writing_result < OK_ID) {
		perror("Error writing: ");
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	};

	close(fds_1[0]);
	close(fds_2[1]);
	return OK_ID;
}

// HIJO lee de fds_1[0], PADRE escribe de fds_1[1] (HIJO <--- PADRE)
// PADRE lee de fds_2[0], HIJO escribe de fds_2[1] (PADRE <--- HIJO)
int
handle_pingpong_interactions(int forking_result, int fds_1[], int fds_2[])
{
	if (am_the_parent_process(forking_result)) {
		return handle_interaction_as_parent(forking_result, fds_1, fds_2);

	} else {  // Estoy en el proceso hijo
		return handle_interaction_as_child(forking_result, fds_1, fds_2);
	};
}