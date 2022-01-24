### API

man poll

```c++

 #include <poll.h>
       int poll(struct pollfd *fds, nfds_t nfds, 
               int timeout);
```

#### 功能

- 监听多个文件描述符号
- 参数
- fds 监听的数组的首地址
- nfds 数组有效元素的最大下标+1
- timeout 超时时间 -1 为一直监听

```c++

 struct pollfd {
               int   fd;         /* 需要监听的文件描述符 */
               short events;     /*  需要监听的文件描述符什么事件*/
               short revents;    /* returned events */
           };


```

#### events

- POLLIN 读事件
- POLLOUT 写事件
