// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <errno.h>
// #include <string.h>
// #include <fcntl.h>
// #include <sys/types.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/time.h>

// #include "shm.h"

// pid_t pid;

// int main(int argc, char **argv)
// {
// 	int ret;
// 	int id_shm;
// 	struct shm_buf *shmbuf;
// 	int i;
// 	struct timeval tv;
// 	time_t start_time;
// 	int first = 1;
// 	unsigned long cs;

// 	if(argc != 1) {
// 		printf("usage: %s\n", argv[0]);
// 		return EXIT_FAILURE;
// 	}
// 	printf("[%d] running %s\n", pid = getpid(), argv[0]);

// 	id_shm = shmget((key_t)KEY_SHM, sizeof(struct shm_buf), 0666|IPC_CREAT);
// 	if(id_shm == -1) {
// 		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
// 		return EXIT_FAILURE;
// 	}

// 	shmbuf = shmat(id_shm, (void *)0, 0);
// 	if(shmbuf == (struct shm_buf *)-1) {
// 		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
// 		return EXIT_FAILURE;
// 	}

// 	for(;;) {
// 		if(gettimeofday(&tv, NULL) == -1) {
// 			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
// 			return EXIT_FAILURE;
// 		}

// 		if(first) {
// 			first = 0;
// 			start_time = tv.tv_sec;
// 		}
// 		else {
// 			if(tv.tv_sec - start_time > TIMEOUT) {
// 				shmbuf->status = STATUS_INVALID;
// 				break;
// 			}
// 		}

// 		sprintf(shmbuf->buf, "%lu.%06lu", tv.tv_sec, tv.tv_usec);
// 		shmbuf->status = STATUS_VALID;
// 		for(i=0, cs=0; i<sizeof(struct shm_buf)-sizeof(unsigned long); i++) {
// 			cs += ((unsigned char *)shmbuf)[i];
// 		}
// 		shmbuf->cs = cs;

// 		//usleep(100000);

// 	}

// 	ret = shmdt(shmbuf);
// 	if(ret == -1) {
// 		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
// 		return EXIT_FAILURE;
// 	}

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
#include <sys/sem.h>
#include <sys/time.h>

#include "shm.h"

/*
 * shm.h에 KEY_SEM이 없으면 여기서 정의한다.
 * 값은 sem 도구(sem.c)를 만들 때 쓴 sem.h의 KEY_SEM과 반드시 같아야 한다.
 */
#ifndef KEY_SEM
#define KEY_SEM 1234
#endif

pid_t pid;

/* 세마포어 P 연산 (lock): 값을 1 감소, 0이면 대기 */
static int sem_lock(int id_sem)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;	/* 프로세스가 비정상 종료해도 자동 복구 */

	while(semop(id_sem, &sb, 1) == -1) {
		if(errno == EINTR) {
			continue;	/* 시그널로 중단되면 재시도 */
		}
		return -1;
	}
	return 0;
}

/* 세마포어 V 연산 (unlock): 값을 1 증가 */
static int sem_unlock(int id_sem)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = SEM_UNDO;

	while(semop(id_sem, &sb, 1) == -1) {
		if(errno == EINTR) {
			continue;
		}
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret;
	int id_shm;
	int id_sem;
	struct shm_buf *shmbuf;
	int i;
	struct timeval tv;
	time_t start_time;
	int first = 1;
	unsigned long cs;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);
	printf("[%d] build: %s %s (sem auto-create version)\n", pid, __DATE__, __TIME__);

	id_shm = shmget((key_t)KEY_SHM, sizeof(struct shm_buf), 0666|IPC_CREAT);
	if(id_shm == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	shmbuf = shmat(id_shm, (void *)0, 0);
	if(shmbuf == (struct shm_buf *)-1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/*
	 * 세마포어가 이미 있으면(sem SETVAL로 만들어 둔 경우) 그대로 사용하고,
	 * 없으면 여기서 직접 만들고 초기값 1로 설정한다.
	 */
	id_sem = semget((key_t)KEY_SEM, 1, 0666);
	if(id_sem == -1) {
		if(errno != ENOENT) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}

		/* IPC_EXCL: 동시에 다른 프로세스가 먼저 만들었는지 구분 */
		id_sem = semget((key_t)KEY_SEM, 1, 0666|IPC_CREAT|IPC_EXCL);
		if(id_sem != -1) {
			union {
				int val;
				struct semid_ds *buf;
				unsigned short *array;
			} su;

			su.val = 1;
			if(semctl(id_sem, 0, SETVAL, su) == -1) {
				printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
				return EXIT_FAILURE;
			}
			printf("[%d] semaphore created (KEY_SEM=%d)\n", pid, KEY_SEM);
		}
		else if(errno == EEXIST) {
			/* 그 사이 다른 프로세스가 만들었으면 그것을 사용 */
			id_sem = semget((key_t)KEY_SEM, 1, 0666);
		}

		if(id_sem == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
	}

	for(;;) {
		if(gettimeofday(&tv, NULL) == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}

		if(first) {
			first = 0;
			start_time = tv.tv_sec;
		}
		else {
			if(tv.tv_sec - start_time > TIMEOUT) {
				/* 종료 표시도 공유 메모리 쓰기이므로 크리티컬 영역 안에서 */
				if(sem_lock(id_sem) == -1) {
					printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
					return EXIT_FAILURE;
				}
				shmbuf->status = STATUS_INVALID;
				if(sem_unlock(id_sem) == -1) {
					printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
					return EXIT_FAILURE;
				}
				break;
			}
		}

		/* ---------- 크리티컬 영역 시작 ---------- */
		if(sem_lock(id_sem) == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}

		sprintf(shmbuf->buf, "%lu.%06lu", tv.tv_sec, tv.tv_usec);
		shmbuf->status = STATUS_VALID;
		for(i=0, cs=0; i<sizeof(struct shm_buf)-sizeof(unsigned long); i++) {
			cs += ((unsigned char *)shmbuf)[i];
		}
		shmbuf->cs = cs;

		if(sem_unlock(id_sem) == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		/* ---------- 크리티컬 영역 끝 ---------- */

		//usleep(100000);
	}

	ret = shmdt(shmbuf);
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}