#include "posix_helper.h"

void aiocb_init(struct aiocb *cb, int fd, size_t buf_size, int offset, int info)
{
	// signal handling
	cb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	cb->aio_sigevent.sigev_signo  = SIGRTMIN;

	cb->aio_sigevent.sigev_value.sival_int = info;

	//definition du bloc de contrôle de l’entrée/sortie
	if ((cb->aio_buf = malloc(buf_size)) == NULL)
		perr_exit("malloc");

	//récupérer le descripteur d’un fichier à partir de son nom
	cb->aio_fildes	= fd;
	cb->aio_nbytes	= buf_size;
	cb->aio_offset	= offset;
	cb->aio_reqprio = 0;
}