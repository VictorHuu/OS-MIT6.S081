#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}
char* getname(char* path){
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;

}
int get_type(char * path){
  int fd;
  struct stat st;
if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return -1;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return -1;
  }
  return st.type;
}
void
find(char *path,char* pattern)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_DEVICE:
  case T_FILE:
    printf("%sn",path);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      if(st.type==T_FILE&&strcmp(pattern,getname(buf))==0)
      	printf("%s\n", buf);
      else if(st.type==T_DIR&&strcmp(getname(buf),".")&&strcmp(getname(buf),"..")){
      	find(buf,pattern);
      }
    }
    break;
  }
  close(fd);
}
int
main(int argc, char *argv[]){
	for(int i=1;i<argc;i+=2){
		char* spath=argv[i];
		char* pattern=argv[i+1];
		if(argc>3)
			printf("%s:\n",spath);
		find(spath,pattern);
		
	}
	exit(0);
}
