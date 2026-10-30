#define KEY_SHM 0x2220

#define STATUS_INVALID 0
#define STATUS_VALID 0xbeafbeaf

#define TIMEOUT 30
#define MAX_BUF 64

struct shm_buf {
	int status;
	char buf[MAX_BUF];
	unsigned long cs;
};

