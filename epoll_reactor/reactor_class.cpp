//反应堆简单版
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "wrap.h"
#include <iostream>
#include <unordered_map>
#include <fcntl.h>

const short _BUF_LEN_ = 1024;
//const short _EVENT_SIZE_ = 1024;
const int _ServerPort_ = 8000;
const int _EpollEvsSize_ = 1024;
const short _EpollWaitTimeout_ = -1;


//const int ServePort = 8000;
//const char *ServerIp = "192.168.2.249";


using namespace std;


class Rector{
private:
    int epoll_fd;
    int listen_fd;
    epoll_event ev, evs[_EpollEvsSize_];
    char buffer[_BUF_LEN_];
    unordered_map<int,string> map;
public:
    Rector()=default;
    ~Rector(){};

    void listen_bind();
    void  epoll_init();
    void initAccept(const int &);
    int readData(const int&  i);
    void senddata(const int & i) ;
    int Run() ;

};

//发送数据
void Rector::senddata(const int & i) {
    cout << "client " << map[evs[i].data.fd ]<< " data : " << buffer << endl;
    string str(buffer);
    str = "server data : " + str;
    Write(evs[i].data.fd, str.c_str(), str.size());
    // 没有使用buffer数据 所以需要memset()
    memset(buffer, 0, sizeof(buffer));
}

//读数据
int Rector::readData(const int&  i) {

    ssize_t n;
    // 如果读一个缓冲区 缓冲区没有数据 如果是带阻塞 就阻塞等待
    // 如果是非阻塞 返回值是-1 并将 erron 设置为 EINTR
    n = read(evs[i].data.fd, buffer, _BUF_LEN_);

    if (n == 0) {

        //对端关闭了连接，从 epoll_fd 上移除 client_fd
        cout << "client [ " << map[evs[i].data.fd] << " ] aborted connection" << endl;
        close(evs[i].data.fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);
        return n;
    }
}

void Rector::listen_bind(){
    listen_fd = tcp4bind(_ServerPort_, nullptr);
    // 监听套结字
    Listen(listen_fd, 128);
    //监听
    //创建epoll树根节点
    epoll_fd = epoll_create(_EpollEvsSize_);
    cout<< "epoll_fd ==="<< epoll_fd<<endl;
    if (epoll_fd == -1) {
        cout << "create epoll_fd error." << endl;
        close(listen_fd);
        exit(1);
    }
}

// epoll初始化
void  Rector::epoll_init( ){
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev)==-1){

        cerr << "epoll_ctl error" << endl;
        close(listen_fd);
        exit(1);
    }//上树

}


//新连接处理
void Rector::initAccept(const int &i ) {
    cout << "epoll_fd = "<<epoll_fd<<endl;

    sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int client_fd = Accept(evs[i].data.fd, (sockaddr *) &client_address, &client_len);


    // save client ip:port
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
    cout << "new client connect . . . and ip is " << ip << " port is : " <<
         ntohs(client_address.sin_port) << endl;
    map[client_fd] = string(ip) + ":" + to_string(ntohs(client_address.sin_port));
    memset(ip,0, sizeof(ip));



    // 设置cfd为非阻塞
    int flag = fcntl(client_fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, flag);


    // 将 client_fd  上树
    ev.data.fd = client_fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

}




int Rector::Run() {


    Rector::listen_bind();


    Rector::epoll_init();



    while (true) {
        int n_ready = epoll_wait(epoll_fd, evs, _EpollEvsSize_, _EpollWaitTimeout_);
        if (n_ready < 0) //调用epoll_wait失败
        {
            //被信号中断
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
            break;

        } else if (n_ready == 0) {
            //超时,继续
            continue;
        }
        else //调用epoll_wait成功,返回有事件发生的文件描述符的个数
        {
            for (ssize_t i = 0; i < n_ready; i++) {

                if (evs[i].data.fd == listen_fd && (evs[i].events & EPOLLIN)) {
                    Rector::initAccept(i);


                }else if (evs[i].events & EPOLLIN){
                    while (true){
                        int n=Rector::readData(i);
                        if (n<=0){
                            break;
                        }else{
                            Rector::senddata(i);
                        }

                    }
                }
            }
        }
    }

    //关闭监听文件描述符
    Close(listen_fd);

    return 0;
}



int main(){

    Rector go;
    go.Run();
    return 0;
}