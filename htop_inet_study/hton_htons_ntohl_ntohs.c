//
// Created by yjs on 2022/1/4.
//

#include <arpa/inet.h>
#include <stdio.h>


int main() {

    unsigned char buffer[4] = {192, 68, 1, 2};
    printf("%d %d %d %d \n", *buffer, *(buffer + 1), *(buffer + 2), *(buffer + 3));
    printf("%x %x %x %x \n", buffer, (buffer + 1), (buffer + 2), (buffer + 3));
    uint32_t num = *(uint32_t *) buffer;
    uint32_t sum = ntohl(num);
    unsigned char *p = &sum;
    printf("%d %d %d %d \n", *p, *(p + 1), *(p + 2), *(p + 3));
    printf("%x %x %x %x \n", p, (p + 1), (p + 2), (p + 3));


    return 0;
}