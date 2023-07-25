# Lab: mmap (hard)
## 基本系统调用的配置
[提交记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/6f34eaa6db0029b15964510bc27ff1f963f567ca)

在此简单说明，对于一个系统调用sys_xxx，需要：

1. 在syscall.c或sysfile.c中添加函数sys_xxx的定义。
2. 在syscall.c中添加sys_xxx的外部声明，在函数指针数组syscalls中添加对应的函数指针。
3. 在syscall.h中添加对应的宏。
4. 在user.h中添加系统调用函数xxx，本实验对应mmap和munmap
5. 在usys.pl中添加对应函数的入口,entry("xxx")。

## 实现mmap以及对应的函数与数据结构
[提交记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/f06d1f08d44fb8cd3b4e416ed62326006f5f3439)

1. 首先需要设计VMA对应的数据结构，参考map函数的声明
```c
void *mmap(void *addr, size_t length, int prot, int flags,
           int fd, off_t offset);
```
在原有的参数中增加vma项是否使用，以及fd对应的文件vfile即可。

根据提示，VMA一个进程中只需有16个就好了，因此在proc结构中添加VMA数组vmas[16]。
2.  完成sys_mmap的编写：

- 根据syscall.c与sysfile.c中提供的获取参数的函数，获取对应的参数。
- 为了简单起见，offset可以假设为0（提示里已经说明），由于mmap第一个参数是NULL，
因此addr必须为0，长度length显然非负。
- 如果文件不可写，并且保护模式为PROT_WRITE，那么便不能共享内存。
- 对于当前的进程p，进程空间不能超越虚拟地址上限MAXVA，然后找到未使用的VMA项，一一赋值，
增加文件的引用计数，这样当文件关闭的时候文件不会消失，最终返回VMA项的地址。

 3. 当遇到页错误时，那么调用mmap_handler，该部分代码在usertrap中实现，与lazy实验相似。
 4. mmap_handler的编写，以下是mmap_handler的原型：
```c
int mmap_handler(int va, int cause);
```
- 在当前进程中找到这样一种VMA项，它虽然被标记为未使用，但是在该项的地址范围内包含虚拟地址va。
- 如果找不到，返回-1.
- 根据保护位的标记来确定页表项的权限，不管怎么样，该页表项一定是用户空间的。
- 如果该VMA项对应的文件发生缺页且无法写操作，那么返回-1.
- 分配物理地址pa，然后进行文件的读操作，读操作是原子的，需要先上锁，从对应的inode中读取数据，

如果无法读取，那么释放文件锁与分配的物理页，否则不需要释放分配的物理页。之后只需要将获取的pa与函数中
的va建立映射关系就可以了，建立失败释放相应物理空间即可。

## 实现munmap以及对应的函数与数据结构
[提交记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/730cc55a8f67835ea67bb3bd0ae8a8d23b301da4)

1. 首先实现munmap函数：
- 首先读取参数addr与length。
- 如果有VMA项的起始位置或结束位置匹配的话，那么该VMA项符合条件。
- 如果该项对应的页面是MAP_SHARED，那么应该写回文件系统。
- 接触该页面的潜在映射。
- 如果该VMA项的虚拟页面长度为0，那么关闭文件，将该项标记为未使用。
2. 在fork与exit中进行相应的修改：fork中子进程复制父进程的虚拟内存区域，exit中清除这些VMA项。
3. 由于该实验采用懒分配，因此仿照lazy实验，如果在uvmunmap与uvmcopy中遇到无效页，继续循环即可。
