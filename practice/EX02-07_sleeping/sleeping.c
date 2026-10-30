#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pid_t pid;

int main(int argc, char **argv)
{
	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

	for(;;) {
		printf("[%d] sleeping 1 second\n", pid);
		sleep(1);
	}

	printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}

