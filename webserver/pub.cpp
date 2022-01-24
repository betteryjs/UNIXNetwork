#include "pub.h"
//通过文件名字获得文件类型
string get_mime_type(const string &str) {
    string dot;
    size_t pos = str.rfind(".");
    //        res=str.substr(pos);
    //        cout<< res<<endl;
    if (pos == string::npos) {
        return string("text/plain; charset=utf-8");
    } else {
        dot = str.substr(pos);
    }
    if (dot == ".html" || dot == ".htm")
        return string("text/html; charset=utf-8");
    if (dot == ".jpg" || dot == ".jpeg")
        return string("image/jpeg");
    if (dot == ".gif")
        return string("image/gif");
    if (dot == ".png")
        return string("image/png");
    if (dot == ".css")
        return string("text/css");
    if (dot == ".au")
        return string("audio/basic");
    if (dot == ".wav")
        return string("audio/wav");
    if (dot == ".avi")
        return string("video/x-msvideo");
    if (dot == ".mov" || dot == ".qt")
        return string("video/quicktime");
    if (dot == ".mpeg" || dot == ".mpe")
        return string("video/mpeg");
    if (dot == ".vrml" || dot == ".wrl")
        return string("model/vrml");
    if (dot == ".midi" || dot == ".mid")
        return string("audio/midi");
    if (dot == ".mp3")
        return string("audio/mpeg");
    if (dot == ".ogg")
        return string("application/ogg");
    if (dot == ".pac")
        return string("application/x-ns-proxy-autoconfig");
    return string("text/plain; charset=utf-8");
};


/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
//获得一行数据，每行以\r\n作为结束标记
int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n')) {
        n = recv(sock, &c, 1, 0);
        /* DEBUG printf("%02X\n", c); */
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);//MSG_PEEK 从缓冲区读数据，但是数据不从缓冲区清除
                /* DEBUG printf("%02X\n", c); */
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        } else
            c = '\n';
    }
    buf[i] = '\0';

    return (i);
}

/*
 * 这里的内容是处理%20之类的东西！是"解码"过程。
 * %20 URL编码中的‘ ’(space)
 * %21 '!' %22 '"' %23 '#' %24 '$'
 * %25 '%' %26 '&' %27 ''' %28 '('......
 * 相关知识html中的‘ ’(space)是&nbsp
 */
// %E8%8B%A6%E7%93%9C
void urldecode(char *to, char *from) {
    for (; *from != '\0'; ++to, ++from) {

        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {//依次判断from中 %20 三个字符

            *to = hexit(from[1]) * 16 + hexit(from[2]);//字符串E8变成了真正的16进制的E8
            from += 2;                                 //移过已经处理的两个字符(%21指针指向1),表达式3的++from还会再向后移一个字符
        } else
            *to = *from;
    }
    *to = '\0';
}

//16进制数转化为10进制, return 0不会出现
int hexit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

//"编码"，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
//strencode(encoded_name, sizeof(encoded_name), name);
void urlencode(char *to, size_t tosize, const char *from) {
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char *) 0) {
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}