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

int handle_parent_comms(int forking_result, int fds_1[], int fds_2[]);

int handle_child_comms(int forking_result, int fds_1[], int fds_2[]);

int handle_parent_and_child_behavior(int forking_result, int fds_1[], int fds_2[]);


int
main(void)
{
	// SET DE LA SEED PARA RAND
	srand(time(NULL));

	int fds_1[2];  // Padre lee de fds_1[0], Hijo escribe de fds_1[1] (Padre <--- Hijo)
	int fds_2[2];  // Hijo lee de fds_2[0], Padre escribe de fds_2[1] (Hijo <--- Padre)

	// CREACION DE PIPES
	int pipes_creation_result = create_dual_pipes(fds_1, fds_2);
	if (pipes_creation_result < OK_ID) {
		return ERROR_ID;
	};

	greet_before_fork(fds_1, fds_2);

	// FORK
	int forking_result = fork();
	if (forking_result < OK_ID) {
		fprintf(stderr, "\n  %s: %i\n", "Error del fork", forking_result);
		close(fds_1[0]);
		close(fds_1[1]);
		close(fds_2[0]);
		close(fds_2[1]);
		return ERROR_ID;
	};

	return handle_parent_and_child_behavior(forking_result, fds_1, fds_2);
}

// Problema actual: es como que permanece el mensaje de la segunda impresion de
// pipes en el greet, o sea que hay que correr los dos ultimos prints de cada
// una de las dos ultimas categorias para arriba

// ================================================================================

int
create_dual_pipes(int fds_1[], int fds_2[])
{
	int result_pipe = pipe(fds_1);
	if (result_pipe < OK_ID) {
		fprintf(stderr,
		        "\n  %s: %i\n",
		        "Error de creacion de fds_1 por pipe",
		        result_pipe);
		return ERROR_ID;
	};
	result_pipe = pipe(fds_2);
	if (result_pipe < OK_ID) {
		fprintf(stderr,
		        "\n  %s: %i\n",
		        "Error de creacion de fds_2 por pipe",
		        result_pipe);
		close(fds_1[0]);
		close(fds_1[1]);
		return ERROR_ID;
	};

	return OK_ID;
}

void
greet_before_fork(int fds_1[], int fds_2[])
{
	printf("\nHola, soy PID %i:\n", getpid());
	printf("- primer pipe me devuelve: [%i, %i]\n", fds_1[0], fds_1[1]);
	printf("- segundo pipe me devuelve: [%i, %i]\n", fds_2[0], fds_2[1]);
}

bool
am_the_parent_process(int forking_result)
{
	return forking_result != 0;
}

int
handle_parent_comms(int forking_result, int fds_1[], int fds_2[])
{
	// cierro los fds que no voy a usar en este proceso (los ends del hijo)
	close(fds_2[0]);
	close(fds_1[1]);

	long msg_to_send = rand();

	printf("\n\n");
	printf("Donde fork me devuelve %i\n", forking_result);
	printf("- getpid me devuelve: %i\n", getpid());
	printf("- getppid me devuelve: %i\n", getppid());
	printf("- random me devuelve: %li\n", msg_to_send);
	printf("- envio valor %li a través de fd=%i\n", msg_to_send, fds_2[1]);

	int writing_result = write(fds_2[1], &msg_to_send, sizeof(long));
	if (writing_result < OK_ID) {
		perror("Error writing: ");
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	};

	// Ahora se lee y se bloquea hasta que responda el hijo

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
	printf("Hola, de nuevo PID %i:\n", getpid());
	printf("- recibi valor %li via fd=%i\n", received_msg, fds_1[0]);

	close(fds_1[0]);
	close(fds_2[1]);

	int waiting_result = wait(NULL);
	if (waiting_result == ERROR_ID) {
		perror("Error waiting for child: ");
		return ERROR_ID;
	}

	return OK_ID;
}

int
handle_child_comms(int forking_result, int fds_1[], int fds_2[])
{
	// cierro los fds que no voy a usar en este proceso (los ends del padre)
	close(fds_1[0]);
	close(fds_2[1]);

	// recibo el mensaje (bloqueo aca para que esto espere hasta despues de que el padre me contacte)
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
	printf("Donde fork me devuelve %i:\n", forking_result);
	printf("- getpid me devuelve: %i\n", getpid());
	printf("- getppid me devuelve: %i\n", getppid());
	printf("- recibo valor %li via fd=%i\n", received_msg, fds_2[0]);
	printf("- reenvio valor en fd=%i y termino\n", fds_1[1]);

	int writing_result = write(fds_1[1], &received_msg, sizeof(long));
	if (writing_result < OK_ID) {
		perror("Error writing: ");
		close(fds_2[0]);
		close(fds_1[1]);
		return ERROR_ID;
	};

	close(fds_2[0]);
	close(fds_1[1]);
	return OK_ID;
}

int
handle_parent_and_child_behavior(int forking_result, int fds_1[], int fds_2[])
{
	if (am_the_parent_process(forking_result)) {
		return handle_parent_comms(forking_result, fds_1, fds_2);

	} else {  // Estoy en el proceso hijo
		return handle_child_comms(forking_result, fds_1, fds_2);
	};
}