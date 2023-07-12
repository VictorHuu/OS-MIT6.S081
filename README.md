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

