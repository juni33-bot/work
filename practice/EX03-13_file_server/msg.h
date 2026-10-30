#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define KEY_MSG 0x3332
#define MAX_FNAME 64

#define MTYPE_C2S 1

struct msg_buf {
	long mtype;
	char fname[MAX_FNAME];
	off_t size;
	mode_t mode;
	int valid;
	pid_t pid;
};

