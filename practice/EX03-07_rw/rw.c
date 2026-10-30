#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define FILE_TEST "./test.txt"
#define LOOP_COUNT 50

pid_t pid;

int main(int argc, char **argv)
{
	int ret;
	int i;
	char c1 = 'A';
	char c2;
	int fd;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	for(i=0; i<LOOP_COUNT; i++) {
		printf("[%d] LOOP %d\n", pid, i);
		fd = open(FILE_TEST, O_RDWR|O_CREAT, 0777);
		if(fd == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		ret = write(fd, &c1, sizeof(char));
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		ret = close(fd);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}

		usleep(rand()%500000);

		fd = open(FILE_TEST, O_RDWR);
		if(fd == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		ret = read(fd, &c2, sizeof(char));
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		ret = close(fd);
		if(ret == -1) {
			printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		if(c1 != c2) {
			printf("[%d] error: not same (%d)\n", pid,  __LINE__);
			return EXIT_FAILURE;
		}

		c1++;
	}

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
