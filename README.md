# Lab locks
## Memory Allocator(Moderate)
> The basic idea is to maintain a free list per CPU, each list with its own lock. Allocations and frees on different CPUs
> can run in parallel, because each CPU will operate on a different list. The main challenge will be to deal with the case in which
>  one CPU's free list is empty, but another CPU's list has free memory; in that case, the one CPU must "steal" part of the other CPU's
> free list.

- 从题目中可以看出，需要为每一个CPU分配一个自由内存块链表，因此kmem修改为NCPU个的匿名结构体。
- 接下来修改kfree与kinit，只需要利用循环为每个kmem进行之前的操作即可，注意kmem的锁名可以用
snprint()函数获得
```c
snprintf(lockname,sizeof(lockname),"kmem_%d",i);
```
值得注意的是，在kfree中需要获得当前的cpu的ID,根据提示，需要在此期间启用push_off与pop_off。

- 在kalloc中，也需要获得当前cpu的自由链表，因此也需要push_off与pop_off.如果没有自由的内存
块，那么就需要从别的cpu偷内存块，可以采用for循环遍历，如果获得的cpuid为当前cpuid，跳过
即可。每个循环中是是之前普通的kalloc操作，注意一下自由链表是对应cpu上的即可。注意锁的分配与释放，
一开始获得的是当前cpu对应的kmem锁，最后需要释放。而在for循环中获得的是bid号cpu的锁，因此在循环
体内释放即可。

[Memory Allocator中对代码的修改记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/94658b95e75c322657733086941aac96f3348d6e)

## Buffer cache (hard)

[Buffer Cache中对代码的修改记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/e65aee001dd7b91ad717b45606b7b75771ef7837)

