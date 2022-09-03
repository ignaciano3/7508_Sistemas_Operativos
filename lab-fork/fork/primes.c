#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

const int ERROR_ID = -1;
const int OK_ID = 0;


int
am_the_parent_process(int forking_result)
{
	return forking_result != 0;
}

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
		fprintf(stderr,
		        "\n\t%s: %i\n",
		        "Error de creacion de fds por pipe",
		        result_pipe);
		return ERROR_ID;
	};

	int forking_result = fork();
	if (forking_result < OK_ID) {
		fprintf(stderr, "\n\t%s: %i\n", "Error del fork", forking_result);
		close(fds[0]);
		close(fds[1]);
		return ERROR_ID;
	};

	if (am_the_parent_process(forking_result)) {
		// Cierro el punto de lectura que no voy a usar (soy el proceso "de la izquierda de todo")
		close(fds[0]);

		for (int i = 2; i <= max_number_required; i++) {
			int writing_result = write(fds[1], &i, sizeof(int));
			if (writing_result < OK_ID) {
				close(fds[1]);
				return ERROR_ID;
			}
		}
		close(fds[1]);

	} else {  // Estoy en el proceso hijo
		// Cierro el proceso de escritura ya que no me voy a recomunicar con el padre
		close(fds[1]);

		int fds_aux[2];

		for (int i = 2; i <= max_number_required; i++) {
			result_pipe = pipe(fds_aux);
			if (result_pipe < OK_ID) {
				fprintf(stderr,
				        "\n\t%s: %i\n",
				        "Error de creacion de fds por pipe",
				        result_pipe);

				close(fds[0]);
				// ... REVISAR SI QUEDAN FDS ABIERTOS

				return ERROR_ID;
			};

			forking_result = fork();
			if (forking_result < OK_ID) {
				fprintf(stderr,
				        "\n\t%s: %i\n",
				        "Error del fork",
				        forking_result);
				close(fds[0]);
				close(fds_aux[0]);
				close(fds_aux[1]);
				// ... REVISAR SI QUEDAN FDS ABIERTOS

				return ERROR_ID;
			};

			if (am_the_parent_process(forking_result)) {
				close(fds_aux[0]);

				int received_msg;  // lo que seria "p" del ejemplo
				int reading_result =
				        read(fds[0], &received_msg, sizeof(int));
				if (reading_result < OK_ID) {
					close(fds[0]);
					close(fds_aux[1]);

					return ERROR_ID;
				}

				printf("\n\t%i", received_msg);

				int aux_received_msg;  // lo que seria "n" del ejemplo

				while (read(fds[0],
				            &aux_received_msg,
				            sizeof(int)) > 0) {
					if (aux_received_msg % received_msg != 0) {
						int writing_result =
						        write(fds_aux[1],
						              &aux_received_msg,
						              sizeof(int));
						if (writing_result < OK_ID) {
							close(fds[0]);
							close(fds_aux[1]);
							return ERROR_ID;
						}
					}
				}


				close(fds[0]);
				close(fds_aux[1]);

				break;

			} else {  // Estoy en el proceso hijo
				close(fds_aux[1]);
				i++;
			};
		}
	};


	return OK_ID;
}