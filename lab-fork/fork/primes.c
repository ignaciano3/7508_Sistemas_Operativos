#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>


const int ERROR_ID = -1;
const int OK_ID = 0;


bool am_the_current_parent_process(int forking_result);

int handle_myself_as_absolute_parent(int max_number_required, int sender_fds);

int handle_my_relative_child(int receiver_fds);

int handle_myself_as_relative_parent(int receiver_fds,
                                     int sender_fds,
                                     int first_number_received);

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("\nError: Necesito un numero natural >= 2 como "
		       "argumento\n");
		return ERROR_ID;
	}
	int max_number_required = strtol(argv[1], NULL, 10);
	if (max_number_required < 2) {
		printf("\nError: Necesito un numero natural >= 2 como "
		       "argumento\n");
		return ERROR_ID;
	};

	int fds[2];

	int result_pipe = pipe(fds);
	if (result_pipe < OK_ID) {
		perror("Error de creacion de fds por pipe");
		return ERROR_ID;
	};

	int forking_result = fork();
	if (forking_result < OK_ID) {
		perror("Error del fork");
		close(fds[0]);
		close(fds[1]);
		return ERROR_ID;
	};

	if (am_the_current_parent_process(forking_result)) {
		// Cierro el punto de lectura que no voy a usar (soy el proceso "de la izquierda de todo")
		close(fds[0]);
		int parent_result =
		        handle_myself_as_absolute_parent(max_number_required,
		                                         fds[1]);
		close(fds[1]);

		int waiting_result = wait(NULL);
		if (waiting_result == ERROR_ID) {
			perror("Error waiting for child");
			return ERROR_ID;
		} else if (parent_result < OK_ID) {
			return ERROR_ID;
		}

	} else {  // Estoy en el proceso hijo
		// Cierro el punto de escritura ya que no me voy a recomunicar con el padre
		close(fds[1]);
		int child_result = handle_my_relative_child(fds[0]);
		close(fds[0]);
		if (child_result < OK_ID) {
			return ERROR_ID;
		}
	}


	return OK_ID;
}


bool
am_the_current_parent_process(int forking_result)
{
	return forking_result != 0;
}

int
handle_myself_as_absolute_parent(int max_number_required, int sender_fds)
{
	for (int i = 2; i <= max_number_required; i++) {
		int writing_result = write(sender_fds, &i, sizeof(int));
		if (writing_result < OK_ID) {
			return ERROR_ID;
		}
	}

	return OK_ID;
}

int
handle_my_relative_child(int receiver_fds)
{
	int first_number_received;  // lo que seria "p" del ejemplo
	int reading_result =
	        read(receiver_fds, &first_number_received, sizeof(int));
	if (reading_result < OK_ID) {
		return ERROR_ID;
	} else if (reading_result == OK_ID) {
		return OK_ID;
	}

	printf("primo %i\n", first_number_received);

	int fds_aux[2];  // Pipe para enviarle al hijo nuevo posteriormente

	int result_pipe = pipe(fds_aux);
	if (result_pipe < OK_ID) {
		perror("Error de creacion de fds por pipe");
		return ERROR_ID;
	};

	int forking_result = fork();
	if (forking_result < OK_ID) {
		perror("Error del fork");
		close(fds_aux[0]);
		close(fds_aux[1]);
		return ERROR_ID;
	};

	if (am_the_current_parent_process(forking_result)) {
		close(fds_aux[0]);
		int parent_result = handle_myself_as_relative_parent(
		        receiver_fds, fds_aux[1], first_number_received);
		close(fds_aux[1]);

		int waiting_result = wait(NULL);
		if (waiting_result == ERROR_ID) {
			perror("Error waiting for child");
			return ERROR_ID;
		} else if (parent_result == ERROR_ID) {
			return ERROR_ID;
		}


	} else {
		close(fds_aux[1]);  // no le voy a enviar nada al padre actual
		int child_result = handle_my_relative_child(fds_aux[0]);
		close(fds_aux[0]);
		if (child_result < OK_ID) {
			return ERROR_ID;
		}
	};

	return OK_ID;
}

int
handle_myself_as_relative_parent(int receiver_fds,
                                 int sender_fds,
                                 int first_number_received)
{
	int next_number_received;  // sería "n" en el ejemplo

	int reading_result =
	        read(receiver_fds, &next_number_received, sizeof(int));

	while (reading_result > 0) {
		if (next_number_received % first_number_received != 0) {
			int writing_result = write(sender_fds,
			                           &next_number_received,
			                           sizeof(int));
			if (writing_result < OK_ID) {
				perror("Writing in pipe");
				return ERROR_ID;
			}
		}
		reading_result =
		        read(receiver_fds, &next_number_received, sizeof(int));
	}
	if (reading_result < OK_ID) {
		perror("Reading from pipe");
		return ERROR_ID;
	} else if (reading_result == OK_ID) {
		return OK_ID;
	}

	return OK_ID;
}