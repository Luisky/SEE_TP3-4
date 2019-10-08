#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <err.h>

#define NB_THREADS 4
#define V_LENGTH 400

struct vec_scal {
	double v1;
	double v2;
};

double res_scal    = 0.0;
int    print_count = 0;

pthread_mutex_t mu;

void *travailUtile(void *arg_void)
{
	double res;
	for (int i = 0; i < 100; i++) {
		res = ((struct vec_scal *)arg_void)[i].v1 *
		      ((struct vec_scal *)arg_void)[i].v2;
		pthread_mutex_lock(&mu);
		res_scal += res;
		pthread_mutex_unlock(&mu);
	}
	pthread_exit((void *)0);
}

int main(int argc, char *argv[])
{
	pthread_t      thread[NB_THREADS];
	pthread_attr_t attr;

	void *status;

	struct vec_scal v_res[V_LENGTH];
	for (int i = 0; i < V_LENGTH; i++) {
		v_res[i].v1 = 1;
		v_res[i].v2 = 1;
	}

	struct vec_scal *v_res_slices[NB_THREADS];

	pthread_mutex_init(&mu, NULL);

	v_res_slices[0] = &v_res[0];
	v_res_slices[1] = &v_res[100];
	v_res_slices[2] = &v_res[200];
	v_res_slices[3] = &v_res[300];

	for (int i = 0; i < NB_THREADS; i++) {
		printf("Creation du thread %d\n", i);
		if (pthread_create(&thread[i], &attr, travailUtile,
				   v_res_slices[i]))
			errx(EXIT_FAILURE, "pthread_create");
	}

	/* liberation des attributs et attente de la terminaison des threads */
	pthread_attr_destroy(&attr);
	for (int i = 0; i < NB_THREADS; i++) {
		if (pthread_join(thread[i], &status))
			errx(EXIT_FAILURE, "pthread_join");
		printf("le join a fini avec le thread %d et a donne le status= %ld\n",
		       i, (long)status);
	}

	pthread_mutex_destroy(&mu);

	printf("resultat produit scalaire : %lf\n", res_scal);

	pthread_exit(NULL);
}
