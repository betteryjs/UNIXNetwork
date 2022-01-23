### epoll

#### 特点

- 没有文件描述符的限制
- 以后每次监听都不需要将监听的文件描述符拷贝到内核中
- 返回的是已经变化的文件描述符 ，不需要遍历树

### API

```c++
int epoll_create(int size);

// size 监听的文件描述符上限
// return 返回的句柄
```

```c++


#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

// epfd 树的句柄
// op
EPOLL_CTL_ADD // 上树
EPOLL_CTL_MOD // 下树
EPOLL_CTL_DEL //
        
```

```c++
struct epoll_event
{
uint32_t events;	// 需要监听的事件
epoll_data_t data;	// 需要监听的文件描述符
} 

```

### 两种触发方式

- 水平触发 缓冲区内有数据就触发
- 边沿触发
