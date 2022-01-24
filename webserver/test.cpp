////
//// Created by yjs on 2022/1/24.
////
//#include <fstream>
//#include <iostream>
//
//using namespace std;
//
//string get_mime_type(const string &str) {
//    string dot;
//    size_t pos = str.rfind(".");
//    //        res=str.substr(pos);
//    //        cout<< res<<endl;
//    if (pos == string::npos) {
//        return string("text/plain; charset=utf-8");
//    } else {
//        dot = str.substr(pos);
//    }
//    if (dot == ".html" || dot == ".htm")
//        return string("text/html; charset=utf-8");
//    if (dot == ".jpg" || dot == ".jpeg")
//        return string("image/jpeg");
//    if (dot == ".gif")
//        return string("image/gif");
//    if (dot == ".png")
//        return string("image/png");
//    if (dot == ".css")
//        return string("text/css");
//    if (dot == ".au")
//        return string("audio/basic");
//    if (dot == ".wav")
//        return string("audio/wav");
//    if (dot == ".avi")
//        return string("video/x-msvideo");
//    if (dot == ".mov" || dot == ".qt")
//        return string("video/quicktime");
//    if (dot == ".mpeg" || dot == ".mpe")
//        return string("video/mpeg");
//    if (dot == ".vrml" || dot == ".wrl")
//        return string("model/vrml");
//    if (dot == ".midi" || dot == ".mid")
//        return string("audio/midi");
//    if (dot == ".mp3")
//        return string("audio/mpeg");
//    if (dot == ".ogg")
//        return string("application/ogg");
//    if (dot == ".pac")
//        return string("application/x-ns-proxy-autoconfig");
//    return string("text/plain; charset=utf-8");
//};
//
//int main() {
//    //    fstream files;
//    //    files.open("Makefile",ios_base::in);
//    //    if (!files){
//    //
//    //        cerr<< "unable to open file"<<endl;
//    //        return 1;
//    //    }
//    //    string  file_count;
//    //    int ch;
//    //    while ((ch=files.get())!=EOF){
//    //
//    //        file_count+=ch;
//    //    }
//    //    cout<< file_count;
//
//    auto res=get_mime_type("aaagggjpac");
//    cout<< res<<endl;
//    return 0;
//}
//
// Created by yjs on 2022/1/11.
//

#include <dirent.h>
#include <filesystem>
#include <iostream>
#include <vector>

using namespace std;
namespace fs = filesystem;


int main() {
    fs::path dir("/home/yjs");
    //is_block_file
    //is_character_file
    //is_directory
    //is_fifo
    //is_other
    //is_regular_file
    //is_socket
    //is_symlink
    vector<string> dirs;
    string string_li;
    if (fs::is_directory(dir)) {
        for (fs::directory_entry entry: fs::directory_iterator(dir)) {
            if (entry.is_directory()) {
                dirs.push_back(entry.path().filename().string() + "/");

            } else {

                dirs.push_back(entry.path().filename().string());
            }
        }
    }
    for (string li: dirs) {
        string_li = "<li><a href=\"" + li + "\">" + li + "</a></li>";
        cout << string_li << endl;
    }

    //
    //       int scandir(const char *dirp, struct dirent ***namelist,
    //              int (*filter)(const struct dirent *),
    //              int (*compar)(const struct dirent **, const struct dirent **));


    return 0;
}