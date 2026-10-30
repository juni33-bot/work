// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <errno.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

// #define MAX_BLOCK_SIZE 4096

// int main(int argc, char **argv)
// {
// 	int ret, src_fd, dst_fd, block_size;
	
// 	char *src_name, *dst_name;
// 	char buf[MAX_BLOCK_SIZE];
// 	unsigned int copied = 0;

// 	if(argc != 4) {
// 		printf("usage: %s {src file} {dst file} {block size in bytes}\n", argv[0]);
// 		return EXIT_FAILURE;
// 	}
// 	printf("running %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

// 	src_name = argv[1];
// 	dst_name = argv[2];
// 	block_size = atoi(argv[3]);

// 	if(block_size > MAX_BLOCK_SIZE) {
// 		printf("error: block size if too big (maximum is %d bytes (%d)\n", MAX_BLOCK_SIZE, __LINE__);
// 		return EXIT_FAILURE;
// 	}
// /* open */
// 	src_fd = open(*dst_name, O_WRONLY | O_EXCL | O_CREAT, S_IRUSR | S_IWUSR);
// 	if(src_fd == -1) {
// 		printf("error: %s (%d)\n", strerror(errno), __LINE__);
// 		return EXIT_FAILURE;

// 	}
	

// 	/* write */
// 	for(ret = 0; ret < block_size; ret++) {
// 		*src_name = (ret % 10) + '0';
// 		ret = write(src_fd, &*src_name, 1024);
// 		if(ret == -1) {
// 			printf("error: %s (%d)\n", strerror(errno), __LINE__);
// 			return EXIT_FAILURE;

// 		}
// 	}

// 	/* close */
// 	close(src_fd);

// 	printf("%s has been created\n",  *dst_name);


// 	return EXIT_SUCCESS;
// }

///////////////////////////////////////////////////////////////

// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <errno.h>
// #include <string.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <fcntl.h>

// #define MAX_BLOCK_SIZE 4096

// int main(int argc, char **argv)
// {
// 	int ret, src_fd, dst_fd, block_size;
// 	char *src_name, *dst_name;
// 	char buf[MAX_BLOCK_SIZE];
// 	unsigned int copied = 0;

// 	if(argc != 4) {
// 		printf("usage: %s {src file} {dst file} {block size in bytes}\n", argv[0]);
// 		return EXIT_FAILURE;
// 	}
// 	printf("running %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

// 	src_name = argv[1];
// 	dst_name = argv[2];
// 	block_size = atoi(argv[3]);

// 	if(block_size > MAX_BLOCK_SIZE) {
// 		printf("error: block size is too big (maximum is %d bytes (%d))\n", MAX_BLOCK_SIZE, __LINE__);
// 		return EXIT_FAILURE;
// 	}

// 	/* 1. 원본 파일 읽기 전용으로 열기 */
// 	src_fd = open(src_name, O_RDONLY);
// 	if(src_fd == -1) {
// 		printf("error: %s (%d)\n", strerror(errno), __LINE__);
// 		return EXIT_FAILURE;
// 	}

// 	/* 2. 사본 파일 쓰기 전용으로 열기 (없으면 생성, 기존 내용 초기화) */
// 	dst_fd = open(dst_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
// 	if(dst_fd == -1) {
// 		printf("error: %s (%d)\n", strerror(errno), __LINE__);
// 		close(src_fd);
// 		return EXIT_FAILURE;
// 	}

// 	/* 3. 블록 단위로 읽고 쓰기 반복 */
// 	while((ret = read(src_fd, buf, block_size)) > 0) {
// 		if(write(dst_fd, buf, ret) != ret) {
// 			printf("error: %s (%d)\n", strerror(errno), __LINE__);
// 			close(src_fd);
// 			close(dst_fd);
// 			return EXIT_FAILURE;
// 		}
// 		copied += ret;
// 	}

// 	if(ret == -1) {
// 		printf("error: %s (%d)\n", strerror(errno), __LINE__);
// 		close(src_fd);
// 		close(dst_fd);
// 		return EXIT_FAILURE;
// 	}

// 	/* 4. 파일 닫기 */
// 	close(src_fd);
// 	close(dst_fd);

// 	printf("%s has been copied to %s (%u bytes)\n", src_name, dst_name, copied);

// 	return EXIT_SUCCESS;
// }

/////////////////////////////////////////////////////
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
    int src_fd = -1;
    int dst_fd = -1;
    char *end;
    long parsed_block_size;
    int block_size;
    char *src_name;
    char *dst_name;
    char buf[MAX_BLOCK_SIZE];
    size_t copied = 0;
    ssize_t nread;

    if (argc != 4) {
        fprintf(stderr, "usage: %s {src file} {dst file} {block size in bytes}\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    src_name = argv[1];
    dst_name = argv[2];

    errno = 0;
    parsed_block_size = strtol(argv[3], &end, 10);
    if (errno == ERANGE || end == argv[3] || *end != '\0' ||
        parsed_block_size <= 0 || parsed_block_size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "error: block size must be between 1 and %d\n",
                MAX_BLOCK_SIZE);
        return EXIT_FAILURE;
    }
    block_size = (int)parsed_block_size;

    src_fd = open(src_name, O_RDONLY);
    if (src_fd == -1) {
        perror("open source");
        goto fail;
    }

    /* O_EXCL means the destination must not already exist. */
    dst_fd = open(dst_name, O_WRONLY | O_CREAT | O_EXCL,
                  S_IRUSR | S_IWUSR);
    if (dst_fd == -1) {
        perror("open destination");
        goto fail;
    }

    for (;;) {
        nread = read(src_fd, buf, (size_t)block_size);
        if (nread == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            goto fail;
        }
        if (nread == 0)
            break;

        size_t written = 0;
        while (written < (size_t)nread) {
            ssize_t nwritten = write(
                dst_fd,
                buf + written,
                (size_t)nread - written
            );

            if (nwritten == -1) {
                if (errno == EINTR)
                    continue;
                perror("write");
                goto fail;
            }
            if (nwritten == 0) {
                fprintf(stderr, "error: write made no progress\n");
                goto fail;
            }

            written += (size_t)nwritten;
        }

        copied += written;
    }

    if (close(src_fd) == -1) {
        src_fd = -1;
        perror("close source");
        goto fail;
    }
    src_fd = -1;

    if (close(dst_fd) == -1) {
        dst_fd = -1;
        perror("close destination");
        goto fail;
    }
    dst_fd = -1;

    printf("%zu bytes copied\n", copied);
    return EXIT_SUCCESS;

fail:
    if (src_fd != -1)
        close(src_fd);
    if (dst_fd != -1)
        close(dst_fd);
    return EXIT_FAILURE;
}