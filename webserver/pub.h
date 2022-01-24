#ifndef _PUB_H
#define _PUB_H
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;
//char *get_mime_type(char *name);
string get_mime_type(const string &);

int get_line(int sock, char *buf, int size);
int hexit(char c);                                        //16进制转10进制
void urlencode(char *to, size_t tosize, const char *from);//编码
void urldecode(char *to, char *from);                     //解码
#endif
