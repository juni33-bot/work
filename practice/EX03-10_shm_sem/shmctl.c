#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "shm.h"

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int id_shm;
	struct shmid_ds dsbuf;

	if(argc != 2) {
		printf("usage: %s {command}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	id_shm = shmget((key_t)KEY_SHM, sizeof(struct shm_buf), 0666|IPC_CREAT);
	if(id_shm == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	if(strcmp(argv[1], "IPC_STAT") == 0) {
		ret = shmctl(id_shm, IPC_STAT, &dsbuf);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		printf("[%d] IPC_STAT OK\n", pid);
		printf("    shm_perm.__key=%#x\n", dsbuf.shm_perm.__key);
		printf("    shm_perm.mode=%#o\n", dsbuf.shm_perm.mode);
		printf("    shm_segsz=%lu\n", dsbuf.shm_segsz);
	}
	else if(strcmp(argv[1], "IPC_RMID") == 0) {
		ret = shmctl(id_shm, IPC_RMID, 0);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		printf("[%d] IPC_RMID OK\n", pid);
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
