#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <err.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */
#include <unistd.h>
#include <sys/types.h>
#include <string.h> /* memcpy */
#include <mqueue.h>
#include <stdbool.h>
#include <stdint.h>

#define NB_WORKERS 4
#define V_LENGTH 400
#define SLICE (V_LENGTH / NB_WORKERS)
#define MQ_NAME "/mq"
#define MSG_PRIO 1

//super utile : https://www.blaess.fr/christophe/2011/09/17/efficacite-des-ipc-les-files-de-messages-posix/
struct vec_data {
	double v1[V_LENGTH];
	double v2[V_LENGTH];
	double v3[V_LENGTH];
	double res;
};

struct thread_data {
	struct vec_data *vec_data;
	int		 thread_id;
};

//mq
mqd_t mq_test;
char *buffer_msg  = NULL;
int   mq_buf_size = 0;

void *worker_thread(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;

	int start = data->thread_id * SLICE;
	for (int i = start; i < start + SLICE; i++) {
		data->vec_data->v3[i] =
			data->vec_data->v1[i] * data->vec_data->v2[i];
	}

	printf("done");

	uint8_t msg = 1;
	if (mq_send(mq_test, (char *)&msg, sizeof(msg), MSG_PRIO) == -1)
		perror("mq_send");

	pthread_exit(NULL);
}

void *printer_thread(void *arg)
{
	struct vec_data *data = (struct vec_data *)arg;

	uint8_t stop_cond = 0;

	while (true) {
		if (mq_receive(mq_test, buffer_msg, mq_buf_size, NULL) == -1) {
			perror("mq_receive");
			pthread_exit(NULL);
		}

		stop_cond += buffer_msg[0];
		if (stop_cond == 4)
			break;
	}

	for (int i = 0; i < V_LENGTH; i++)
		data->res += data->v3[i];

	printf("---\ndata->res : %lf\n---\n", data->res);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	pthread_t worker_threads[NB_WORKERS];
	pthread_t print_thread;

	pthread_attr_t attr;
	struct mq_attr mq_test_attr;

	void *status;

	if ((mq_test = mq_open(MQ_NAME, O_RDWR | O_CREAT, 0600, NULL)) == -1) {
		perror("mq_open");
		exit(EXIT_FAILURE);
	}

	if (mq_getattr(mq_test, &mq_test_attr) != 0) {
		perror("mq_getattr");
		exit(EXIT_FAILURE);
	}

	mq_buf_size = mq_test_attr.mq_msgsize;

	printf("mq_buf_size : %d\n", mq_buf_size);

	buffer_msg = malloc(mq_buf_size);
	if (buffer_msg == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	struct vec_data vec_data;
	for (int i = 0; i < V_LENGTH; i++) {
		vec_data.v1[i] = 1.0;
		vec_data.v2[i] = 1.0;
		vec_data.v3[i] = 0.0;
	}
	vec_data.res = 0.0;

	struct thread_data thread_data_workers[NB_WORKERS];
	for (int i = 0; i < NB_WORKERS; i++) {
		thread_data_workers[i].thread_id = i;
		thread_data_workers[i].vec_data	 = &vec_data;
	}

	// thread_creation
	for (int i = 0; i < NB_WORKERS; i++) {
		printf("pthread_create : id = %d\n", i);
		if (pthread_create(&worker_threads[i], &attr, worker_thread,
				   &thread_data_workers[i]))
			errx(EXIT_FAILURE, "pthread_create");
	}

	printf("pthread_create : id = print\n");
	if (pthread_create(&print_thread, &attr, printer_thread, &vec_data))
		errx(EXIT_FAILURE, "pthread_create");

	/* liberation des attributs et attente de la terminaison des threads */
	pthread_attr_destroy(&attr);
	for (int i = 0; i < NB_WORKERS; i++) {
		if (pthread_join(worker_threads[i], &status))
			errx(EXIT_FAILURE, "pthread_join");
		printf("pthread_join : %d status = %ld\n", i, (long)status);
	}
	if (pthread_join(print_thread, &status))
		errx(EXIT_FAILURE, "pthread_join");
	printf("pthread_join : print status = %ld\n", (long)status);

	// cleaning up mq
	mq_close(mq_test);
	mq_unlink(MQ_NAME);

	pthread_exit(NULL);
}
