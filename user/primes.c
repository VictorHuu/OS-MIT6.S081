#include "kernel/types.h"
#include "user/user.h"


int get_prime(int lpipe[2])
{
  int res=0;
  if (read(lpipe[0], &res, sizeof(int)) == sizeof(int)) {
    printf("prime %d\n", res);
    return res;
  }
  return -1;
}

void get_rest(int lpipe[2], int rpipe[2], int first)
{
  int data;
  // read data from the LEFT pipe
  while (read(lpipe[0], &data, sizeof(int)) == sizeof(int)) {
    // sieve the data that can't be divided entirely(the 1st had been fetched in the get_prime function)
    if (data % first)
      write(rpipe[1], &data, sizeof(int));
  }
  close(lpipe[0]);//close the read-side of the previous one
  close(rpipe[1]);//close the write-side of the current one
}
// Eratothese Sieve
void sieve(int lpipe[2])
{
  close(lpipe[1]);
  int first=get_prime(lpipe);
  if(first==-1){
  	exit(1);
  }else{//Get the first number(prime) successfully
    int p[2];
    pipe(p); // the current pipe

    if (fork() == 0) {
      sieve(p);    // child: recursive
    } else {
    	get_rest(lpipe, p, first);
      close(p[0]);
      wait(0);
    }
  }
  exit(0);
}

int main(int argc, char*argv[])
{
  if(argc>1){
  	fprintf(2,"The arguments are too many!\n");
  	exit(1);
  }
  int p[2];
  pipe(p);
  if (fork() == 0) {
    sieve(p);
  } else {
  	for (int i = 2; i <= 35; ++i){ //feeds the number 2 to 35 into the pipeline
    		write(p[1], &i, sizeof(i));
	}
    	close(p[1]);
    	close(p[0]);
    	wait(0);
  }
  exit(0);
}
