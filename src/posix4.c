#include <aio.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s {filename}\n", argv[0]);
		return -1;
	}
	struct aiocb  cb; // bloc de contrôle de l’E/S asynchrone
	struct aiocb *cbs[1];
	//Ouverture du fichier (spécifié en argument) sur lequel l’E/S va être effectuée
	FILE *file = fopen(argv[1], "r+");
	//definition du bloc de contrôle de l’entrée/sortie
	cb.aio_buf = malloc(11);
	//récupérer le descripteur d’un fichier à partir de son nom
	cb.aio_fildes = fileno(file);
	cb.aio_nbytes = 10;
	cb.aio_offset = 0;

	//lancer la lecture
	aio_read(&cb);
	//Suspension du processus dans l’attente de la terminaison de la lecture
	cbs[0] = &cb;
	aio_suspend(cbs, 1, NULL);
	printf("operation AIO a retouree %ld\n", aio_return(&cb));
	return 0;
}