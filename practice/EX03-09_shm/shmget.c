// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <errno.h>
// #include <string.h>
// #include <fcntl.h>
// #include <sys/types.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>

// #include "shm.h"

// pid_t pid;

// int main(int argc, char **argv)
// {
// 	int ret, i;
// 	int id_shm;
// 	struct shm_buf *shmbuf;
// 	unsigned long cs = 0;

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
// 		if(shmbuf->status != STATUS_VALID) {
// 			break;
// 		}

// 		for(i=0, cs=0; i<sizeof(struct shm_buf)-sizeof(unsigned long); i++) {
// 			cs += ((unsigned char *)shmbuf)[i];
// 		}
// 		if(shmbuf->cs != cs) {
// 			printf("[%d] error: invalid checksum (checksum=%ld, calculated=%ld)\n", pid, shmbuf->cs, cs);
// 			break;
// 		}

// 		printf("[%d] time=%s, checksum=%lu\n", pid, shmbuf->buf, shmbuf->cs);

// 		usleep(1000000);
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

#include "shm.h"

/*
 * shm.h에 KEY_SEM이 없으면 여기서 정의한다.
 * 값은 shmput.c, sem 도구와 반드시 같아야 한다.
 */
#ifndef KEY_SEM
#define KEY_SEM 1234
#endif

pid_t pid;

/* 세마포어 P 연산 (lock) */
static int sem_lock(int id_sem)
{
	struct sembuf sb;

	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = SEM_UNDO;

	while(semop(id_sem, &sb, 1) == -1) {
		if(errno == EINTR) {
			continue;
		}
		return -1;
	}
	return 0;
}

/* 세마포어 V 연산 (unlock) */
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

/* 세마포어를 열고, 없으면 만들면서 초기값 1로 설정 (shmput과 동일한 방식) */
static int open_sem(void)
{
	int id_sem;

	id_sem = semget((key_t)KEY_SEM, 1, 0666);
	if(id_sem != -1) {
		return id_sem;
	}
	if(errno != ENOENT) {
		return -1;
	}

	id_sem = semget((key_t)KEY_SEM, 1, 0666|IPC_CREAT|IPC_EXCL);
	if(id_sem != -1) {
		union {
			int val;
			struct semid_ds *buf;
			unsigned short *array;
		} su;

		su.val = 1;
		if(semctl(id_sem, 0, SETVAL, su) == -1) {
			return -1;
		}
		printf("[%d] semaphore created (KEY_SEM=%d)\n", pid, KEY_SEM);
		return id_sem;
	}
	if(errno == EEXIST) {
		/* 그 사이 다른 프로세스가 만들었으면 그것을 사용 */
		return semget((key_t)KEY_SEM, 1, 0666);
	}
	return -1;
}

int main(int argc, char **argv)
{
	int ret, i;
	int id_shm;
	int id_sem;
	struct shm_buf *shmbuf;
	struct shm_buf snap;	/* 공유 메모리의 스냅샷 (락 밖에서는 이것만 사용) */
	unsigned long cs = 0;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

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

	id_sem = open_sem();
	if(id_sem == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	for(;;) {
		/* ---------- 크리티컬 영역 시작: 공유 메모리를 통째로 복사만 한다 ---------- */
		if(sem_lock(id_sem) == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}

		memcpy(&snap, shmbuf, sizeof(struct shm_buf));

		if(sem_unlock(id_sem) == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		/* ---------- 크리티컬 영역 끝 ---------- */

		/* 이후 검증과 출력은 모두 복사본(snap)으로 한다 */
		if(snap.status != STATUS_VALID) {
			break;
		}

		for(i=0, cs=0; i<sizeof(struct shm_buf)-sizeof(unsigned long); i++) {
			cs += ((unsigned char *)&snap)[i];
		}
		if(snap.cs != cs) {
			printf("[%d] error: invalid checksum (checksum=%ld, calculated=%ld)\n", pid, snap.cs, cs);
			break;
		}

		printf("[%d] time=%s, checksum=%lu\n", pid, snap.buf, snap.cs);

		usleep(1000000);
	}

	ret = shmdt(shmbuf);
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}