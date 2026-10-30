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
	struct msg_buf msgbuf;
	struct stat statb;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	id_msg = msgget((key_t)KEY_MSG, 0666|IPC_CREAT);
	if(id_msg == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	for(;;) {
		ret = msgrcv(id_msg, (void *)&msgbuf, sizeof(struct msg_buf) - sizeof(long), MTYPE_C2S, 0);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}

		if (stat(msgbuf.fname, &statb) == -1) {
			msgbuf.valid = 0;
		}   
		else {
			msgbuf.valid = 1;
			msgbuf.size = statb.st_size;
			msgbuf.mode = statb.st_mode;
		}   
		msgbuf.mtype = msgbuf.pid;

		ret = msgsnd(id_msg, (void *)&msgbuf, sizeof(struct msg_buf) - sizeof(long), 0); 
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}   
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}

