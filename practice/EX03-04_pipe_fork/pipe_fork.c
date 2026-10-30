#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_BUF 512

pid_t pid;

int main(int argc, char **argv)
{
	int fd_pipe[2];
	int ret;
	char wbuf[MAX_BUF];
	char rbuf[MAX_BUF];
	pid_t pid_temp;
	int status;


	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	ret = pipe(fd_pipe);
	if(ret == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}

	pid_temp = fork();
	if(pid_temp == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}
	else if(pid_temp == 0) {
		printf("[%d] child running\n", pid = getpid());

		close(fd_pipe[1]);
		ret = read(fd_pipe[0], rbuf, MAX_BUF);
		if(ret == -1) {
			printf("[%d] error: %s\n", pid, strerror(errno));
			return EXIT_FAILURE;
		}
		printf("[%d] read %d bytes from pipe [%s]\n", pid, ret, rbuf);
		close(fd_pipe[0]);
	}
	else {

		close(fd_pipe[0]);
		strcpy(wbuf, "hello");
		ret = write(fd_pipe[1], wbuf, strlen(wbuf)+1);
		if(ret == -1) {
			printf("[%d] error: %s\n", pid, strerror(errno));
			return EXIT_FAILURE;
		}
		printf("[%d] wrote %d bytes to pipe [%s]\n", pid, ret, wbuf);
		close(fd_pipe[1]);

		printf("[%d] waiting child's termination\n", pid);
		pid_temp = wait(&status);
		printf("[%d] pid %d has been terminated with status %#x\n", pid, pid_temp, status);
	}

	printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}
