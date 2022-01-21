//
// Created by yjs on 2022/1/15.
//

#include <iostream>
#include <unistd.h>
#include "thread_pool_class.h"


using namespace std;




void print1(const int & num){
    sleep(2);
    cout<< num<<endl;
}
void print2(const int & num){
    sleep(4);
    cout<< num<<endl;
}
void print3(const int & num){
    sleep(1);
    cout<< num<<endl;
}

int main(){
    ssize_t thread_num=3;
//    ThreadPool(thread_num);
    ThreadPool* pools= new  ThreadPool(thread_num);
    std::future<void> f1 =pools->commit(print1,1);
    std::future<void> f2 =pools->commit(print2,2);
    std::future<void> f3 =pools->commit(print3,3);
    f1.get();
    f2.get();
    f3.get();






    return 0;
}