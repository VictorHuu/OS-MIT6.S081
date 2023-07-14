#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXSZ 512
//state machine
enum state {
  S_WAIT,//wating for the next param         
  S_ARG,          
  S_ARG_END,      
  S_ARG_LINE_END, 
  S_LINE_END,     
  S_END           
};

enum char_type {
  C_SPACE,
  C_CHAR,
  C_LINE_END
};
char prev='\0';
enum char_type get_char_type(char c)
{
  if(c=='n'&&prev=='\\'){
  	return C_LINE_END;
  }
  prev=c;
  switch (c) {
  case ' ':
    return C_SPACE;
  case '\n':
    return C_LINE_END;
  default:
    return C_CHAR;
  }
}

enum state transform_state(enum state cur, enum char_type ct)
{
  switch (cur) {
  case S_ARG:
    if (ct == C_SPACE)    return S_ARG_END;
    if (ct == C_LINE_END) return S_ARG_LINE_END;
    if (ct == C_CHAR)     return S_ARG;
    break;
  default:
    if (ct == C_SPACE)    return S_WAIT;
    if (ct == C_LINE_END) return S_LINE_END;
    if (ct == C_CHAR)     return S_ARG;
    break;
  }
  return S_END;
}

//clear the rest of the argument values i.e. the input params through redirection
void clearInArgs(char *x_argv[MAXARG], int beg)
{
  for (int i = beg; i < MAXARG; ++i)
    x_argv[i] = 0;
}

int main(int argc, char *argv[])
{
  if (argc - 1 >= MAXARG) {
    fprintf(2, "xargs: too many arguments.\n");
    exit(1);
  }
  char lines[MAXSZ];
  char *p = lines;
  char *x_argv[MAXARG] = {0}; 

  for (int i = 1; i < argc; ++i) {
    x_argv[i - 1] = argv[i];
  }
  int arg_beg = 0;          
  int arg_end = 0;          
  int arg_cnt = argc - 1;   
  enum state st = S_WAIT;   

  while (st != S_END) {
    if (read(0, p, sizeof(char)) != sizeof(char)) {
      st = S_END;
    } else {
      st = transform_state(st, get_char_type(*p));
    }

    if (++arg_end >= MAXSZ) {
      fprintf(2, "xargs: arguments through standard input too long.\n");
      exit(1);
    }

    switch (st) {
    case S_WAIT:          
      ++arg_beg;
      break;
    case S_ARG_END:       
      x_argv[arg_cnt++] = &lines[arg_beg];
      arg_beg = arg_end;
      *p = '\0';          
      break;
    case S_ARG_LINE_END:  
      x_argv[arg_cnt++] = &lines[arg_beg];
      
    case S_LINE_END:      
      arg_beg = arg_end;
      *p = '\0';
      if (fork() == 0) {//in child process
        exec(argv[1], x_argv);
      }else{
      	arg_cnt = argc - 1;
      	clearInArgs(x_argv, arg_cnt);
      	wait(0);
      }
      break;
    default:
      break;
    }

    ++p;    
  }
  exit(0);
}
