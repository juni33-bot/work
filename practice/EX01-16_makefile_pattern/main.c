#include <stdio.h>
extern void func(void);
int main(void)
{
	printf("main: Hello!\n");
	func();
	return 0;
}
