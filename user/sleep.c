#include"kernel/types.h"
#include"kernel/stat.h"
#include"user/user.h"

int main(int argc,char* argv[]){
	if(argc<=1)
		fprintf(2,"usage:sleep [user-specified number of ticks]\n");
	int puticks=atoi(argv[1]);//user-specified No. of ticks
	char* message="nothing happens to a little while\n";
	printf(message);
	sleep(puticks);
	exit(0);
}
