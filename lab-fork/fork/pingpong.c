#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

const int ERROR_ID = -1;
const int OK_ID = 0;


int create_dual_pipes(int fds_1[], int fds_2[]);

void greet_before_fork(int fds_1[], int fds_2[]);

int am_the_child_process(int forking_result);

int handle_parent_comms(int forking_result, int fds_1[], int fds_2[]);

int handle_child_comms(int forking_result, int fds_1[], int fds_2[]);

int handle_parent_and_child_behavior(int forking_result, int fds_1[], int fds_2[]);


int
main(void)
{
	int fds_1[2];  // Padre lee de fds_1[0], Hijo escribe de fds_1[1] (Padre <--- Hijo)
	int fds_2[2];  // Hijo lee de fds_2[0], Padre escribe de fds_2[1] (Hijo <--- Padre)

	// CREACION DE PIPES
	int pipes_creation_result = create_dual_pipes(fds_1, fds_2);
	if (pipes_creation_result < OK_ID) {
		return ERROR_ID;
	};

	greet_before_fork(fds_1, fds_2);

	// SET DE LA SEED PARA RAND
	srand(time(NULL));

	// FORK
	int forking_result = fork();
	if (forking_result < OK_ID) {
		fprintf(stderr, "\n\t%s: %i\n", "Error del fork", forking_result);
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
		        "\n\t%s: %i\n",
		        "Error de creacion de fds_1 por pipe",
		        result_pipe);
		return ERROR_ID;
	};
	result_pipe = pipe(fds_2);
	if (result_pipe < OK_ID) {
		fprintf(stderr,
		        "\n\t%s: %i\n",
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
	printf("\nHola, soy PID %i:", getpid());
	printf("\n\t- primer pipe me devuelve: [%i, %i]", fds_1[0], fds_1[1]);
	printf("\n\t- segundo pipe me devuelve: [%i, %i]", fds_2[0], fds_2[1]);
}

int
am_the_child_process(int forking_result)
{
	return forking_result == 0;
}

int
handle_parent_comms(int forking_result, int fds_1[], int fds_2[])
{
	// cierro los fds que no voy a usar en este proceso (los ends del hijo)
	close(fds_2[0]);
	close(fds_1[1]);

	long msg_to_send = rand();

	printf("\n\nDonde fork me devuelve %i", forking_result);
	printf("\n\t- getpid me devuelve: %i", getpid());
	printf("\n\t- getppid me devuelve: %i", getppid());
	printf("\n\t- random me devuelve: %li", msg_to_send);
	printf("\n\t- envio valor %li a través de fd=%i", msg_to_send, fds_2[1]);

	int writing_result = write(fds_2[1], &msg_to_send, sizeof(long));
	if (writing_result < OK_ID) {
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	};

	// Ahora se lee y se bloquea hasta que responda el hijo

	long received_msg;
	int reading_result = read(fds_1[0], &received_msg, sizeof(long));
	if (reading_result < OK_ID) {
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	}

	printf("\n\nHola, de nuevo PID %i", getpid());
	printf("\n\t- recibi valor %li via fd=%i\n", received_msg, fds_1[0]);

	close(fds_1[0]);
	close(fds_2[1]);

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
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	}

	printf("\n\nDonde fork me devuelve %i:", forking_result);
	printf("\n\t- getpid me devuelve: %i", getpid());
	printf("\n\t- getppid me devuelve: %i", getppid());
	printf("\n\t- recibo valor %li via fd=%i", received_msg, fds_2[0]);
	printf("\n\t- reenvio valor en fd=%i y termino", fds_1[1]);

	int writing_result = write(fds_1[1], &received_msg, sizeof(long));
	if (writing_result < OK_ID) {
		close(fds_1[0]);
		close(fds_2[1]);
		return ERROR_ID;
	};

	close(fds_2[0]);
	close(fds_1[1]);
	return OK_ID;
}

int
handle_parent_and_child_behavior(int forking_result, int fds_1[], int fds_2[])
{
	if (!am_the_child_process(forking_result)) {  // Estoy en el proceso padre
		return handle_parent_comms(forking_result, fds_1, fds_2);

	} else {  // Estoy en el proceso hijo
		return handle_child_comms(forking_result, fds_1, fds_2);
	};
}