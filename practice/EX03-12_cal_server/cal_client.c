#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "msg.h"

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int id_msg;
	struct msg_buf msgbuf;

	if(argc != 3) {
		printf("usage: %s {op1} {op2}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s %s\n", pid = getpid(), argv[0], argv[1], argv[2]);

	id_msg = msgget((key_t)KEY_MSG, 0666|IPC_CREAT);
	if(id_msg == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	msgbuf.mtype = MTYPE_C2S;
	msgbuf.op1 = atoi(argv[1]);
	msgbuf.op2 = atoi(argv[2]);

	ret = msgsnd(id_msg, (void *)&msgbuf, sizeof(struct msg_buf) - sizeof(long), 0);
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	ret = msgrcv(id_msg, (void *)&msgbuf, sizeof(struct msg_buf) - sizeof(long), MTYPE_S2C, 0);
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
	printf("[%d] %d * %d = %d\n", pid, msgbuf.op1, msgbuf.op2, msgbuf.result);

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}

