//
// Created by yjs on 2022/1/6.
//

#include "wrap.h"
#include <iostream>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>


// static data
static const short port = 8000;


using namespace std;

int main(int argc, char **argv) {

    // create socket and bind
    int lfd = tcp4bind(port, "0.0.0.0");

    // listen
    Listen(lfd, 128);

    // while
    int maxfd = lfd;// 最大的文件描述符

    fd_set old_set, r_set;
    FD_ZERO(&old_set);
    FD_ZERO(&r_set);
    // 将lfd添加到old_set中
    FD_SET(lfd, &old_set);

    while (true) {
        r_set = old_set;//将老的old_set 赋值给需要监听的集合中
        // select listen
        //        unordered_map<int ,string> map;
        int n = select(maxfd + 1, &r_set, nullptr, nullptr, nullptr);
        if (n < 0) {
            perror("select error");
            break;
        } else if (n == 0) {
            continue;// 如果没有变化，重新监听
        } else {
            // 监听到了文件描述符号的变化
            // lfd 变化
            if (FD_ISSET(lfd, &r_set)) {
                // 提取
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                char ip[16] = "";
                // 提取新的连接
                int cfd = Accept(lfd, (struct sockaddr *) &client_address, &len);
                inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
                cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(client_address.sin_port) << endl;
                // 将cfd添加至oldest集合中，以下次监听

                FD_SET(cfd, &old_set);

                //                map[lfd]=string(ip)+":"+to_string(ntohs(client_address.sin_port));

                // 更新maxfd
                if (cfd > maxfd) {
                    maxfd = cfd;
                }
                // 如果只有lfd变化 continue
                if (--n == 0) {
                    continue;
                }
            }

            // cfd 变化 遍历lfd之后的文件描述符是否

            for (int i = lfd + 1; i <= maxfd; ++i) {

                // 如果i文件描述符在r_set中
                if (FD_ISSET(i, &r_set)) {

                    char buffer[1500] = "";

                    ssize_t ret_n = Read(i, buffer, sizeof(buffer));

                    if (ret_n < 0) {
                        // error 将cfd关闭 从oldset中删除
                        cerr << "error !!! " << endl;
                        close(i);
                        FD_CLR(i, &old_set);
                    } else if (ret_n == 0) {

                        cout << "client close . . ." << endl;
                        close(i);
                        FD_CLR(i, &old_set);
                    } else {
                        cout << "client data : " << buffer << endl;
                        string str(buffer);
                        str = "server data : " + str;
                        write(i, str.c_str(), str.size());
                    }
                }
            }
        }
    }


    return 0;
}
