#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int
main(int argc,char*argv[]){
	if(argc!=1)
		fprintf(2,"Too many or too small number of arguments...\n");
	int pipefd[2];//a pipe in which 0 for read and 1 for write
	char buffer[256];
	int status;
	//create the pipe
	if(pipe(pipefd)==-1){
		fprintf(2,"pipe creation is failed");
		exit(1);
	}
	//In child process
	if(fork()==0){
		int new_fd=dup(pipefd[0]);
		read(new_fd,buffer,sizeof(buffer));
		printf("%d:received %s\n",getpid(),buffer);
		close(new_fd);
		memset(buffer,0,sizeof(buffer));
		write(pipefd[1],"pong",4);
		close(pipefd[1]);
		exit(0);
	}else{//In parent process
		
		write(pipefd[1],"ping",4);
		close(pipefd[1]);
		int child_pid=wait(&status);	
		if(child_pid==-1){
			fprintf(2,"waiting for child process error\n");
			exit(1);
		}
		int new_fd=dup(pipefd[0]);
		read(new_fd,buffer,sizeof(buffer));
		printf("%d:received %s\n",getpid(),buffer);
		close(new_fd);
		
	}
	exit(0);
}
