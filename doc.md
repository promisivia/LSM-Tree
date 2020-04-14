# LSM

 LSMtree是由O’Neiletal在1995年提出的结构，主要针对在大量写入的场景下，B-Tree的写入带来额外的I/O开销的问题，支持了更高效的数据更新、写入和删除。LSM核心思想就是将写入推迟(Defer)并转换为批量(Batch)写，一次I/O可以进行多条数据的写入，充分利用每一次I/O。

## 1. 设计

LSM树（Log-Structured-Merge-Tree）的结构是横跨内存和磁盘的，包含Memtable、SSTable等多个部分。LSM树会将所有的数据插入、修改、删除等操作保存在内存之中，当此类操作达到一定的数据量后，再批量地写入到磁盘当中。

### 	1.1. MemTable

Memtable可以使用跳跃表或者搜索树等数据结构来组织数据以保持数据的有序性，在本项目中使用了SkipList来储存，生长概率设为了常见的0.5。

![image-20200407212025893](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200407212025893.png)

PUT、GET等操作都首先在MemTable中操作，当MemTable达到一定的数据量后（在本项目中设定为2M，使用了一个变量来记录当前大小），MemTable会转化成为一个SSTable，储存相应文件到磁盘。

### 	1.2. SSTable

SSTable(Sorted String Table)为有序键值对集合，是LSM树存入磁盘中的文件的结构。

![image-20200407213401905](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200407213401905.png)

SSTable文件的基本结构如上图所示，包括数据区、索引区，这里添加了一个divide变量方便从磁盘的SSTable文件加载索引区。

- **数据区：**SSTable的数据部分大小可能较大，同时保存了数据的key和value值（当然其实可以不存储key来节省一定的空间）

- **索引区：**索引文件中保存数据与 *该数据在数据文件中偏移* 的映射关系。索引部分的大小比较小，只包括了键和偏移量offset，而不包括值value。因此从将SSTable缓存在内存中不会占用过多的内存

###     1.3. Disk

![disk](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200407214445001.png)

​    SSTable文件在磁盘中分层存储，每层最大文件数由预设的比例决定。同时随着层数的增加，规定一个文件个数的上限。本项目中使用的参数：比例为2，第零层最大个数为2，文件数最多不超过128。

### 	1.4 Compaction and Merge

由于硬盘上的数据文件一旦创建就不可以再更改，所以一段时间之后就有可能产生大量的数据文件，数据文件过多会影响查询速度，所以定期需要合并文件。每当文件数量达到了一定阈值就会触发compaction操作。

Comopaction的基本操作包括：

- 在本层选择文件：

  第一层文件的key不有序所以必须要全部选择。对于其余各层，为了保证文件数量操作后要小于最大值，需要选择所有多出来的文件，考虑到LSM设计的目的之一想要让最近操作的文件更容易被访问到，简单的选取靠前放入的文件。

- 去下层选取有重合的文件，进行归并排序并全部存入下一行。

- 继续检查下一层是否需要Compaction

Compaction是最耗时的操作，为了提升性能，简单做了如下的优化

- 除了第一层一定要进行Merge操作，其他各层如果在下一层没有选到文件可以直接移入下一层。

- 如果第一层没有选到文件，检查第一层的文件是否已经有序，有序的话也不用再进行Merge操作。

- 当需要Merge的文件个数达到一个上限（极端情况几十个文件overlap了）的时候，简单的在当前层和下一层之间添加新的一个空层，改变下面所有文件的文件名和最大可存储数，将当前层选到的文件（不包括下一层选上的）移入新开辟的一层。

  > 注：该条件需要对第一层和第二层文件归并失效，以防直接把第一层文件移入下一层，事实上，上限设置的较大时必然不会出现第一第二层加起来已经超过该上限。

###     1.5 Cache

在内存中建立一个LevelList结构来记录每层文件的基本信息，包括文件名，maxKey，minKey，divide和文件的缓存，当发现没有时从磁盘中重新加载。



## 2. 测试

测试环境：windows vs2017 x64模式

### 	2.1 Latency

1. 当没有涉及到文件操作

   测试数据: 1-1024，random string

   ![image-20200408090831528](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200408090831528.png)

2. 涉及到文件操作但没有compaction

   测试数据: 1-1024*5，random string

   ![image-20200407232548621](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200407232548621.png)
   
   当没有涉及到磁盘操作时，所需要的平均时间很少，push>delete>get,  当涉及到磁盘操作时，所需要的平均时间很少，push远小于delete和get, 可见LSM确实支持更高效的数据更新和写入。

### 		2.2 Throughput

测试数据: 0 - 1024*100 string的长度固定

测试方法: 每PUT 100次统计所用的时间，计算每秒的平均操作数，带来一定误差。

![image-20200407234845028](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200407234845028.png)

![image-20200414133136494](C:\Users\olivia\AppData\Roaming\Typora\typora-user-images\image-20200414133136494.png)

整体吞吐量较为稳定，开始最大。

在最开始几乎没有merge操作时吞吐量非常大；在其余的大部分区域，merge次数增多对应吞吐量减少；还有一些吞吐量和merge操作同时剧减的地方，可能是系统调度的问题。