#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <time.h>

#include "msg.h"

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int id_msg;
	struct msg_buf msgbuf;

	if(argc != 2) {
		printf("usage: %s {file name}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	if(strlen(argv[1]) >= MAX_FNAME) {
		printf("[%d] error: maximum file length is %d\n", pid, MAX_FNAME);
		return EXIT_FAILURE;
	}

	/* Implement code */


	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}

