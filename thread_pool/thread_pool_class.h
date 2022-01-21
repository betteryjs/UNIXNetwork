//
// Created by yjs on 2022/1/15.
//

#ifndef UNIXNETWORKTEST_THREAD_POOL_CLASS_H
#define UNIXNETWORKTEST_THREAD_POOL_CLASS_H


#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <future>
#include <queue>
#include <functional>


using namespace std;

#define  THREADPOOL_MAX_NUM 16

class ThreadPool{

private:
    atomic<bool> stop{false };     //线程池是否执行
    condition_variable _task_cv;   //条件阻塞
    atomic<int>  _idlThrNum{ 0 };  //空闲线程数量
    queue<function<void()>> _tasks; //任务队列
    vector<thread> _pools;     //线t程池
    mutex _lock; //线程池的锁

public:
     ThreadPool(size_t);
     ~ThreadPool();
    //添加指定数量的线程
    void create_threadpool(size_t);
    // 提交一个任务
    // 调用.get()获取返回值会等待任务执行完,获取返回值
    // 有两种方法可以实现调用类成员，
    // 一种是使用   bind： .commit(std::bind(&Dog::sayHello, &dog));
    // 一种是用   mem_fn： .commit(std::mem_fn(&Dog::sayHello), this)
    template<class F, class... Args>
    auto commit(F&& f, Args&&... args) ->future<decltype(f(args...))>;
    //空闲线程数量
    int free_thread_count() { return _idlThrNum; }
    //线程数量
    int thread_count() { return _pools.size(); }
};



#endif //UNIXNETWORKTEST_THREAD_POOL_CLASS_H
