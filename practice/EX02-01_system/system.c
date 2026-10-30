#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
	int ret;

	if(argc != 1) {
		printf("usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("running %s\n", argv[0]);

	printf("command = ls -l\n");
	ret = system("ls -l");
	printf("-> ret = %#x (%#x)\n", ret, WEXITSTATUS(ret));

	printf("command = wrong command\n");
	ret = system("wrong command");
	printf("-> ret = %#x (%#x)\n", ret, WEXITSTATUS(ret));

	printf("command = sleep 3\n");
	ret = system("sleep 3");
	printf("-> ret = %#x (%#x)\n", ret, WEXITSTATUS(ret));

	printf("command = ls /etc | grep init > init.txt\n");
	ret = system("ls /etc | grep init > init.txt");
	printf("-> ret = %#x (%#x)\n", ret, WEXITSTATUS(ret));

	return EXIT_SUCCESS;
}

