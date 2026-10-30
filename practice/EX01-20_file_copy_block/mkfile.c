#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int i, ret, size, fd;
	char c, *name;

	if(argc != 3) {
		printf("usage: %s {file name} {file size}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("running %s %s %s\n", argv[0], argv[1], argv[2]);

	name = argv[1];
	size = atoi(argv[2]);

	/* open */
	fd = open(name, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
	if(fd == -1) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;

	}

	/* write */
	for(i = 0; i < size; i++) {
		c = (i % 10) + '0';
		ret = write(fd, &c, 1);
		if(ret == -1) {
			printf("error: %s (%d)\n", strerror(errno), __LINE__);
			return EXIT_FAILURE;

		}
	}

	/* close */
	close(fd);

	printf("%s has been created\n", name);

	return EXIT_SUCCESS;
}

