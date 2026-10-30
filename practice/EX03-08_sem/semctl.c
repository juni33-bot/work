#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "sem.h"

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int id_sem;

	if(argc != 2) {
		printf("usage: %s {command}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	id_sem = semget((key_t)KEY_SEM, 1, 0666|IPC_CREAT);
	if(id_sem == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	if(strcmp(argv[1], "SETVAL") == 0) {
		union semun su;
		su.val = 1;
		ret = semctl(id_sem, 0, SETVAL, su);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		printf("[%d] SETVAL OK\n", pid);
	}
	else if(strcmp(argv[1], "IPC_RMID") == 0) {
		ret = semctl(id_sem, 0, IPC_RMID);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		printf("[%d] IPC_RMID OK\n", pid);
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
