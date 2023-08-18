# Lab: Copy-on-Write Fork for xv6
## Implement copy-on write
- 首先修改uvmcopy的代码，如果获得的页表项是可写的，那么就要撤销可写位，增加COW位PTE_COW,
对于原先获得的旧PTE也是如此。接着就是把mappages中要映射的物理地址变为旧有的pa，增加引用数即可。
```c
 if(flags & PTE_W) {
      flags = (flags | PTE_COW) & ~PTE_W;
      *pte = PA2PTE(pa) | flags;
    }

    if(mappages(new, i, PGSIZE, pa, flags) != 0){
      goto err;
    }
    kaddrefcnt((char*)pa);
```
如果不修改可写页的权限，那么会发生r_scause为2的错误，表示是一个非法指令(可能覆写了程序的指令部分)。

我们希望存储页错误页，可以增加一个PTE_COW位，撤销PTE_W位，当对应的页需要写的时候再分配真正的物理页。

- 既然要增加引用数，那么就要增加一个结构，包含引用数，并用锁来维护。物理地址的范围是PHYSTOP，页大小
为PGSIZE，因此共有PHYSTOP/PGSIZE个物理页面。
```c
struct{
  struct spinlock lock;
  int cnt[PHYSTOP/PGSIZE];
}ref;
```
- 在usertrap函数中，if条件中增加scause为13或15，表示页错误，那么就要首先判断是否是cow页，是否可以分配。
cow页用cowpage判断，如下：
```c
//Judge whether the very page is a cow page.
int cowpage(pagetable_t pagetable, uint64 va) {
  if(va >= MAXVA)
    return -1;
  pte_t* pte = walk(pagetable, va, 0);
  if(pte == 0)
    return -1;
  if((*pte & PTE_V) == 0)
    return -1;
  return (*pte & PTE_COW ? 0 : -1);
}
```
该页的虚拟地址必须合法，对应页表项存在，有效，是cow页。

- 接下来考虑一下哪些函数可能需要对页引用数操作，包括kalloc,kfree以及cowalloc。
由于可能会有多个函数对引用数操作，因此需要用锁ref.lock维护原子性。

1. 在kfree中，首先获取ref.lock，然后进行递减操作，递减操作一旦完成，
立即释放ref.lock,如果引用数为0，那么执行原来的free操作
，在freelist上增加被释放的物理页（同样是原子操作）。
2. 在kalloc中，如果存在符合条件的自由页，那么就进行原子操作，将引用数加1.
3. 在cowalloc中，如果对应的物理地址引用数增加后只有1份，那么就复原标志位即可。
如果没有对应的物理地址，那么和原先的uvmcopy操作差不多，也是需要找到一个合法的pa，
然后恢复标志位，进行va->pa的映射，最后还可能回收原先的物理页。
```c
void* cowalloc(pagetable_t pagetable, uint64 va) {
  uint flags;
  if(va % PGSIZE != 0)
    return 0;

  uint64 pa = walkaddr(pagetable, va);  
  if(pa == 0)
    return 0;

  pte_t* pte = walk(pagetable, va, 0); 

  if(krefcnt((char*)pa) == 1) {//Only 1 reference
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
    return (void*)pa;
  } else {//late map a real page for the PTE
    char* mem = kalloc();
    if(mem == 0)
      return 0;

    memmove(mem, (char*)pa, PGSIZE);

    *pte &= ~PTE_V;//avoid remap error
    flags=PTE_FLAGS(*pte);

    flags=flags|PTE_W;
    flags=flags&~PTE_COW;
    if(mappages(pagetable, va, PGSIZE, (uint64)mem, flags) != 0) {
      kfree(mem);
      *pte |= PTE_V;//pair with the above PTE_V disabling.
      return 0;
    }


    kfree((char*)PGROUNDDOWN(pa));
    return mem;
  }
}
```
- 以上改动只能使部分测试通过，file测试不通过。通过查看对应的代码，发现文件操作filetest()涉及到
的read系统调用异常，返回-1.
接着查看sys_read(),发现fileread的返回值是4！查看之前的argfd函数（读取文件描述符），
```c
if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
```
发现可能是ofile[fd]出错，那么就是在创建文件描述符的时候出错，也就是在调用pipe时。
pipe当中会调用copyout，copyout中发生了cow页错误。
```c
if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
     copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0)
```
如果是COW页，那么就用cowalloc分配物理地址，否则用walkaddr即可。
最后，usertests全部通过。
![57573b0c821a13880b2a2f742e5d8b04](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/13f9cefa-ece2-4f3c-8285-2534a308e9ce)

