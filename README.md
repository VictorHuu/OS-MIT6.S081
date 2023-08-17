# Lab: traps
## RISC-V assembly (easy)
### 答案#1：
a0~a7 我们必须注意到 const char* fmt 的指针存储在 a0 上，a0 上为字符串分配了内存空间。
13 是第三个参数，因此它存储在 a2 中。
### 答案#2：
由于编译器内联了函数 f 和 g，因此很难找到确切的调用入口。

> 24:	4635                	li	a2,13
> 26:	45b1                	li	a1,12

看上面的asm行，我们直接得到了a2和a1，这表明a2是通过inline机制预先生成的。
请注意，main 的条目是 1c，这有点奇怪，而 f 的条目是 0x0..e（带有 instr）。 14~1a 与g函数相同，因此g内联到f中，f内联到main中。
回到最初的问题，函数f的调用在0x...0e处，而函数g的调用在14处，后续指令属于main。
### 答案#3：
0x628
### 答案#4：
>  30: 00000097 auipc ra,0x0

| Imm                      | dest   | opcode   |
| ------------------------ | ------ | -------- |
| 0000 0000 0000 0000 0000 | 0000 1 | 001 0111 |

auipc 将左移 12 位的偏移量 0x0 添加到 30 的 pc 上，结果 ra 为 0x30。

| Imm            | rs1   | funct3 | rd1   | opcode   |
| -------------- | ----- | ------ | ----- | -------- |
| 0101 1111 1000 | 00001 | 000    | 00001 | 110 0111 |

立即数为 1528 或 0x5F8H。在 ra:0x30 中加上 Imm 后，ra 变为 0x628。
简而言之，就是main中的jalr到printf之后的0x628。
### 答案#5：
```sh
HE110 World
```
### 答案#6：
```sh
x=3 y=5213
```

因为即使没有第三个参数，但在 printf 中 a2 寄存器被分配给 16(s0)，所以第二个变量 y 将是 trapframe->a2 。在这种情况下，它是 5213！
## Backtrace (moderate)
根据提示一二，先在defs.h中定义backtrace，在riscv.h中添加r_fp()的声明。
当backtrace被调用时，可以通过r_fp()获得当前的pageframe，根据提示4来确定循环条件进行遍历，获得的framepage如果没有调用者，那么就没有fp，也就是说fp的大小为0,PGROUNDUP(fp)-PGROUNDDOWN(fp)=0,终止循环。
根据提示3，地址位于当前帧页中偏移量为-8的地方，被调用者fp位于偏移量为-16的地方。
```c
void backtrace(void){
	printf("backtrace:\n");
	uint64 fp=r_fp();
	while(PGROUNDUP(fp)-PGROUNDDOWN(fp)>0){
		uint64* addr=(uint64*)(fp-8);
		fp=*(uint64*)(fp-16);
		printf("%p\n",*addr);
	}
}
```
运行结果如下：

![4b1fa90c1d0a66bf22db79025d079366](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/2d9d9f47-65ad-4e56-ad9e-205f370d30e8)

![e5e3c763dabb0b852efb1fd8e4895ad6](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/9b0844ac-c18b-4fe5-908e-4cc37d5db2a3)
## Alarm(hard)
### test0: invoke handler
首先根据系统调用的经验，将必要的文件都配置好。

在proc.h中，新声明了3个变量：
```c
typedef void(*AlarmFunc) (void);
struct proc{
  ...
  int a_intvl;			//Alarm interval
  AlarmFunc handler;		//Invoker Handler
  int elapsed_ticks;		//number of ticks have passed since the last call
}
``` 
用typedef定义函数指针方便之后的处理。
然后是函数sys_sigalarm，从trapframe上面的ax等寄存器获取传输的变量，
如果获取失败就返回-1，并赋给当前进程对应的proc的相应的值。
```c
uint64 sys_sigalarm(void){
	int ticks;
	uint64 fn;
	struct proc* p=myproc();
	if(argint(0,&ticks)<0)
		return -1;
	if(argaddr(1,&fn)<0)
		return -1;
	p->a_intvl=ticks;
	p->handler=(AlarmFunc)fn;
	return 0;
}
```
在usertrap函数中，当which_dev==2，表示发生了时钟中断的时候开始进行处理
```c
  if(which_dev == 2){
    p->elapsed_ticks++;
    if(p->elapsed_ticks>p->a_intvl&&p->a_intvl>0){
        p->trapframe->epc=(uint64)p->handler;
    	p->elapsed_ticks=0;
    }
```
将epc即用户空间的PC值设置为处理函数，那么当从内核态返回用户态时将会执行相对应的函数。

![ea5431be62fd3fc602ea4c46529ba0c2](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/eccde6b1-954b-4353-9831-4970805c4f30)

第一个实验测试通过
### test1/test2(): resume interrupted code
- 这个实验最重要的是需要存储非常多寄存器，一般寄存器都位于trapframe中，因此需要整体复制，
可以在proc中设置一个alarm_trapframe，用于存储相应的寄存器信息，以便于恢复。
当发生时钟中断的时候，需要复制trapframe到alarm_trapframe中，当执行sigreturn函数的时候，需要恢复trapframe。
- 同样的，为了防止处理函数还没有结束就再次调用，需要在proc中设置一个in_alarming，0表示没有调用处理函数，可以执行处理函数。
当发生时钟中断的时候，需要设置in_alarming=1，当执行sigreturn函数的时候，in_alarming=0，表示处理函数执行完毕。
```c
uint64 sys_sigreturn(void){
 	struct proc*p=myproc();
  	memmove(p->trapframe,p->alarm_trapframe,sizeof(struct trapframe));
  	p->in_alarming=0;
	return 0;
}
```
```c
 if(which_dev == 2){
    p->elapsed_ticks++;
    if(p->elapsed_ticks>p->a_intvl&&p->a_intvl>0&&p->in_alarming==0){
        memmove(p->alarm_trapframe,p->trapframe,sizeof(struct trapframe));
        p->trapframe->epc=(uint64)p->handler;
    	p->elapsed_ticks=0;
    	p->in_alarming=1;

    }
```
- 在进程初始化的时候，或者释放的时候，需要对新增的成员进行处理，模仿其他成员的处理方式即可。
```c
In allocproc:
// Allocate a alarm_trapframe page.
  if((p->alarm_trapframe = (struct trapframe *)kalloc()) == 0){
    release(&p->lock);
    return 0;
  }
In freeproc:
if(p->alarm_trapframe)
    kfree((void*)p->alarm_trapframe);
```
其他新增变量设置为0就可以了。

以下是实验结果：

![2df33296a0ca6236cdf12d7481a155e4](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/ce6de1d9-0a5a-4872-99f9-b2149cfcedd1)

