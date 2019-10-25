#ifndef POSIX_HELPER_H
#define POSIX_HELPER_H

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <aio.h>
#include <mqueue.h>

#define SYS_READ 0
#define SYS_WRITE 1

#define perr_exit(msg)                                                         \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

void aiocb_init(struct aiocb *cb, int fd, size_t buf_size, int offset,
		int info);

#endif //POSIX_HELPER_H