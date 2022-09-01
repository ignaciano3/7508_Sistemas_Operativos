#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

const int ERROR_ID = -1;
const int OK_ID = 0;

int
main(void)
{
	int file_descriptor_1[2];  // Padre lee de fds_1[0], Hijo escribe de fds_1[1] (Hijo ---> Padre)
	int file_descriptor_2[2];  // Hijo lee de fds_2[0], Padre escribe de fds_2[1] (Padre ---> Hijo)

	// CREACION DE PIPES
	int result_pipe = pipe(file_descriptor_1);
	if (result_pipe < OK_ID) {
		fprintf(stderr,
		        "\n\t%s: %i\n",
		        "Error de creacion de fds_1 por pipe",
		        result_pipe);
		return ERROR_ID;
	};
	result_pipe = pipe(file_descriptor_2);
	if (result_pipe < OK_ID) {
		fprintf(stderr,
		        "\n\t%s: %i\n",
		        "Error de creacion de fds_2 por pipe",
		        result_pipe);
		close(file_descriptor_1[0]);
		close(file_descriptor_1[1]);
		return ERROR_ID;
	};

	const int pid_padre = getpid();
	printf("\nHola, soy PID <%i>:", pid_padre);
	printf("\n\t - primer pipe me devuelve: [%i, %i]",
	       file_descriptor_1[0],
	       file_descriptor_1[1]);
	printf("\n\t - segundo pipe me devuelve: [%i, %i]",
	       file_descriptor_2[0],
	       file_descriptor_2[1]);


	// SET DE LA SEED PARA RANDOM
	srandom(time(NULL));

	// FORK
	int result_fork = fork();
	if (result_fork < OK_ID) {
		fprintf(stderr, "\n\t%s: %i\n", "Error del fork", result_fork);
		close(file_descriptor_1[0]);
		close(file_descriptor_1[1]);
		close(file_descriptor_2[0]);
		close(file_descriptor_2[1]);
		return ERROR_ID;
	};

	// RANDOM DENTRO DE CADA PROCESO
	long msg = random();

	// if () {
	// };


	return OK_ID;
}