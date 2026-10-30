#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BLOCK_SIZE 4096

int main(int argc, char **argv)
{
	int block_size;
	size_t uret1, uret2, copied = 0;
	FILE *src_fp, *dst_fp;
	char *src_name, *dst_name;
	char buf[MAX_BLOCK_SIZE];
	int eof_flag = 0;

	if(argc != 4) {
		printf("usage: %s {src file} {dst file} {block size in bytes}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("running %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

	src_name = argv[1];
	dst_name = argv[2];
	block_size = atoi(argv[3]);

	if(block_size > MAX_BLOCK_SIZE) {
		printf("error: block size if too big (maximum is %d bytes (%d)\n", MAX_BLOCK_SIZE, __LINE__);
		return EXIT_FAILURE;
	}

	/* open source file */
	src_fp = fopen(src_name, "r");
	if(src_fp == NULL) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/* open destination file */
	dst_fp = fopen(dst_name, "w");
	if(dst_fp == NULL) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/* copy */
	for(;;) {
		uret1 = fread(buf, 1, (size_t)block_size, src_fp);
		if(uret1 < (size_t)block_size) {
			if(feof(src_fp) != 0) {
				eof_flag = 1;
			}
			else {
				printf("error: fread failed (%d)\n", __LINE__);
				return EXIT_FAILURE;
			}
		}

		uret2 = fwrite(buf, 1, uret1, dst_fp);
		if(uret2 < uret1) {
			printf("error: fwrite failed (%d)\n", __LINE__);
			return EXIT_FAILURE;
		}
		copied += uret2;
		if(eof_flag) break;
	}

	/* close */
	fclose(src_fp);
	fclose(dst_fp);

	printf("%lu bytes copied\n", copied);

	return EXIT_SUCCESS;
}

