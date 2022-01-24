//
// Created by yjs on 2022/1/12.
//
#include "thread_pool_yjs.h"

using namespace std;

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    create_threadpool(threads);
}

ThreadPool::~ThreadPool() {
    stop = true;
    _task_cv.notify_all();// 唤醒所有线程执行

    for (thread &thread: _pools) {
        //thread.detach(); // 让线程“自生自灭”
        if (thread.joinable())
            thread.join();// 等待任务结束， 前提：线程一定会执行完
    }
}


void ThreadPool::create_threadpool(size_t threads) {
    for (size_t i = 0; i < threads && threads < THREADPOOL_MAX_NUM; ++i) {
        _pools.emplace_back([this] {
            //工作线程函数
            while (!stop) {
                function<void()> task;// 获取一个待执行的 task
                {
                    // unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
                    unique_lock<mutex> lock{_lock};

                    _task_cv.wait(lock, [this] {
                        return stop || !_tasks.empty();
                    });// wait 直到有 task

                    if (stop && _tasks.empty())
                        return;
                    task = move(_tasks.front());// 按先进先出从队列取一个 task
                    _tasks.pop();
                    _idlThrNum--;
                    task();//执行任务
                    _idlThrNum++;
                }
            }
            _idlThrNum++;
        });
    }
}


//
//template<class F, class... Args>
//auto ThreadPool::commit(F&& f, Args&&... args) -> future<decltype(f(args...))>{
//
//    if(stop){
//        throw runtime_error("commit on ThreadPool is stopped.");
//
//    }
//    using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
//    auto task = make_shared<packaged_task<RetType()>>(
//            bind(forward<F>(f), forward<Args>(args)...)
//    ); // 把函数入口及参数,打包(绑定)
//    future<RetType> future = task->get_future();
//
//    {    // 添加任务到队列
//
//        lock_guard<mutex> lock{ _lock };//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
//        _tasks.emplace([task](){
//            (*task)();
//        });
//    }
//    _task_cv.notify_one(); // 唤醒一个线程执行
//    return future;
//
//}


//空闲线程数量
int ThreadPool::free_thread_count() { return _idlThrNum; }

//线程数量
unsigned long ThreadPool::thread_count() { return _pools.size(); }

//template<class F, class... Args>
//auto ThreadPool::commit(F &&f, Args &&... args) -> future<decltype(f(args...))> {
//
//    if(stop){
//        throw runtime_error("commit on ThreadPool is stopped.");
//
//    }
//    using RetType = decltype(f(args...)); // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
//    auto task = make_shared<packaged_task<RetType()>>(
//            bind(forward<F>(f), forward<Args>(args)...)
//    ); // 把函数入口及参数,打包(绑定)
//    future<RetType> futures = task->get_future();
//
//    {    // 添加任务到队列
//
//        lock_guard<mutex> lock{ _lock };//对当前块的语句加锁  lock_guard 是 mutex 的 stack 封装类，构造的时候 lock()，析构的时候 unlock()
//        _tasks.emplace([task](){
//            (*task)();
//        });
//    }
//    _task_cv.notify_one(); // 唤醒一个线程执行
//    return futures;
//
// }
