#include "posix_helper.h"

#define BUF_SIZE 64

static void handler(int sig, siginfo_t *si, void *uc)
{
	if (si->si_value.sival_int == SYS_READ)
		puts("aio_read finished");
	else if (si->si_value.sival_int == SYS_WRITE)

		puts("aio_write finished");
	else
		puts("unspecified aio finished");
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		errx(EXIT_FAILURE, "Usage: %s {filename}\n", argv[0]);

	int fd;

	struct aiocb cbr1, cbr2, cbw; // bloc de contrôle de l’E/S asynchrone
	const struct aiocb *cbs[3];

	//Ouverture du fichier (spécifié en argument) sur lequel l’E/S va être effectuée
	if ((fd = open(argv[1], O_RDWR, 0600)) == -1)
		perr_exit("open");

	// setting up signal handling
	struct sigaction sa;
	sa.sa_flags	= SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1)
		perr_exit("sigaction");

	aiocb_init(&cbr1, fd, BUF_SIZE, 0, SYS_READ);
	aiocb_init(&cbr2, fd, BUF_SIZE, 0, SYS_READ);

	char * msg_to_write = "GREETINGS_FRIENDS\n";
	size_t mtw_len	    = strlen(msg_to_write);
	aiocb_init(&cbw, fd, mtw_len, 0, SYS_WRITE);
	memcpy((char *)cbw.aio_buf, msg_to_write, mtw_len);

	cbs[0] = &cbr1;
	cbs[1] = &cbr2;
	cbs[2] = &cbw;

	//lancer la lecture
	if (aio_read(&cbr1) == -1)
		perr_exit("aio_read");

	if (aio_write(&cbw) == -1)
		perr_exit("aio_write");

	if (aio_read(&cbr2) == -1)
		perr_exit("aio_read");

	//Suspension du processus dans l’attente de la terminaison de la lecture
	//aio_suspend(cbs, 3, NULL);

	if (lio_listio(LIO_WAIT, (struct aiocb *const *)cbs, 3, NULL))
		perr_exit("lio_listio");

	int ret;
	if ((ret = aio_return(&cbr1)) == -1)
		perr_exit("aio_return");
	printf("cbr1 returned %d\n", ret);

	if ((ret = aio_return(&cbr2)) == -1)
		perr_exit("aio_return");
	printf("cbr2 returned %d\n", ret);

	if ((ret = aio_return(&cbw)) == -1)
		perr_exit("aio_return");
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

	close(fd);

	return EXIT_SUCCESS;
}