#ifndef POSIX_HELPER_H
#define POSIX_HELPER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

#define err_exit(msg)                                                          \
	{                                                                      \
		perror((msg));                                                 \
		exit(EXIT_FAILURE);                                            \
	}

#endif //POSIX_HELPER_H