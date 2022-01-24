//
// Created by yjs on 2022/1/24.
//
#include <algorithm>
#include <event.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <arpa/inet.h>
#include <cstring>
#include <dirent.h>
#include <event.h>
#include <event2/listener.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


using namespace std;

namespace fs = std::filesystem;

static const int SERVER_PORT = 8001;
static const char SERVER_IP[] = "0.0.0.0";

void listen_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);
void bevent_cb(struct bufferevent *, short, void *);
void read_cb(struct bufferevent *, void *);
void read_client_requests(bufferevent *, string &);
static string get_mime_type(const string &);
int send_http_headers(bufferevent *, const int &, const string &, const string &, const int &);
int send_file_requests(bufferevent *, const string &);

static vector<string> split(const string &, const string &);
extern std::string UrlDecode(const std::string &);


// ###############################

static vector<string> split(const string &str, const string &delim) {//将分割后的子字符串存储在vector中
    vector<string> res;
    if (str.empty()) return res;
    string strs = str + delim;//*****扩展字符串以方便检索最后一个分隔出的字符串
    size_t pos;
    size_t size = strs.size();

    for (int i = 0; i < size; ++i) {
        pos = strs.find(delim, i);             //pos为分隔符第一次出现的位置，从i到pos之前的字符串是分隔出来的字符串
        if (pos < size) {                      //如果查找到，如果没有查找到分隔符，pos为string::npos
            string s = strs.substr(i, pos - i);//*****从i开始长度为pos-i的子字符串
            res.push_back(s);                  //两个连续空格之间切割出的字符串为空字符串，这里没有判断s是否为空，所以最后的结果中有空字符的输出，
            i = pos + delim.size() - 1;
        }
    }
    return res;
}


static string get_mime_type(const string &str) {
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


int send_file_requests(bufferevent *bev, const string &file_path) {


    fstream files;
    files.open(file_path, ios_base::in);
    if (!files) {

        cerr << "unable to open file" << endl;
        return 1;
    }
    string file_count;
    int ch;
    while ((ch = files.get()) != EOF) {

        file_count += ch;
    }
    bufferevent_write(bev, file_count.c_str(), file_count.size());
    files.close();
    return 0;
}

int send_http_headers(bufferevent *bev, const int &status_code, const string &info, const string &file_type, const int &length) {

    // 发送状态行
    string buffer = "HTTP/1.1 " + to_string(status_code) + " " + info + "\r\n";
    // 发送消息头
    buffer += "Content-Type:" + file_type + "\r\n";
    if (length > 0) {
        // 发送长度
        buffer += "Content-Length:" + to_string(length) + "\r\n";
    }
    buffer += "\r\n";
    // 发送空行
    bufferevent_write(bev, buffer.c_str(), buffer.size());
    return 0;
}

void read_client_requests(bufferevent *bev, string &path) {
    string string_file = UrlDecode(path.substr(1));
    // 判断请求的文件在不在
    //        fs::path dir(string_file);
    if (string_file.empty()) {
        // 127.0.0.1:8000/ 没有请求的文件 默认请求当前目录
        cout << "path [ "
             << "/"
             << " ] exists " << endl;
        string_file = "./";
    }
    if (fs::exists(string_file)) {
        cout << "path [ " << string_file << " ] exists " << endl;
        if (fs::is_directory(string_file)) {
            // 是一个目录
            cout << "is a directory " << endl;
            // 发送一个列表
            send_http_headers(bev, 200, string("OK"), get_mime_type("*.html"), 0);
            // 发送 dir_header.html
            send_file_requests(bev, "dir_header.html");

            vector<string> dirs;
            string string_li;
            if (fs::is_directory(string_file)) {
                for (fs::directory_entry entry: fs::directory_iterator(string_file)) {
                    if (entry.is_directory()) {
                        dirs.push_back(entry.path().filename().string() + "/");

                    } else {
                        dirs.push_back(entry.path().filename().string());
                    }
                }
            }
            for (string li: dirs) {

                string_li += "<li><a href=\"" + li + "\">" + li + "</a></li>";

                cout << string_li << endl;
                //                string_li.clear();
            }
            bufferevent_write(bev, string_li.c_str(), string_li.size());
            //            // 发送 dir_tail.html
            send_file_requests(bev, "dir_tail.html");


        } else {
            // 是一个文件
            cout << "is a file " << endl;
            // 先发送报头 （状态行 消息头）
            struct stat s;
            stat(string_file.c_str(), &s);
            send_http_headers(bev, 200, string("OK"), get_mime_type(string_file), s.st_size);
            // 发送文件
            send_file_requests(bev, string_file);
        }
    } else {
        cout << "path [ " << string_file << " ] not exists " << endl;
        // 先发送报头 （状态行 消息头）
        send_http_headers(bev, 404, string("NOT FOUND"), get_mime_type("*.html"), 0);
        // 发送 error.html
        send_file_requests(bev, "error.html");
    }
}


void read_cb(bufferevent *bev, void *pVoid) {
    char buffer[256] = {0};
    size_t ret = bufferevent_read(bev, buffer, sizeof(buffer));

    string request_head1(buffer);
    vector<string> res = split(request_head1, " ");
    string method, path, http_version;
    method = res[0];
    path = res[1];
    http_version = res[2];

    // 判断是否为GET 请求
    transform(method.begin(), method.end(), method.begin(), ::tolower);
    if (method == "get") {
        //处理客户端的请求
        char bufline[256];
        write(STDOUT_FILENO, buffer, ret);
        //确保数据读完
        while ((ret = bufferevent_read(bev, bufline, sizeof(bufline))) > 0) {
            write(STDOUT_FILENO, bufline, ret);
        }
        read_client_requests(bev, path);
    }
}

void listen_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *server_address, int socklen, void *arg) {
    cout << "new connect " << endl;
    //定义与客户端通信的bufferevent
    event_base *base = reinterpret_cast<event_base *>(arg);
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, nullptr, bevent_cb, base);//设置回调
    bufferevent_enable(bev, EV_READ | EV_WRITE);              //启用读和写
}
void bevent_cb(struct bufferevent *bev, short what, void *ctx) {
    if (what & BEV_EVENT_EOF) {//客户端关闭
        printf("client closed\n");
        bufferevent_free(bev);
    } else if (what & BEV_EVENT_ERROR) {
        printf("err to client closed\n");
        bufferevent_free(bev);
    } else if (what & BEV_EVENT_CONNECTED) {//连接成功
        printf("client connect ok\n");
    }
}


int main() {
    // 获取当前目录的工作路径
    fs::path pwd_path = fs::current_path();
    pwd_path /= "web-http";
    // 切换目录

    chdir(pwd_path.string().c_str());

    struct event_base *base = event_base_new();//创建根节点
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr.s_addr);
    //    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener *listener = evconnlistener_new_bind(base,
                                                              listen_cb, base, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                                              (struct sockaddr *) &server_address, sizeof(server_address));//连接监听器


    event_base_dispatch(base);//循环

    event_base_free(base);        //释放根节点
    evconnlistener_free(listener);//释放链接监听器
    return 0;
}