#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define MAX_BUF 512

pid_t pid;

int main(int argc, char **argv)
{
	int fd_pipe[2];
	int ret;
	char wbuf[MAX_BUF];
	char rbuf[MAX_BUF];

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	/* open pipe */ 
	ret = pipe(fd_pipe);
	if(ret == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}

	/* write data to pipe */
	strcpy(wbuf, "hello");
	ret = write(fd_pipe[1], wbuf, strlen(wbuf)+1);
	if(ret == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("[%d] wrote %d bytes to pipe [%s]\n", pid, ret, wbuf);

	/* read data from pipe */
	ret = read(fd_pipe[0], rbuf, MAX_BUF);
	if(ret == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("[%d] read %d bytes from pipe [%s]\n", pid, ret, rbuf);

	printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}
