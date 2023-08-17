# Lab Thread
## Using threads (moderate)
- 这里需要修改thread_switch的汇编代码，与内核的swtch无太大差异。

汇编代码中只保存被调用函数的变量，因为调用函数的变量存储在自身的栈上。

thread_switch不存储PC，而存储RA的值，是调用该函数的地址，结束后要通过RA返回调用函数。

- 对应地，在uthread.c中创建用户上下文结构t_context，与内核的context没多大区别。
在每一个线程中，增加一个上下文结构，便于进程的上下文切换。
1. 在thread_schedule中调用thread_switch函数。
2. 在thread_create对线程初始化时，ra设置为调用该线程的func函数指针，
3. sp为栈指针，根据t->stack与栈大小可以确认。

[Using threads实验代码记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/4ddb2e8b9a5ea95b70c738dbd84c80634dde1613)
## Uthread: switching between threads (moderate)
-  通过利用Unix自带的pthread库，首先对线程数组全局初始化。
对于每一个对应桶的插入操作，由于可能会有多个线程对哈希表写操作，因此需要用对应的锁隔离
，维护原子性。

[Uthread实验代码记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/89e453dae05cea63ccf8e1bfbf8b26ae96865b85)

## Barrier(moderate)
- 根据提示提供的pthread API，可以看出本实验关于条件变量的等待、广播的机制。
- 由于要修改bstate，自然在barrier函数的开头结尾为获取与释放bstate的barrier互斥锁。
- 每当所有的线程都调用了barrier时，就清空nthread的计数，并增加一轮(round)，然后广播出去。
- 否则就等待，直到广播的到来。

[Barrier实验代码记录](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/05d651bd500d31bf9500382bd8278c03afec862a)

## 实验结果截图
![ScreenshotFortheLabThreads](https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/assets/103842499/baa2494c-ff8b-4cac-88b9-7efe047cfe43)
