/*
  This example program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>

#ifndef _WIN32

#include <netinet/in.h>

#ifdef _XOPEN_SOURCE_EXTENDED

#include <arpa/inet.h>

#endif

#include <sys/socket.h>

#endif

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>


using namespace std;


static const char MESSAGE[] = "Hello, World!\n";
static const int PORT = 8000;


static void listener_cb(struct evconnlistener *, evutil_socket_t,
                        struct sockaddr *, int socklen, void *);

static void conn_writecb(struct bufferevent *, void *);

static void conn_eventcb(struct bufferevent *, short, void *);

static void signal_cb(evutil_socket_t, short, void *);

static void conn_read_cb(struct bufferevent *, void *);

int main(int argc, char **argv) {
    struct evconnlistener *listener;
    struct event *signal_event;

#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif

    event_base *base = event_base_new();
    if (!base) {
        cerr << "Could not initialize libevent!" << endl;
        return 1;
    }

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    // 创建链接监听器

    listener = evconnlistener_new_bind(base, listener_cb, (void *) base,
                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                       (sockaddr *) &sin,
                                       sizeof(sin));

    if (!listener) {
        cerr << "Could not create a listener!" << endl;
        return 1;
    }


    // 创建信号触发的节点

    signal_event = evsignal_new(base, SIGINT, signal_cb, (void *) base);// SIGINT == ctrl + c

    // 将信号的节点上树

    if (!signal_event || event_add(signal_event, nullptr) < 0) {
        cerr << "Could not create/add a signal event!" << endl;
        return 1;
    }

    // 循环监听

    event_base_dispatch(base);

    // 释放链接监听器

    evconnlistener_free(listener);
    // 释放信号节点
    event_free(signal_event);

    // 释放base节点

    event_base_free(base);

    printf("done\n");
    return 0;
}

static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
            struct sockaddr *sa, int socklen, void *user_data) {
    event_base *base = reinterpret_cast<event_base *>(user_data);
    // 将fd上树
    // 新建一个 buffer_event 节点
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        cerr << "Error constructing bufferevent!" << endl;

        event_base_loopbreak(base);
        return;
    }
    // 设置回调
    bufferevent_setcb(bev, conn_read_cb, conn_writecb, conn_eventcb, nullptr);
    // 设置写事件使能
    bufferevent_enable(bev, EV_WRITE | EV_READ);
    // 设置读事件非使能
    // bufferevent_disable(bev, EV_READ);

    bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
}


static void conn_read_cb(struct bufferevent *bev, void *user_data) {
    char buff[1500] = "";

    size_t n = bufferevent_read(bev, buff, sizeof(buff));
    cout << "client data : " << buff << endl;
    bufferevent_write(bev, buff, sizeof(buff));
}

static void
conn_writecb(struct bufferevent *bev, void *user_data) {

    // 获取缓冲区类型
    struct evbuffer *output = bufferevent_get_output(bev);
    if (evbuffer_get_length(output) == 0) {
        //		printf("flushed answer\n");
        //		// 释放节点 自动关闭
        //        bufferevent_free(bev);
    }
}


// 断开链接产生事件回调
static void conn_eventcb(struct bufferevent *bev, short events, void *user_data) {
    if (events & BEV_EVENT_EOF) {
        printf("Connection closed.\n");
    } else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: %s\n",
               strerror(errno)); /*XXX win32*/
    }
    /* None of the other events can happen here, since we haven't enabled
     * timeouts */
    bufferevent_free(bev);
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data) {
    event_base *base = reinterpret_cast<event_base *>(user_data);
    timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

    // 退出循环监听

    event_base_loopexit(base, &delay);
}
