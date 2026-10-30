#define KEY_MSG 0x3331

#define MTYPE_C2S 1 /* Client -> Server */
#define MTYPE_S2C 2 /* Server -> Client */

struct msg_buf {
	long mtype;
	int op1;
	int op2;
	int result;
};

