// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <errno.h>
// #include <string.h>
// #include <fcntl.h>
// #include <sys/types.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/msg.h>
// #include <time.h>

// #include "msg.h"

// pid_t pid;

// int main(int argc, char **argv)
// {
// 	int ret;
// 	int id_msg;
// 	struct msg_buf msgbuf;

// 	if(argc != 2) {
// 		printf("usage: %s {file name}\n", argv[0]);
// 		return EXIT_FAILURE;
// 	}
// 	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

// 	if(strlen(argv[1]) >= MAX_FNAME) {
// 		printf("[%d] error: maximum file length is %d\n", pid, MAX_FNAME);
// 		return EXIT_FAILURE;
// 	}

// 	/* Implement code */


// 	printf("[%d] terminted\n", pid);

// 	return EXIT_SUCCESS;
// }


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

	/* 서버가 만들어 둔 메시지 큐에 접근 */
	id_msg = msgget((key_t)KEY_MSG, 0666 | IPC_CREAT);
	if(id_msg == -1) {
		printf("[%d] error: msgget() failed: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}

	/* 경로1~2: 요청 전송 (mtype = 1, fname, pid) */
	memset(&msgbuf, 0, sizeof(msgbuf));
	msgbuf.mtype = MTYPE_C2S;
	strncpy(msgbuf.fname, argv[1], MAX_FNAME - 1);
	msgbuf.pid = pid;

	ret = msgsnd(id_msg, &msgbuf, sizeof(msgbuf) - sizeof(long), 0);
	if(ret == -1) {
		printf("[%d] error: msgsnd() failed: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}

	/* 경로4: mtype이 자신의 PID인 응답 수신 */
	ret = msgrcv(id_msg, &msgbuf, sizeof(msgbuf) - sizeof(long), pid, 0);
	if(ret == -1) {
		printf("[%d] error: msgrcv() failed: %s\n", pid, strerror(errno));
		return EXIT_FAILURE;
	}

	/* 결과 출력 */
	if(msgbuf.valid) {
		printf("[%d] %s: size = %lld bytes\n", pid, msgbuf.fname,
				(long long)msgbuf.size);
		printf("[%d] %s: mode = %07o\n", pid, msgbuf.fname,
				(unsigned int)msgbuf.mode);
	} else {
		printf("[%d] %s: invalid file\n", pid, msgbuf.fname);
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}