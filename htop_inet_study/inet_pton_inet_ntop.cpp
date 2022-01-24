//
// Created by yjs on 2022/1/4.
//

#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

int main() {

    //     char str[]="192.168.2.1";
    //     const short bufferSize=4;
    //    unsigned int num=0;
    //    unsigned int* tmp=&num;
    //    auto p=reinterpret_cast<unsigned char *>(tmp);
    //
    //    inet_pton(AF_INET, str, &num);
    ////    printf("%d %d %d %d \n",*p,*(p+1),);
    //    for_each(p, p+bufferSize,[](auto c){cout<< static_cast<int>(c) << " ";});

    char str[] = "192.168.2.1";
    unsigned char buf[sizeof(struct in_addr)];// ipv4
    inet_pton(AF_INET, str, buf);
    for_each(begin(buf), end(buf), [](auto c) { cout << static_cast<int>(c) << " "; });
    cout << endl;


    //    uint32_t sum =ntohl(*((uint32_t *)buf));
    //    unsigned char buf2[sizeof(struct in_addr)];

    char ip[16];
    inet_ntop(AF_INET, buf, ip, INET6_ADDRSTRLEN);
    cout << ip << endl;


    return 0;
}