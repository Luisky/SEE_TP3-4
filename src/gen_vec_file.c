#include "posix_helper.h"

int main(int argc, char **argv)
{
	if (argc != 2)
		errx(EXIT_FAILURE, "Usage: %s {filename}\n", argv[0]);

	int fd;
	if ((fd = open(argv[0], O_CREAT | O_WRONLY, 0644)) == -1)
		err_exit("open");

	return EXIT_SUCCESS;
}