#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "msg.h"

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int id_msg;

	if(argc != 2) {
		printf("usage: %s {command}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	id_msg = msgget((key_t)KEY_MSG, 0666|IPC_CREAT);
	if(id_msg == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	if(strcmp(argv[1], "IPC_STAT") == 0) {
		struct msqid_ds dsbuf;
		ret = msgctl(id_msg, IPC_STAT, &dsbuf);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		printf("[%d] IPC_STAT OK\n", pid);
		printf("    msg_perm.__key=%#x\n", dsbuf.msg_perm.__key);
		printf("    msg_perm.mode=%#o\n", dsbuf.msg_perm.mode);
		printf("    msg_qbytes=%ld\n", dsbuf.msg_qbytes);
	}
	else if(strcmp(argv[1], "IPC_RMID") == 0) {
		ret = msgctl(id_msg, IPC_RMID, 0);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		printf("[%d] IPC_RMID OK\n", pid);
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
