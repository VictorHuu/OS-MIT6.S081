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
- 首先,根据提示设置数据结构：bcache中有一个桶数组，桶的大小是质数13，以尽量减少哈希冲突，每个hashbuf有
一个缓冲区与自旋锁。
```c
#define NBUCKET 13
#define HASH(id) (id % NBUCKET)
struct hashbuf {
  struct buf head;
  struct spinlock lock;
};
struct {
  struct buf buf[NBUF];
  struct hashbuf buckets[NBUCKET];
} bcache;
```
- 有了bcache与hashbuf之后，就需要修改代码的对应部分。
1. 首先是init，还是利用snprint为每个桶的锁命名，与上一个实验思路差不多。
2. 接下来是bpin与bunpin。块id可以通过HASH函数与块的块号blockno获得。锁变为对应桶的锁。
3. 还有bget，但是修改完对应部分后，还需要修改LRU的标准并序列化，这只是完成了部分工作。
- 将LRU的标准修改为按时间戳，每当对块进行写操作时，就需要获取时间戳，但获取过程必须是原子的。
```c
acquire(&tickslock);
      b->timestamp = ticks;
      release(&tickslock);
```
有3个地方需要用到这段代码，分别是：
1. brelse()中引用数递减后需要修改时间戳，释放过程移到bget函数中。
2. bget()中缓存命中时，需要增加引用数，因此也需要修改时间戳。
3. bget()回收最近最少使用的块时。
- bget()中如果缓存命中，与之前一样，除了缓存区是哈希分布的。
- bget(）如果不命中，那么就要回收缓冲块。过程如下：
按桶遍历，cycle是桶数，i是桶中的偏移，也就是缓存块的id：
1. id不能与设备dev对应的id相同，同时必须获取对应桶的锁。
之前已经获得了bcache的锁，两个锁的作用不同([详见讲义](https://pdos.csail.mit.edu/6.828/2020/lec/l-fs.txt))：
>Two levels of locking here
  > - bcache.lock protects the description of what's in the cache
  > - b->lock protects just the one buffer

简单地说，bcache的锁保护元信息，对应桶的锁保护缓存的详细信息。

2. 根据LRU的时间戳原则找到最早使用的缓存块b。
3. 如果b存在，那么在遵循第1条的前提下，将b块从链表中释放，之后释放桶对应的锁，再将设备dev
对应id的对应bcache添加到链表头，表示最近使用，同时修改其余描述信息，因此还要原子化的更新时间戳。
4. 如果没有找到到，释放对应桶的锁。最后循环结束后，不要忘记释放bcache的锁。

[Buffer Cache中对代码的修改记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/e65aee001dd7b91ad717b45606b7b75771ef7837)

## 通过提交截图
![image](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/assets/103842499/55e03a94-01a5-4731-bb2b-11414cf7b906)


