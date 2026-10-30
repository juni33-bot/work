#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

pid_t pid;

int main(int argc, char **argv)
{
	int ret, len;
	FILE *fp_w;

	if(argc != 2) {
		printf("usage: %s {string}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s %s\n", pid = getpid(), argv[0], argv[1]);

	fp_w = popen("tr a-z A-Z", "w");
	if(fp_w == NULL) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	len = strlen(argv[1])+1;
	fwrite(argv[1], 1, len, fp_w);

	ret = pclose(fp_w);
	printf("\n[%d] ret = 0x%x\n", pid, ret);

	printf("[%d] terminted\n", pid);

	return EXIT_SUCCESS;
}
