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

#define NB_WORKERS 4
#define V_LENGTH 400
#define SLICE (V_LENGTH / NB_WORKERS)
#define SHM_NAME "/mem"

struct vec_data {
	double v1[V_LENGTH];
	double v2[V_LENGTH];
	double v3[V_LENGTH];
	double res;
	int    p_count;

	pthread_mutex_t *mut_cond;
	pthread_cond_t * cond;
};

struct thread_data {
	struct vec_data *vec_data;
	int		 thread_id;
};

void *worker_thread(void *arg)
{
	struct thread_data *data = (struct thread_data *)arg;

	int start = data->thread_id * SLICE;
	for (int i = start; i < start + SLICE; i++) {
		data->vec_data->v3[i] =
			data->vec_data->v1[i] * data->vec_data->v2[i];

		pthread_mutex_lock(data->vec_data->mut_cond);
		data->vec_data->p_count++;
		if (data->vec_data->p_count == V_LENGTH)
			pthread_cond_signal(data->vec_data->cond);

		pthread_mutex_unlock(data->vec_data->mut_cond);
	}

	pthread_exit(NULL);
}

void *printer_thread(void *arg)
{
	struct vec_data *data = (struct vec_data *)arg;

	pthread_mutex_lock(data->mut_cond); // we'll unlock it later

	while (data->p_count != V_LENGTH) // because of spurios wakeup
		pthread_cond_wait(data->cond, data->mut_cond);

	pthread_mutex_unlock(data->mut_cond);

	for (int i = 0; i < V_LENGTH; i++)
		data->res += data->v3[i];

	printf("---\ndata->res : %lf\n---\n", data->res);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	pthread_t worker_threads[NB_WORKERS];
	pthread_t print_thread;

	pthread_attr_t	attr;
	pthread_mutex_t mut_cond;
	pthread_cond_t	cond;

	pthread_mutex_init(&mut_cond, NULL);
	pthread_cond_init(&cond, NULL);

	void *status;

	int    fd;
	size_t shm_size;
	void * mm_addr = NULL;

	if ((fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0644)) == -1)
		errx(EXIT_FAILURE, "shm_open");

	shm_size = sizeof(struct vec_data) / sysconf(_SC_PAGE_SIZE);
	shm_size++; // we should use a modulo to check if we have to do this
	shm_size *= sysconf(_SC_PAGE_SIZE);

	if (ftruncate(fd, shm_size) == -1)
		errx(EXIT_FAILURE, "ftruncate");

	mm_addr =
		mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mm_addr == NULL || mm_addr == MAP_FAILED)
		errx(EXIT_FAILURE, "mmap");

	struct vec_data vec_data;
	for (int i = 0; i < V_LENGTH; i++) {
		vec_data.v1[i] = 1.0;
		vec_data.v2[i] = 1.0;
		vec_data.v3[i] = 0.0;
	}
	vec_data.res	  = 0.0;
	vec_data.p_count  = 0;
	vec_data.mut_cond = &mut_cond;
	vec_data.cond	  = &cond;

	memcpy(mm_addr, &vec_data, sizeof(struct vec_data));

	struct thread_data thread_data_workers[NB_WORKERS];
	for (int i = 0; i < NB_WORKERS; i++) {
		thread_data_workers[i].thread_id = i;
		thread_data_workers[i].vec_data	 = (struct vec_data *)mm_addr;
	}

	// thread_creation
	for (int i = 0; i < NB_WORKERS; i++) {
		printf("pthread_create : id = %d\n", i);
		if (pthread_create(&worker_threads[i], &attr, worker_thread,
				   &thread_data_workers[i]))
			errx(EXIT_FAILURE, "pthread_create");
	}

	printf("pthread_create : id = print\n");
	if (pthread_create(&print_thread, &attr, printer_thread, mm_addr))
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

	pthread_mutex_destroy(&mut_cond);
	pthread_cond_destroy(&cond);

	// TODO: cleanup function with those three
	close(fd);
	shm_unlink(SHM_NAME);
	munmap(mm_addr, shm_size);

	pthread_exit(NULL);
}
