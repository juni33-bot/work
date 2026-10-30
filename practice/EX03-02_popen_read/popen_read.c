#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

pid_t pid;

#define BUF_SIZE 32

int main(int argc, char **argv)
{
	int ret;
	size_t ulen;
	FILE *fp_r;
	char buf[BUF_SIZE];
	char cmd[BUF_SIZE];

	if(argc != 2) {
		printf("usage: %s {directory}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	sprintf(cmd, "ls %s", argv[1]);
	fp_r = popen(cmd, "r");
	if(fp_r == NULL) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	for(;;) {
		ulen = fread(buf, 1, BUF_SIZE-1, fp_r);
		if(ulen == 0) break;
		buf[ulen] = '\0';
		printf("%s", buf);
	}

	ret = pclose(fp_r);
	printf("[%d] ret = 0x%x\n", pid, ret);

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
