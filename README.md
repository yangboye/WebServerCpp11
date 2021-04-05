# Webserver

## 0. 磨刀不误砍柴工
### 0.0 Google Test

参考：[GoogleTestDemo](https://github.com/yangboye/GoogleTestDemo)



## 1. 定时器

> 本节代码对应`src/timer`。



### 1.0 补充

1. Linux网络程序需要处理的三个事件：I/O事件、信号、定时器。

2. Linux提供的三种定时方法：

   （1）socket选项: `SO_RCVTIMEO`和`SO_SNDTIMEO`（根据系统调用的返回值及`errno`来判断）;

   （2）`SIGALRM`信号：`alarm`和`setitimer`函数；

   （3）I/O复用系统调用的超时参数。

3. 实现方式：

   （1）基于升序链表的定时器；

   （2）时间轮（循环数组 + 双向链表）；

   （3）时间堆（最小堆，可用数组作为底层存储结构）。

4. 本程序采用时间堆，将所有定时器中超时时间最小的一个超时值作为心搏间隔。



### 1.1 问题

- 底层存储结构是什么？

  底层存储结构为`std::vector`，通过树的特性：节点`i`的子节点为`2*i+1`和`2*i+2`，来将其转化成树结构。



- 上滤操作`SiftUp_()`的工作机制？

  函数`SiftUp_(size_t i)`是对第`i`个节点进行向上调整（也即所谓的上滤操作），其终止条件是父节点的生效时间比子节点小。



- 下滤操作`SiftDown_()`的工作机制？

  函数`SiftDown_(size_t index, size_t n)`是对第`i`个节点进行向下调整（也即所谓的下滤操作），其中`n`通常为堆中总的节点个数，终止条件是父节点的生效时间比子节点小。返回类型为`bool`, 当返回`true`时，说明节点`i`执行下滤成功；当返回`false`时，说明节点`i`没有执行下滤，可能需要上滤操作，因此如果要调整一个节点，一般先尝试对其执行下滤操作，如果下滤失败，再执行上滤操作，这样就能保证该节点被正确调整。用法如下：

  ```c++
  // 先执行下滤，如果下滤失败，则再执行上滤(代码摘自`HeapTimer::Add()`函数)
  if (!SiftDown_(i, heap_.size())) {
      SiftUp_(i);
  }
  ```

  

- 删除操作`Del_()`的工作机制？

  函数`Del_(size_t i)`是对第`i`个节点进行删除。其删除步骤是：先将要删除的节点与堆尾节点交换（这样，第`i`个节点就放到堆尾了）；然后对刚刚那个换到第`i`个位置的节点做上滤、下滤调整；最后删除堆尾的那个元素即可。



- 添加操作`Add()`的工作机制？

  函数`Add(int id, int timeout, const TimeoutCallBack& cb)`是向堆中添加一个节点。如果该节点是新的节点（通过文件描述符`id`判断），其添加步骤是：先将要添加的节点放到堆尾，然后再执行上滤操作；如果该节点是已存在的节点，其步骤是：先修改对应节点的生效时间，然后再执行下滤、上滤操作。



## 2. 缓冲区

> 本节相关代码对应`src/buffer`。



### 2.0 补充

注意，该实现中与muduo中的实现有区别：本处没有prependable bytes。

$$ 0 <= read_{pos} <= write_{pos} <= buffer.size() \tag{1}$$

$$ readable = write_{pos} - read_{pos} \tag{2} $$

$$ writable = buffer.size() - write_{pos} \tag{3}$$



### 2.1 问题

- 为什么non-bolcking网络编程中应用层buffer是必须的?
    TODO

- 为什么`Buffer`中要用两个`ssize_t`类型的成员函数（`read_pos_`和`write_pos_`）来标位置，而不用迭代器？

  ​		目的是应对迭代器失效。当`buffer_`长度不够时，`MakeSpace_`中会调用`buffer_.resize()`扩容，这时，由于vector重新分配了内存，原来指向其元素的指针会失效（查看STL vector的resize机制）。因此，使用整数下标而不是指针（迭代器）。



- `MakeSpace_`函数工作机制？

  （1）首先判断`buffer_`内部剩余的空间（除了`write_pos_ - read_pos_`之间的空间）能否放下新的数据，如果不能放下，则对`buffer_`扩容；

  （2）如果能够放下，则将`write_pos_ - read_pos_`之间的数据移动到最前面，然后将新数据添加到`buffer_`尾部。

  

- `RetrieveAll`函数工作机制？

  获取全部数据之后，对`buffer_`清空，`read_pos_`和`write_pos`清零。

  

- `ReadFd`函数工作机制？

  ​		在栈上准备一个64KB的`extrabuf`（对应代码中的`buff`），然后利用`readv()`来读取数据，`iovec`有两块（iov[0]指向`buffer_`，iov[1]指向`extrabuf`），如果数据不多，那么全部都读到`buffer_`中；如果数据较多，先放满`buffer_`，然后将剩余的数据放到`extrabuf`中，最后用`Append()`函数将`extrabuf`中的数据添加到`buffer_`中。

  

- `size_t`与`ssize_t`

1. 类型区别：
```c++
// 在32位系统中
using size_t = unsigned int; 
using ssize_t = int; 

// 在64位系统中
using size_t = unsigned long; 
using ssize_t = long int; 
```
2. 作用区别：
>(1) size_t一般用来表示一种**计数**，比如有多少东西被拷贝等。例如：sizeof操作符的结果类型是size_t，该类型保证能容纳实现所建立的最大对象的字节大小。 它的意义大致是“适于计量内存中可容纳的数据项目个数的无符号整数类型”。所以，它在数组下标和内存管理函数之类的地方广泛使用。
>(2) ssize_t这个数据类型用来表示可以被执行读写操作的**数据块的大小**。它和size_t类似,但必需是signed，意即：它表示的是signed size_t类型的。

参考：[Size_t和int区别](https://blog.csdn.net/wc11223/article/details/70553583)



- `std::string`中的`data()`和`c_str()`的区别？

  `c_str()`会在数据的末尾添加`\0`结束符，多数用于使用字符串的场合；`data()`不会添加`\0`结束符。



## 3. 线程池

> 本节代码对应`src/pool`。



### 3.1 问题

- 析构函数为什么设置为私有(private)？



## 4. 日志

> 本节代码对应`src/log`。



### 4.0 补充

#### 4.0.0 C/C++时间函数

常用的是获取当前时间，用法如下：

```c++
time_t timer = time(nullptr);
struct tm* sys_time = localtime(&timer);
// tm结构体如下
// struct tm
// {
//    int tm_sec;  /*秒，正常范围0-59， 但允许至61*/
//    int tm_min;  /*分钟，0-59*/
//    int tm_hour; /*小时， 0-23*/
//    int tm_mday; /*日，即一个月中的第几天，1-31*/
//    int tm_mon;  /*月， 从一月算起，0-11*/  1+p->tm_mon;
//    int tm_year;  /*年， 从1900至今已经多少年*/  1900＋ p->tm_year;
//    int tm_wday; /*星期，一周中的第几天， 从星期日算起，0-6*/
//    int tm_yday; /*从今年1月1日到目前的天数，范围0-365*/
//   int tm_isdst; /*日光节约时间的旗标*/
//};
```

参考：[c++ 时间类型详解 time_t](https://www.runoob.com/w3cnote/cpp-time_t.html)

获取更精确的时间使用`int gettimeofday(struct timeval *tv, struct timezone *tz);`，用法如下：

```c++
struct timeval now = {0, 0};
gettimeofday(&now, nullptr);
time_t t_sec = now.tv_sec;
struct tm* sys_time = localtime(&t_sec);
```



#### 4.0.1 `va_list`及相关函数

- `int snprintf ( char * s, size_t n, const char * format, ... );`

  将格式化的字符串存储在`s`中，且长度不大于`n-1`，大于`n-1`的部分将被丢弃（但该部分也会计算到返回结果中）。返回值要特别注意：如果`返回值∈[0, n)`时（注意n没有被取到），字符串完全写入；如果`返回值>=n`，部分写入；如果`返回值<0`，编码错误。

  ```c++
  char buff[N] = {0};
  snprintf(buff, sizeof(buff), "your format", data...)
  ```

  

- `int sprintf ( char * str, const char * format, ... );`

  和上面的类似，但是该函数不太安全（如果格式化的字符串比`str`长的话程序会crash），因此，推荐使用`snprintf()`;

- `int vsnprintf (char * s, size_t n, const char * format, va_list arg );`

  常用用法：

  ```c++
  #include <stdio.h>
  #include <stdarg.h> // va_list va_start va_end
  
  void Print(const char* format, ...) {
      char buff[256];
      va_list args;
      va_start(args, format);
      vsnprintf(buffer, sizeof(buff), format, args);
      va_end(args);
  }
  ```

  



#### 4.0.2 文件操作



#### 4.0.3 `lock_guard`和`unique_lock`

- `lock_guard`: 

  - 创建即加锁，作用域结束自动析构并解锁，无需手工解锁；
  - 不能中途解锁，必须等作用域结束才解锁；
  - 不能复制。

- `unique_lock`:

  - 创建时可以不锁定（通过指定第二个参数为`std::defer_lock`），而在需要时再锁定；
  - 可以随时加锁解锁；
  - 作用域规则同`lock_guard`，析构时自动释放锁；
  - 不可复制，可移动；
  - 条件变量需要该类的锁作为参数（此时必须使用`unique_lock`）。

  > 需要使用锁的时候，首先考虑`lock_guard`。

参考: [c++11中的lock_guard和unique_lock使用浅析](https://blog.csdn.net/guotianqing/article/details/104002449)





### 4.1 同步队列

> 对应代码`src/log/block_queue.h`

`BlockQueue<T>`使用了**生产者-消费者**模式。竞态资源为`deq_`。



### 4.2 日志类

> 对应代码`src/log/log.h`和`src/log/log.cpp`。

`Log`类使用了**单例模式**。判断是否同步/异步日志：当`init()`中的`maxQueueSize`为0时，同步日志；>0时异步日志。



### 4.3 问题

- 在头文件(`.h`)中声明为`const char*`的变量，在`.cpp`文件中何时赋值？

  比如`Log`类中的实例成员变量`path_`和`suffix_`。



## 5. Http请求和响应处理

> 对应代码`src/http`



### 5.0 补充

#### 5.0.0 Http



#### 5.0.1 MySQL数据库操作

