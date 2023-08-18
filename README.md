# Lab fs
## Large files
本实验修改文件块的存储结构让xv6能存储65803个块。

- xv6原本一个inode节点能存储12个直接块和1个间接块，一个间接块包含256个块。共计12+256=268个块。
如果拿出两个块，1个块存储单间接块，有256个块，一个块存储双间接块，有65536个块，共计12+256+256*256=65803个块。
仿照之前的代码多加一层循环就可以完成实验。
1. 首先在fs.h中修改宏定义，NDIRECT直接块有11个，总共有NDIRECT+2个块。相应地，最大文件个数为NDIRECT+NINDIRECT+NINDIRECT*NINDIRECT
2. 对应地，修改itrunc函数
```c
struct buf *bp1;
  uint *a1;
  if(ip->addrs[NDIRECT+1]){
  	bp=bread(ip->dev,ip->addrs[NDIRECT+1]);
	a=(uint*)bp->data;
	for(i=0;i<NINDIRECT;i++){
		if(a[i]){
			bp1=bread(ip->dev,a[i]);
			a1=(uint*)bp1->data;
			for(j=0;j<NINDIRECT;j++){
				if(a1[j]){
					bfree(ip->dev,a1[j]);
				}
			}
			brelse(bp1);
			bfree(ip->dev,a[i]);
		}
	}
	brelse(bp);
	bfree(ip->dev,ip->addrs[NDIRECT+1]);
	ip->addrs[NINDIRECT+1]=0;
  }
```
相比于单间接块，双间接块只是多了一层循环而已。在获取第一层间接块后，然后获取对应第二层的起始位置，然后用同样的方法获取第二层间接块。

3. bmap函数同理
```c
 bn-=NINDIRECT;
  if(bn < NINDIRECT*NINDIRECT){
  	//Load doubly-indirect block,allocating if necessary.
	if((addr=ip->addrs[NDIRECT+1])==0)
		ip->addrs[NDIRECT+1]=addr=balloc(ip->dev);
	bp=bread(ip->dev,addr);
	a=(uint*)bp->data;
	if((addr=a[bn/NINDIRECT])==0){
		a[bn/NINDIRECT]=addr=balloc(ip->dev);
		log_write(bp);
	}
	brelse(bp);
	bp=bread(ip->dev,addr);
	a=(uint*)bp->data;
	if((addr=a[bn%NINDIRECT])==0){
		a[bn%NINDIRECT]=addr=balloc(ip->dev);
		log_write(bp);
	}
	brelse(bp);
	return addr;  
  }
```
## Symbolic links
(提交记录)[https://github.com/VictorHuu/ClassDesign-MIT6.S081Fork/commit/5dfa7ea433bc0349ab8c0a060770e86d8d095c81]

- sys_symlink 系统调用实现：

1. 首先，从用户态获取两个参数，即新的符号链接路径 new 和目标文件路径 old。
2. 调用 begin_op() 开始文件系统操作，为了确保操作的原子性。
3. 调用 create() 函数创建一个新的 inode，表示符号链接。这里传递的文件类型参数为 T_SYMLINK，并且权限位设置为 0，因为符号链接不需要实际的权限控制。
4. 如果 create 失败，解锁 inode，结束文件系统操作并返回错误。
5. 使用 writei() 函数将目标文件路径 new 写入刚刚创建的 inode 的数据块中。这样，符号链接就存储了目标文件的路径信息。
6. 如果写入不成功（写入的字节数小于 MAXPATH），则解锁 inode，结束文件系统操作并返回错误。
7. 解锁 inode，结束文件系统操作并返回成功（0）。

- 在 sys_link 系统调用中的修改部分：

1. 如果 ip（表示被链接文件的 inode）的类型是 T_SYMLINK，并且打开模式 omode 不包含 O_NOFOLLOW 标志，则进行以下操作：
2. 使用一个循环，最多尝试 10 次，来递归地跟随符号链接。每次循环，都读取 inode 数据块中的路径信息到 path 变量中。
3. 如果读取的字节数不等于 MAXPATH，则解锁 inode，结束文件系统操作并返回错误。
4. 解锁 inode，并使用 namei() 函数根据路径 path 获取新的 inode。如果获取失败，结束文件系统操作并返回错误。
5. 锁定新的 inode，检查其类型。如果新的 inode 的类型不是 T_SYMLINK，则退出循环。
6. 如果新的 inode 仍然是符号链接类型，继续循环，进行下一次链接跟随。
7. 如果循环结束后，新的 inode 不再是符号链接类型，则继续执行后续操作。
8. 如果循环结束时，新的 inode 仍然是符号链接类型，则解锁新的 inode，结束文件系统操作并返回错误。
