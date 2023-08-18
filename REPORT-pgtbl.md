# Lab pgtbl 实验报告
## Print a page table (easy)
> - You can put vmprint() in kernel/vm.c.

> - Use the macros at the end of the file kernel/riscv.h.

这个指PTE2PA，将页表项转换为物理地址
> - The function freewalk may be inspirational.

freewalk的代码中尤其是错误提示中可以看出，一个页表对应的页表项可读可写可执行（将成为rwx），那么说明这个页表项不在最后一级页表中。
这里用depth来表示页表的深度，并根据页表来确定。
xv6中的页表是一个3级页表，每一个页表的由物理地址中国的9位来确定，一共27位，最后12位是偏移量（offset），一共39位。
```c
int depth=1;
void
vmprint(pagetable_t pagetable){
    if(depth==1)
    	printf("page table %p\n",pagetable);
    for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      if(depth==1)
      	printf("..");
      else if(depth==2)
      	printf(".. ..");
      printf("%d: pte %p pa %p\n",i,pte,child);
      depth++;
      vmprint((pagetable_t)child);
      depth--;
    } else if(pte & PTE_V){//the leaf page
      uint64 child = PTE2PA(pte);
      printf(".. .. ..%d: pte %p pa %p\n",i,pte,child);
    }
  }

}
```
- Define the prototype for vmprint in kernel/defs.h so that you can call it from exec.c.
```c
//vm.c
...
void		vmprint(pagetable_t pagetable);
```
- Use %p in your printf calls to print out full 64-bit hex PTEs and addresses as shown in the example.
## A kernel page table per process (hard)
还是先看提示，根据第一个提示
> - Add a field to struct proc for the process's kernel page table.

结构proc是一个表示进程的结构，pagetable已经表示了用户状态下的页表，添加一个kernelpt表示内核态下的页表。
```c
struct proc{
    pagetable_t pagetable;       // User page table
   pagetable_t kernelpt;		//Kernel page table per-process
};
```
> - A reasonable way to produce a kernel page table for a new process is to implement a modified version of kvminit that makes a new page table instead of modifying kernel_pagetable. You'll want to call this function from allocproc.

这里我新建了一个函数proc_kpt_init，几乎和kvminit相同，知识kvmmap改为了uvmmap，uvmmap也几乎与kvmmap几乎相同，
但是panic的错误信息不同，这个有助于在出现异常的时候区分是在内核态还是用户态。

另外，proc_ktp_init与uvmmap相比另外两个函数，都多了一个页表的参数，因为之前的两个函数都是对全局的kernel_pagetable
操作，而这两个是针对每一个进程的，同时proc_ktp_init用于返回初始化后的页表。
> Make sure that each process's kernel page table has a mapping for that process's kernel stack. In unmodified xv6, all the kernel stacks are set up in procinit. You will need to move some or all of this functionality to allocproc.

这是procinit函数的代码片段
```c
char *pa = kalloc();
      if(pa == 0)
        panic("kalloc");
      uint64 va = KSTACK((int) (p - proc));
      kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      p->kstack = va;
```
只需要先复制到allocproc函数里面，再修改kvmmap为uvmmap，并且添加对应的页表为新增参数。
注意，这里用kalloc函数获取了一份物理页以及对应的地址，在根据当前进程相对于cpu中进程基址的偏移量确定KSTACK的虚拟地址，
![image](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/103ff345-f15a-4e33-886b-40a5f218def2)
通过查找KSTACK的宏：
```sh
$ grep -rn KSTACK .
./memlayout.h:56:#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PGSIZE)
```
我发现，（（p）+1）说明每个KSTACK上方有保护页，*2*PGSIZE表示每个KSTACK相距两个页，那么每个KSTACK的大小也就是一个页。
> - Modify scheduler() to load the process's kernel page table into the core's satp register (see kvminithart for inspiration). Don't forget to call sfence_vma() after calling w_satp().
> - scheduler() should use kernel_pagetable when no process is running.

提示里面要求在w_satp()之后调用sfence_vma()，是为了清除陈旧的页表项记录。
```c
            // Load the kernal page table into the SATP
			proc_inithart(p->kernelpt);

			swtch(&c->context, &p->context);

			// Come back to the global kernel page table
			kvminithart();
```
在上下文交换之前，仍有进程p处于运行态，因此需要加载进程的内核页表到内存中，SATP是页表的基址寄存器，
由于```void proc_inithart(pagetable_t)```与kvminithart几乎一样，除了初始化的内核页表不一样之外，
因此不再展示proc_inithart
> Free a process's kernel page table in freeproc.

首先用uvunmap解除KSTACK中的va->pa的映射关系，然后将kernelpt与kstack都赋值为0即可。
```c
static void
freeproc(struct proc *p)
{
  ...
  if(p->kernelpt){
    uvmunmap(p->kernelpt, p->kstack, 1, 1);
    proc_freekernelpt(p->kernelpt);
  }
  p->kernelpt=0;
  p->kstack = 0;
  ...
}
```
> - You'll need a way to free a page table without also freeing the leaf physical memory pages.

只需要稍微修改一下freewalk中的对应条件，比如只要是有效的页表项，那么都需要清除，只要是目录页表项，那么就需要递归。
注意到freewalk的注释，在调用前需要清除全部的从页表项到物理页表指针的映射，否则会造成内存泄露！
清除映射关系
- 首先需要根据内容大小确定页面所占大小，
- 遍历虚拟页，根据虚拟页和walk函数获取PTE即页表项，
- 然后判断页表项是否有对应的内容并且是否有效来确定是否是有效的映射，
- 如果是，并且dofree为真的情况下，那么就可以根据PTE获取页的物理地址，从而删除对应的物理页。
而在本次任务中不清楚物理页我猜测是因为kernelpt只是用于辅助copyin与copyinstr获取目录态传来的参数，
只有在清除kernel_pagetable或者是进程的用户页表时才会真正清除页表与页的映射关系。
```c
void
proc_freekernelpt(pagetable_t kernelpt)
{
  // similar to the freewalk method
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = kernelpt[i];
    if(pte & PTE_V){
      kernelpt[i] = 0;
      if ((pte & (PTE_R|PTE_W|PTE_X)) == 0){
        uint64 child = PTE2PA(pte);
        proc_freekernelpt((pagetable_t)child);
      }
    }
  }
  kfree((void*)kernelpt);
}
```
## Simplify copyin/copyinstr (hard)
首先在vm.c中增加一个pagetable到kernelpt的转换函数。
该函数的原型如下
```void u2kvmcopy(pagetable_t pagetable, pagetable_t kernelpt, uint64 oldsz, uint64 newsz)```
其中oldsz与newsz表示需要映射的虚拟地址的区间。oldsz需要页向上对齐，采用PGGROUNDUP，是为了不破坏旧有的虚拟地址，
for循环的终止条件其实就是newsz的向下页对齐。
对于每一个页，根据页基址i获取旧有的pagetable页表项pte_from，分配并获取kernelpt的页表项pte_from，
再用PTE2PA获得旧页表项的物理地址pa。根据提示
> What permissions do the PTEs for user addresses need in a process's kernel page table? (A page with PTE_U set cannot be accessed in kernel mode.)

PTE_U按位取反，然后与原有的权限位flags按位求和得到新的权限位，再与旧有的页表项按位求或即可得到新的页表项值（pte_from解引用后）。

第二步，将copyin与copyinstr调用新的copyinste_new,copyin_new，还要在defs.h添加新的声明。

第三步，将procinit，exec，sbrk中的growproc，fork中添加u2kvmcopy生成每个进程对应的内存页表。注意growproc会增长用户空间，
但不能越过PLIC的位置，因此需要在growproc中增加
```c
if (PGROUNDUP(sz + n) >= PLIC){
      return -1;
    }
```

