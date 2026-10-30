#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

typedef void (*sighandler_t)(int);
pid_t pid;

void sigint_handler(int signal)
{
	printf("[%d] got signal %d\n", pid, signal);
}

int main(int argc, char **argv)
{
	sighandler_t sig_ret;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("[%d] running %s\n", pid = getpid(), argv[0]);

#if 0
	sig_ret = signal(SIGINT, sigint_handler);
	if(sig_ret == SIG_ERR) {
		printf("[%d] can't set signal handler\n", pid);
		return EXIT_FAILURE;
	}
#endif

#if 0
	sig_ret = signal(SIGINT, SIG_DFL);
	if(sig_ret == SIG_ERR) {
		printf("[%d] can't set signal handler\n", pid);
		return EXIT_FAILURE;
	}
#endif

#if 1
	sig_ret = signal(SIGINT, SIG_IGN);
	if(sig_ret == SIG_ERR) {
		printf("[%d] can't set signal handler\n", pid);
		return EXIT_FAILURE;
	}
#endif

	for(;;) {
		printf("[%d] sleeping 1 second\n", pid);
		sleep(1);
	}

	printf("[%d] terminated\n", pid);

	return EXIT_SUCCESS;
}

