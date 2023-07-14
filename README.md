# Lab Lazy
## Eliminate from sbrk()(easy)
```sh
git checkout 9220c97
```
进入sysproc.c后，即可看到以下代码：
```c
uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  myproc()->sz+=n;
  return addr;
}
```

![image](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/1b523238-fc9e-4cb6-b406-4c30fb6d5ee3)

通过在usertests.c的汇编语言中查找，发现是reparent函数在传送sp的s1时发生了trap缺页中断，reparent接受一个结构proc
，proc的上下文context用于进程的上下文切换，其中包含s1，因此可以大致判断发生在上下文切换时。
![29bc05821b6c57bdeabf5485155b6243](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/bc6047ca-561f-4038-a42c-fab3436de916)

## Lazy allocation (moderate)
- 根据提示一，可以在usertrap函数中增加r_scause()==15||r_scause()==13条件
- 根据提示2，获得虚拟地址uint64 va=r_stval();
- 物理地址pa可以先用kalloc分配，然后再确定是否分配成功：
  1. 如果分配成功，那么就将对应的页的内容全面置0
  2. PGROUNDDOWN将va的地址页对齐
  3. 用mappages确定va->pa的映射关系，并且具有读写用户权限
  4. 如果没有分配成功，那么就要释放pa对应的物理页，并结束当前进程。
- 没有分配成功，就直接结束当前进程。
```c
void usertrap(){
    ...
else if(r_scause()==15||r_scause()==13){
  	uint64 va=r_stval();
  	printf("page fault:%p\n",va);
  	uint64 pa=(uint64)kalloc();
  	if(pa==0){
  		p->killed=1;
  	}else{
  		memset((void*)pa,0,PGSIZE);
  		va=PGROUNDDOWN(va);
  		if(mappages(p->pagetable,va,PGSIZE,pa,PTE_W|PTE_U|PTE_R)!=0){
  			kfree((void*)pa);
  			p->killed=1;
  		}

  	}

  } 
}
```
在uvunmap中，如果未成功分配页表项或者页表项无效，那么就要继续执行，因为有可能是因为延迟分配引起的。
```c
if((*pte & PTE_V) == 0){
      continue;
     }
```
![ff5162a6e3ac32269ef12ac11e66e522](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/709ed29c-ac3d-4134-b930-e2feeb1df9c8)

## Lazytests and Usertests (moderate)
这个实验主要是对之前代码的复用并注意一些细节：

- 如果sbrk参数为负，那么就要用dealloc函数取消分配，因为这是growproc参数为负的情况，但是返回值仍然是未取消分配前的进程的sz大小。
- 进一步精确缺页是因为懒分配的范围，在栈顶之上，堆中的sp之下，即
```c
if(PGROUNDUP(p->trapframe->sp)-1<*ip&&p->sz>*ip)
```
- 如果物理页表未分配成功，也需要结束当前进程
- usertrap中处理缺页中断的程序如下

 ![image](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/26bd9940-0d38-4013-b9fb-c38df0bdf8a9)
 
- 在读写函数进行系统调用时，需要用到argaddr函数来传递指针，如果指针对应的虚拟地址未分配，那么同样需要进行中断处理，这里只需要将va
设置为argraw获取的指针解引用后的地址即可。

![f91a3047c7a88fa8c07ec6feb7c67430](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/fe4da10f-3240-4837-9808-743ad72b0cd2)

![7a737f36692b7dc1194d6e5d1315a633](https://github.com/VictorHuu/XV6LabTJ/assets/103842499/d825fdcf-9149-4e69-af13-0e1691cc3b61)

