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
struct msg_buf msgarr[] = {{1,"text1"},{2,"text2"},{3,"text3"},{4,"text4"},{5,"text5"},
	{4,"text4"},{5,"text5"},{6,"text6"},{7,"text7"},{8,"text8"}};  

int main(int argc, char **argv)
{
	int ret;
	int id_msg;
	int i;

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

	for(i=0; i<sizeof(msgarr)/sizeof(msgarr[0]); i++) {
		ret = msgsnd(id_msg, (void *)&msgarr[i], sizeof(struct msg_buf) - sizeof(long), 0);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}

