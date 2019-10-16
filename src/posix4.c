//  flashback
#define _GNU_SOURCE
#include <aio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define SYS_READ 0
#define SYS_WRITE 1
#define BUF_SIZE 64

#define err_exit(msg)                                                          \
	{                                                                      \
		perror((msg));                                                 \
		exit(EXIT_FAILURE);                                            \
	}

static void handler(int sig, siginfo_t *si, void *uc)
{
	if (si->si_value.sival_int == SYS_READ)
		puts("aio_read finished");
	else if (si->si_value.sival_int == SYS_WRITE)

		puts("aio_write finished");
	else
		puts("unspecified aio finished");
}

void aiocb_init(struct aiocb *cb, int fd, size_t buf_size, int offset, int info)
{
	// signal handling
	cb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	cb->aio_sigevent.sigev_signo  = SIGRTMIN;

	cb->aio_sigevent.sigev_value.sival_int = info;

	//definition du bloc de contrôle de l’entrée/sortie
	if ((cb->aio_buf = malloc(buf_size)) == NULL)
		err_exit("malloc");

	//récupérer le descripteur d’un fichier à partir de son nom
	cb->aio_fildes	= fd;
	cb->aio_nbytes	= buf_size;
	cb->aio_offset	= offset;
	cb->aio_reqprio = 0;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: %s {filename1} {filename2}\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int fd1, fd2;

	struct aiocb cbr1, cbr2, cbw; // bloc de contrôle de l’E/S asynchrone
	const struct aiocb *cbs[3];

	//Ouverture du fichier (spécifié en argument) sur lequel l’E/S va être effectuée
	if ((fd1 = open(argv[1], O_RDWR, 0600)) == -1)
		err_exit("open");

	if ((fd2 = open(argv[2], O_RDWR, 06000)) == -1)
		err_exit("open");

	// setting up signal handling
	struct sigaction sa;
	sa.sa_flags	= SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1)
		err_exit("sigaction");

	aiocb_init(&cbr1, fd1, BUF_SIZE, 0, SYS_READ);
	aiocb_init(&cbr2, fd1, BUF_SIZE, 0, SYS_READ);

	char * msg_to_write = "GREETINGS_FRIENDS\n";
	size_t mtw_len	    = strlen(msg_to_write);
	aiocb_init(&cbw, fd1, mtw_len, 0, SYS_WRITE);
	memcpy((char *)cbw.aio_buf, msg_to_write, mtw_len);

	//lancer la lecture
	if (aio_read(&cbr1) == -1)
		err_exit("aio_read");

	if (aio_write(&cbw) == -1)
		err_exit("aio_write");

	if (aio_read(&cbr2) == -1)
		err_exit("aio_read");

	//Suspension du processus dans l’attente de la terminaison de la lecture
	cbs[0] = &cbr1;
	cbs[1] = &cbr2;
	cbs[2] = &cbw;

	aio_suspend(cbs, 3, NULL);

	int ret;
	if ((ret = aio_return(&cbr1)) == -1)
		err_exit("aio_return");
	printf("cbr1 returned %d\n", ret);

	if ((ret = aio_return(&cbr2)) == -1)
		err_exit("aio_return");
	printf("cbr2 returned %d\n", ret);

	if ((ret = aio_return(&cbw)) == -1)
		err_exit("aio_return");
	printf("cbw returned %d\n", ret);

	char tab[BUF_SIZE];

	memcpy(tab, (char *)cbr1.aio_buf, BUF_SIZE);
	tab[BUF_SIZE - 1] = '\0';
	printf("cbr1 what was read :\n%s\n", tab);

	memcpy(tab, (char *)cbr2.aio_buf, BUF_SIZE);
	tab[BUF_SIZE - 1] = '\0';
	printf("cbr2 what was read :\n%s\n", tab);

	// cleanup the malloc
	//free(cbr1.aio_buf);

	close(fd1);
	close(fd2);

	return 0;
}