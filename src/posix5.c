#define _GNU_SOURCE
#include "posix_helper.h"

#define NB_WORKERS 2
#define V_LENGTH 400
#define SLICE (V_LENGTH / NB_WORKERS)
#define MQ_NAME "/mq"
#define MSG_PRIO 1
#define NB_AIO_REQ 4

//super utile : https://www.blaess.fr/christophe/2011/09/17/efficacite-des-ipc-les-files-de-messages-posix/
struct vec_data {
	double v1[V_LENGTH];
	double v2[V_LENGTH];
	double res;

	//mq
	mqd_t mq;
	char *mq_buf_msg;
	int   mq_buf_size;
};

struct thread_data {
	struct vec_data *vec_data;
	int		 thread_id;
};

static void handler(int sig, siginfo_t *si, void *uc)
{
	if (si->si_value.sival_int == SYS_READ)
		puts("aio_read finished");
	else if (si->si_value.sival_int == SYS_WRITE)
		puts("aio_write finished");
	else
		puts("unspecified aio finished");
}

void *worker_thread(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;

	double local_res = 0.0;

	int start = data->thread_id * SLICE;
	for (int i = start; i < start + SLICE; i++)
		local_res += data->vec_data->v1[i] * data->vec_data->v2[i];

	if (mq_send(data->vec_data->mq, (char *)&local_res, sizeof(local_res),
		    MSG_PRIO) == -1)
		perror("mq_send");

	pthread_exit(NULL);
}

void *printer_thread(void *arg)
{
	struct vec_data *data = (struct vec_data *)arg;

	int stop_cond = 0;

	while (true) {
		if (mq_receive(data->mq, data->mq_buf_msg, data->mq_buf_size,
			       NULL) == -1) {
			perror("mq_receive");
			goto p_exit;
		}

		if (*(double *)data->mq_buf_msg == 0.0) {
			printf("received value 0\n");
			goto p_exit;
		}
		data->res += *(double *)data->mq_buf_msg;

		if (++stop_cond == NB_WORKERS)
			break;
	}

	printf("---\ndata->res : %lf\n---\n", data->res);
p_exit:
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	if (argc != 3)
		errx(EXIT_FAILURE, "Usage: %s {filename1} {filename2}\n",
		     argv[0]);

	pthread_t worker_threads[NB_WORKERS];
	pthread_t print_thread;

	pthread_attr_t attr;
	struct mq_attr mq_attr;

	void *status;

	struct vec_data vec_data;
	vec_data.res = 0.0;

	if ((vec_data.mq = mq_open(MQ_NAME, O_RDWR | O_CREAT, 0600, NULL)) ==
	    -1)
		perr_exit("mq_open");

	if (mq_getattr(vec_data.mq, &mq_attr) != 0)
		perr_exit("mq_getattr");

	vec_data.mq_buf_size = mq_attr.mq_msgsize;
	vec_data.mq_buf_msg  = malloc(vec_data.mq_buf_size);
	if (vec_data.mq_buf_msg == NULL)
		perr_exit("malloc");

	struct thread_data thread_data_workers[NB_WORKERS];
	for (int i = 0; i < NB_WORKERS; i++) {
		thread_data_workers[i].thread_id = i;
		thread_data_workers[i].vec_data	 = &vec_data;
	}

	int fd_vec1, fd_vec2;
	//Ouverture du fichier (spécifié en argument) sur lequel l’E/S va être effectuée
	if ((fd_vec1 = open(argv[1], O_RDWR, 0600)) == -1)
		perr_exit("open");

	if ((fd_vec2 = open(argv[2], O_RDWR, 0600)) == -1)
		perr_exit("open");

	// BEGIN SIGACTION
	struct sigaction sa;
	sa.sa_flags =
		SA_SIGINFO |
		SA_RESTART; // SA_RESTART because of interupted system call : https://unix.stackexchange.com/questions/509375/what-is-interrupted-system-call
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGRTMIN, &sa, NULL) == -1)
		perr_exit("sigaction");
	// END SIGACTION

	// BEGIN AIO
	struct aiocb cb_vec0, cb_vec1, cb_vec2, cb_vec3;
	size_t	     nb_bytes_aio = SLICE * sizeof(double);

	const struct aiocb *cbs[4];

	// thought it was nb_bytes_aio + 1 for vec1 and vec3 but no :(
	aiocb_init(&cb_vec0, fd_vec1, nb_bytes_aio, 0, SYS_READ);
	aiocb_init(&cb_vec1, fd_vec1, nb_bytes_aio, nb_bytes_aio, SYS_READ);
	aiocb_init(&cb_vec2, fd_vec2, nb_bytes_aio, 0, SYS_READ);
	aiocb_init(&cb_vec3, fd_vec2, nb_bytes_aio, nb_bytes_aio, SYS_READ);

	cbs[0] = &cb_vec0;
	cbs[1] = &cb_vec1;
	cbs[2] = &cb_vec3;
	cbs[3] = &cb_vec3;

	if (aio_read(&cb_vec0) == -1)
		perr_exit("aio_read");
	if (aio_read(&cb_vec1) == -1)
		perr_exit("aio_read");
	if (aio_read(&cb_vec2) == -1)
		perr_exit("aio_read");
	if (aio_read(&cb_vec3) == -1)
		perr_exit("aio_read");

	// skeleton taken from : https://www.systutorials.com/docs/linux/man/7-aio/#lbAH
	int open_reqs = NB_AIO_REQ;
	while (open_reqs > 0) {
		if (aio_suspend(cbs, open_reqs, NULL) == -1)
			perr_exit("aio_suspend");

		for (int i = 0; i < NB_AIO_REQ; i++) {
			if (cbs[i] != NULL) {
				if (aio_error(cbs[i]) == 0) {
					cbs[i] = NULL;
					open_reqs--;
				} else
					perr_exit("aio_error");
			}
		}
	}

	if (aio_return(&cb_vec0) == -1)
		perr_exit("aio_return");
	if (aio_return(&cb_vec1) == -1)
		perr_exit("aio_return");
	if (aio_return(&cb_vec2) == -1)
		perr_exit("aio_return");
	if (aio_return(&cb_vec3) == -1)
		perr_exit("aio_return");

	memcpy((char *)vec_data.v1, (char *)cb_vec0.aio_buf, nb_bytes_aio);
	memcpy(&((char *)vec_data.v1)[nb_bytes_aio], (char *)cb_vec1.aio_buf,
	       nb_bytes_aio);
	memcpy((char *)vec_data.v2, (char *)cb_vec2.aio_buf, nb_bytes_aio);
	memcpy(&((char *)vec_data.v2)[nb_bytes_aio], (char *)cb_vec3.aio_buf,
	       nb_bytes_aio);
	// END AIO

	// thread_creation
	for (int i = 0; i < NB_WORKERS; i++) {
		printf("pthread_create : id = %d\n", i);
		if (pthread_create(&worker_threads[i], &attr, worker_thread,
				   &thread_data_workers[i]))
			perr_exit("pthread_create");
	}

	printf("pthread_create : id = print\n");
	if (pthread_create(&print_thread, &attr, printer_thread, &vec_data))
		perr_exit("pthread_create");

	/* liberation des attributs et attente de la terminaison des threads */
	pthread_attr_destroy(&attr);
	for (int i = 0; i < NB_WORKERS; i++) {
		if (pthread_join(worker_threads[i], &status))
			perr_exit("pthread_join");
		printf("pthread_join : %d status = %ld\n", i, (long)status);
	}
	if (pthread_join(print_thread, &status))
		perr_exit("pthread_join");
	printf("pthread_join : print status = %ld\n", (long)status);

	// cleaning up mq
	mq_close(vec_data.mq);
	mq_unlink(MQ_NAME);

	free((void *)cb_vec0.aio_buf);
	free((void *)cb_vec1.aio_buf);
	free((void *)cb_vec2.aio_buf);
	free((void *)cb_vec3.aio_buf);

	return EXIT_SUCCESS;
}
