# REPORT syscall
## 系统调用的相关配置于systrace的编写
1. 将$U/trace添加到Makefile中
2. 修改Perl脚本usys.pl,该脚本能够产生相应的汇编代码，使用了RISC-V的ecall指令来传递给内核。ecall调用系统调用函数。
3. 接下来的部分就是仿照已有的系统调用的写法。比如要在sysproc.c中添加系统调用的函数定义。在syscall的头文件于源文件中增加该系统调用的声明即可。
4. 注意syscall.h中syscall函数的用法，syscall根据进程空间trapframe中的相关寄存器值获取系统调用的号码，然后用函数指针数组定位到函数指针从而进行调用。
5. mask确定了是哪一个syscall，右移相应的位数就可以确定是否被调用。如果可以，打印相应的信息。
```C
if((p->mask>>num)%2==1)
        printf("%d: syscall %s -> %d \n",myproc()->pid,syscall_names[num-1],p->trapframe->a0);
```
6. 注意在user用户空间下声明系统调用函数。
7. 对于trace主函数的编写，注意传参的第一个参数需要为整数mask，因此要用atoi函数转化。mask可以看作掩码，确认调用了哪些系统调用。经过调整参数位置后，调用exec函数来执行各个xv6中的命令，从而查看某命令底层调用了哪些syscall。
## sysinfo的编写
[提交记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/cdad6deb7aa004aeded48909db9ef2850e1d8913)

1. sysinfo的准备步骤与systrace相同，不再赘述。
2. 然后创建sysinfo结构体，包含freemem空闲内存的总量，用collect_fm函数获取，nproc进程总数，用collect_up函数获取。
3. 这部分说明一下系统调用函数命名看似无参是如何获取参数的：
- 主要有argint，argaddr，argfs等函数。获取复杂结构体的指针用argaddr即可。据此获得sysinfo结构体。然后调用collect_fm与collect_up函数获得结构体中成员的值。
4. collect_up函数遍历当前系统中状态为UNUSED的全部进程，获取运行的进程数。
5. collect_fm函数遍历内存中的空闲分区链表kmem.freelist，获取页面个数乘以PGSIZE页面大小即可获取空闲内存大小。

