#include "posix_helper.h"

#define V_LENGTH 400

int main(int argc, char **argv)
{
	if (argc != 2)
		errx(EXIT_FAILURE, "Usage: %s {filename}\n", argv[0]);

	int fd;
	if ((fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
		perr_exit("open");

	double buf[V_LENGTH];

	for (int i = 0; i < V_LENGTH; i++)
		buf[i] = 1.0;

	if (write(fd, buf, sizeof(double) * V_LENGTH) !=
	    (sizeof(double) * V_LENGTH))
		perr_exit("write");

	close(fd);

	return EXIT_SUCCESS;
}