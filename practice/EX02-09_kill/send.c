#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	pid_t sig_pid;
	int sig_num;

	if(argc != 3) {
		printf("usage: %s {process ID} {signal number}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s %s\n", pid = getpid(), argv[0], argv[1], argv[2]);
	sig_pid = (pid_t)atoi(argv[1]);
	sig_num = atoi(argv[2]);

	printf("[%d] sending signal %d to process %d\n", pid, sig_num, sig_pid);
	ret = kill(sig_pid, sig_num);
	if(ret == -1) {
		printf("[%d] error: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}
	printf("[%d] sent signal successfully\n", pid);

	printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}

