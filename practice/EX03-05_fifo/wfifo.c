#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PATH_FIFO "./my_fifo"

pid_t pid;

int main(int argc, char **argv)
{
	int fd_fifo;
	int ret;
	char *wstr;

	if(argc != 2) {
		printf("usage: %s {str}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	ret = mkfifo(PATH_FIFO, 0777);
	if(ret == -1) {
		if(errno == EEXIST) {
			printf("[%d] %s already exists\n", pid, PATH_FIFO);
		}
		else {
			printf("[%d] error: %s\n", pid, strerror(errno));
			return EXIT_FAILURE;
		}
	}

	fd_fifo = open(PATH_FIFO, O_WRONLY);
	if(fd_fifo == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}

	wstr = argv[1];
	ret = write(fd_fifo, wstr, strlen(wstr)+1);
	if(ret == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("[%d] wrote %d bytes to %s [%s]\n", pid, ret, PATH_FIFO, wstr);

	close(fd_fifo);

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
