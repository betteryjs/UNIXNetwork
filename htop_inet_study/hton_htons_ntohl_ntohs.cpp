//
// Created by yjs on 2022/1/4.
//

#include <algorithm>
#include <arpa/inet.h>
#include <iomanip>
#include <iostream>

using namespace std;

int main() {
    const int bufferSize = 4;
    unsigned char buffer[bufferSize] = {192, 168, 1, 2};

    cout << "before ntohl : ";
    for_each(buffer, buffer + bufferSize, [](auto c) { cout << static_cast<int>(c) << " "; });
    cout << endl;

    uint32_t num = *((uint32_t *) buffer);// 取得4个字节


    uint32_t sum = ntohl(num);
    uint32_t *tmp = &sum;
    auto *p = reinterpret_cast<unsigned char *>(tmp);
    cout << "after ntohl : ";

    for_each(p, p + bufferSize, [](auto c) { cout << static_cast<int>(c) << " "; });
    cout << endl;


    uint16_t a = 0x0102;
    uint16_t b = htons(a);

    cout << showbase << hex << b << endl;
    cout << dec;
    unsigned char buffer2[bufferSize] = {1, 2, 168, 192};
    cout << "before ntohl : ";
    for_each(buffer2, buffer2 + bufferSize, [](auto c) { cout << static_cast<int>(c) << " "; });
    cout << endl;
    uint32_t num2 = *((uint32_t *) buffer2);// 取得4个字节


    uint32_t sum2 = ntohl(num2);
    uint32_t *tmp2 = &sum2;
    auto *p2 = reinterpret_cast<unsigned char *>(tmp2);
    cout << "after ntohl : ";

    for_each(p2, p2 + bufferSize, [](auto c) { cout << static_cast<int>(c) << " "; });
    cout << endl;


    return 0;
}