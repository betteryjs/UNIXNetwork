### select 函数API

```c
// man select
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
int select(int nfds, fd_set *readfds, fd_set *writefds,
          fd_set *exceptfds, struct timeval *timeout);
void FD_CLR(int fd, fd_set *set);
int  FD_ISSET(int fd, fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);
```

`select`参数

- `nfds` : 最大文件描述符号+1
- `readfds` ： 需要监听的读的文件描述符存放路径
- `writefds` ： 需要监听的写的文件描述符存放路径
- `exceptfds` : 需要监听的异常的文件描述符存放路径
- `timeout` : 多长时间监听一次 固定的时间 限时等待 null 一直监听

```c

 struct timeval {
               long    tv_sec;         /* seconds */
               long    tv_usec;        /* microseconds */
           };
```

#### 返回值

> 返回的是变化的文件描述符的个数

### 问题

- 文件描述符 1024的限制
- 大量并发 少了活跃 select 效率低下