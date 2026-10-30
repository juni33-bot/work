#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int ret, src_fd, dst_fd;
	char c, *src_name, *dst_name;
	unsigned int copied = 0;

	if(argc != 3) {
		printf("usage: %s {src file} {dst file}\n", argv[0]);
		return EXIT_FAILURE;
	}
	printf("running %s %s %s\n", argv[0], argv[1], argv[2]);

	src_name = argv[1];
	dst_name = argv[2];

	/* open source file */
	src_fd = open(src_name, O_RDONLY);
	if(src_fd == -1) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/* open destination file */
	dst_fd = open(dst_name, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
	if(dst_fd == -1) {
		printf("error: %s (%d)\n", strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}

	/* copy */
	for(;;) {
		ret = read(src_fd, &c, 1);
		if(ret == -1) {
			printf("error: %s (%d)\n", strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		if(ret == 0) break;

		ret = write(dst_fd, &c, 1);
		if(ret == -1) {
			printf("error: %s (%d)\n", strerror(errno), __LINE__);
			return EXIT_FAILURE;
		}
		copied++;
	}

	/* close */
	close(src_fd);
	close(dst_fd);

	printf("%u bytes copied\n", copied);

	return EXIT_SUCCESS;
}

