#include <stdio.h>

extern int plus(int a, int b);
extern int minus(int a, int b);

int main(int argc, char **argv)
{
	printf("3 + 2 = %d\n", plus(3, 2));
	printf("3 - 2 = %d\n", minus(3, 2));

	return 0;
}
