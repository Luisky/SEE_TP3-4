#include "posix_helper.h"

#define NB_THREADS 3

void *travailUtile(void *arg_void)
{
	int    i;
	double resultat = 0.0;

	for (i = 0; i < 1000000; i++) {
		resultat = resultat + *(int *)arg_void;
	}
	printf("resultat = %e\n", resultat);
	pthread_exit((void *)0);
}

int main(int argc, char *argv[])
{
	pthread_t      thread[NB_THREADS];
	pthread_attr_t attr;
	int	       rc, t;
	void *	       status;

	if (argc != 4)
		errx(EXIT_FAILURE, "please specify 3 numbers");

	/* Initialisation et activation d’attributs */
	pthread_attr_init(&attr); //valeur par défaut

	size_t stacksize;
	pthread_attr_getstacksize(&attr, &stacksize);
	printf("stacksize :  %ld KB\n", stacksize / 1024);
	pthread_attr_setstacksize(&attr, (stacksize + 1024 * 1024));
	pthread_attr_getstacksize(&attr, &stacksize);
	printf("stacksize :  %ld KB\n", stacksize / 1024);

	// PTHREAD_CREATE_JOINABLE est la valeur par défaut
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	//attente du thread possible

	int thread_args[NB_THREADS];
	for (int i = 0; i < NB_THREADS; i++) {
		thread_args[i] = atoi(argv[1 + i]);
	}

	for (t = 0; t < NB_THREADS; t++) {
		printf("Creation du thread %d\n", t);
		rc = pthread_create(&thread[t], &attr, travailUtile,
				    &thread_args[t]);
		if (rc) {
			printf("ERROR; le code de retour de pthread_create() est %d\n",
			       rc);
			exit(-1);
		}
	}

	/* liberation des attributs et attente de la terminaison des threads */
	pthread_attr_destroy(&attr);
	for (t = 0; t < NB_THREADS; t++) {
		rc = pthread_join(thread[t], &status);
		if (rc) {
			printf("ERROR; le code de retour du pthread_join() est %d\n",
			       rc);
			exit(-1);
		}
		printf("le join a fini avec le thread %d et a donne le status= %ld\n",
		       t, (long)status);
	}

	return EXIT_SUCCESS;
}
