#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	int x = 42;
	int *y = NULL;
	y = &x;
	printf("x = %d\n", x);
	printf("y = 0x%x\n", *y);

	printf("size int = %ld\n", sizeof(x));
	printf("size int * = %ld\n", sizeof(y));
	printf("size void * = %ld\n", sizeof(void *));
	printf("size float * = %ld\n", sizeof(float *));
	printf("size float = %ld\n", sizeof(float));
	printf("size long = %ld\n", sizeof(long));
	return EXIT_SUCCESS;
}