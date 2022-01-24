//
// Created by yjs on 2022/1/15.
//

#ifndef thread_pool_yjs_H
#define thread_pool_yjs_H

#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


using namespace std;


class ThreadPool {
private:
    atomic<bool> stop{false};      //线程池是否执行
    condition_variable _task_cv;   //条件阻塞
    atomic<int> _idlThrNum{0};     //空闲线程数量
    queue<function<void()>> _tasks;//任务队列
    vector<thread> _pools;         //线t程池
    mutex _lock;                   //线程池的锁
    const int THREADPOOL_MAX_NUM = 16;

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
    auto commit(F &&f, Args &&...args) -> future<decltype(f(args...))> {

        if (stop) {
            throw runtime_error("commit on ThreadPool is stopped.");
        }
        using RetType = decltype(f(args...));// typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
        auto task = make_shared<packaged_task<RetType()>>(
                bind(forward<F>(f), forward<Args>(args)...));// 把函数入口及参数,打包(绑定)
        future<RetType> future = task->get_future();

        {// 添加任务到队列

            lock_guard<mutex> lock{_lock};//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
            _tasks.emplace([task]() {
                (*task)();
            });
        }
        _task_cv.notify_one();// 唤醒一个线程执行
        return future;
    }

    //空闲线程数量
    int free_thread_count();

    //线程数量
    unsigned long thread_count();
};


#endif// thread_pool_yjs_H
