#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

pid_t pid;

int main(int argc, char **argv)
{
    pid_t pid_temp; 
    int i;
    int num_of_child;
    

    printf("[%d] running %s", pid = getpid(), argv[0]);
    for(i=1; i<argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");

    num_of_child = argc - 2;

    /* Implement code */
    for(i=0; i<num_of_child; i++){
        pid_temp = fork();
        if(pid_temp == 0){
            // execlp("grep", "grep", argv[1], argv[i+2], NULL);
			execlp("grep", "grep","-r", argv[i+2], argv[1], NULL);
            printf("error: cannot excute grep: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else {
            // 부모 프로세스 영역 (필요시 추가 로직 작성)
        }
    } // 자식 생성 포문 닫기
	pid_t pid_wait[num_of_child];
	int status[num_of_child];
	
    for(i=0; i<num_of_child; i++)
		{
      
		
		pid_wait[i] = wait(&status[i]);
		// pid_wait = waitpid(-1, &status, 0);
    }
	for(i=0; i<num_of_child; i++)
		// printf("[%d]")
	printf("[%d] child %d terminated with status %d\n", pid, pid_wait[i], status[i]);

	printf("[%d] terminted\n", pid);


    return EXIT_SUCCESS;
}