#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

pid_t pid;

int main(int argc, char **argv)
{
	int ret;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

#if 1 /* A */
	ret = execl("/bin/ls", "ls", "-l", NULL);
#endif

#if 0 /* B */
	ret = execl("/bin/ls", "xxx", "-l", NULL);
#endif

#if 0 /* C */
	ret = execl("ls", "ls", "-l", NULL);
#endif

#if 0 /* D */
	ret = execlp("ls", "ls", "-l", NULL);
#endif

	printf("[%d] ret = %d\n", pid, ret);

	printf("[%d] terminted\n", pid);

	return EXIT_FAILURE;
}

