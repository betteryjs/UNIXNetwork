### `libevent` 的使用

#### 创建`event_base`根节点

```c++

struct event_base *event_base_new(void);

```

返回值是event_base 的根节点

#### 释放`event_base`根节点

```c++

void event_base_free(struct event_base *);

```

### 循环监听

```c++

int event_base_loopexit(struct event_base *, const struct timeval *);

```


