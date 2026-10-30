#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

pid_t pid;

int main(int argc, char **argv)
{
	pid_t pid_temp;	
	char *msg = "none";

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	pid_temp = fork();

	if(pid_temp == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
	else if(pid_temp == 0) {
		pid = getpid();
		msg = "this is child";
		sleep(3);
	}
	else {
		msg = "this is parent";
		sleep(5);
	}

	printf("[%d] pid_temp = %d, msg = %s, ppid = %d\n", pid, pid_temp, msg, getppid());
	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}

