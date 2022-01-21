#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <list>
#include <queue>
#include <functional>
#include <future>
#include <condition_variable>
#include <unistd.h>


using std::cout;
using std::endl;



class ThreadPools{

public:
    using pool_seconds = std::chrono::seconds;
    /** 线程池的配置
        * core_threads: 核心线程个数，线程池中最少拥有的线程个数，初始化就会创建好的线程，常驻于线程池
        *
        * max_threads: >=core_threads，当任务的个数太多线程池执行不过来时，
        * 内部就会创建更多的线程用于执行更多的任务，内部线程数不会超过max_threads
        *
        * max_task_size: 内部允许存储的最大任务个数，暂时没有使用
        *
        * time_out: Cache线程的超时时间，Cache线程指的是max_threads-core_threads的线程,
        * 当time_out时间内没有执行任务，此线程就会被自动回收
     */
     struct ThreadPoolConfig{
         int core_threads;
         int max_threads;
         int max_task_size;
         pool_seconds time_out{};
         ThreadPoolConfig(){
             core_threads=2;
             max_threads=static_cast<int>(std::thread::hardware_concurrency());
             max_task_size=8;
             time_out=std::chrono::duration<int>(5); // 5s
         }
     };



   //// 线程的状态：有等待、运行、停止
   enum class ThreadState{
       StateInit=0,
       StateWaiting=1,
       StateRunning=2,
       StateStop=3
   };
   //// 线程的种类标识：标志该线程是核心线程还是Cache线程，Cache是内部为了执行更多任务临时创建出来的
   enum class ThreadFlag{
       FlagInit=0,
       FlagCorn=1,
       FlagCache=2
   };
    using ThreadPtr = std::shared_ptr<std::thread>;
    using ThreadId = std::atomic<int>;
    using ThreadStateAtomic = std::atomic<ThreadState>;
    using ThreadFlagAtomic = std::atomic<ThreadFlag>;
    //// 线程池中线程存在的基本单位，每个线程都有个自定义的ID，有线程种类标识和状态

    struct ThreadWrapper{
        ThreadPtr ptr;
        ThreadId id{};
        ThreadFlagAtomic flag{};
        ThreadStateAtomic state{};
//        ThreadWrapper(std::shared_ptr<ThreadWrapper> sharedPtr) {
//            ptr= nullptr;
//            id=0;
//            state.store(ThreadState::StateInit);
//        }
        ThreadWrapper() {
            ptr = nullptr;
            id = 0;
            state.store(ThreadState::StateInit);
        }
    };

    using ThreadWrapperPtr = std::shared_ptr<ThreadWrapper>;
    using ThreadPoolLock = std::unique_lock<std::mutex>;


    explicit ThreadPools(ThreadPoolConfig config): config_(config){
        // 设置初始函数函数数目为0
        this->total_function_num_.store(0);
        this->waiting_thread_num_.store(0);

        this->thread_id_.store(0);
        this->is_shutdown_.store(false);

        if (is_vaild_config(this->config_)){
            is_available_.store(true);
            cout<< "Thread Pools Config Loads Success . . ."<<endl;
        }else{
            is_available_.store(false);
        }
    }

    ~ThreadPools(){
        shut_down(false);
        cout<< "Thread Pools Destruction  Success . . ."<<endl;


    }


// 当前线程池是否可用
    bool is_available() { return is_available_.load(); }

    int GetNextThreadId() { return this->thread_id_++; }

    //// 向线程池中提交函数
    template<typename F,typename... Args>
    auto commit(F&& f,Args&&... args)->std::shared_ptr<std::future<decltype(f(args...))>>{
        if (this->is_shutdown_.load()|| !is_available()){
            return nullptr;
        }
        if (get_waiting_thread_size()==0 && get_total_thread_size()<config_.max_threads){

            // add_thread()
            add_thread(GetNextThreadId(), ThreadFlag::FlagCache);


        }
        using return_type = decltype(f(args...));

        auto task=std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f),std::forward<Args>(args)...));

        total_function_num_++;
        std::future<return_type> res=task->get_future();
        {
            ThreadPoolLock lock(this->task_mutex_);
            this->tasks_.template emplace([task](){

                (*task)();
            });
            this->task_cv_.notify_one();
            return std::make_shared<std::future<return_type>>(std::move(res));
        }







    }




    void add_thread(int id, ThreadFlag thread_flag) {
        cout << "add_thread " << id << " flag " << static_cast<int>(thread_flag) << endl;
        ThreadWrapperPtr thread_ptr = std::make_shared<ThreadWrapper>();

        thread_ptr->id.store(id);
        thread_ptr->flag.store(thread_flag);
        auto func = [this, thread_ptr]() {
            for (;;) {
                std::function<void()> task;
                {
                    ThreadPoolLock lock(this->task_mutex_);
                    if (thread_ptr->state.load() == ThreadState::StateStop) {
                        break;
                    }
                    cout << "thread id " << thread_ptr->id.load() << " running start" << endl;
                    thread_ptr->state.store(ThreadState::StateWaiting);
                    ++this->waiting_thread_num_;
                    bool is_timeout = false;
                    if (thread_ptr->flag.load() == ThreadFlag::FlagCache) {
                        this->task_cv_.wait(lock, [this, thread_ptr] {
                            return (this->is_shutdown_ || !this->tasks_.empty() ||
                                    thread_ptr->state.load() == ThreadState::StateStop);
                        });
                    } else {
                        this->task_cv_.wait_for(lock, this->config_.time_out, [this, thread_ptr] {
                            return (this->is_shutdown_ || !this->tasks_.empty() ||
                                    thread_ptr->state.load() == ThreadState::StateStop);
                        });
                        is_timeout = !(this->is_shutdown_ ||  !this->tasks_.empty() ||
                                       thread_ptr->state.load() == ThreadState::StateStop);
                    }
                    --this->waiting_thread_num_;
                    cout << "thread id " << thread_ptr->id.load() << " running wait end" << endl;

                    if (is_timeout) {
                        thread_ptr->state.store(ThreadState::StateStop);
                    }

                    if (thread_ptr->state.load() == ThreadState::StateStop) {
                        cout << "thread id " << thread_ptr->id.load() << " state stop" << endl;
                        break;
                    }
                    if (this->is_shutdown_ && this->tasks_.empty()) {
                        cout << "thread id " << thread_ptr->id.load() << " shutdown" << endl;
                        break;
                    }
                    thread_ptr->state.store(ThreadState::StateRunning);
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task();
            }
            cout << "thread id " << thread_ptr->id.load() << " running end" << endl;
        };
        thread_ptr->ptr = std::make_shared<std::thread>(std::move(func));
        if (thread_ptr->ptr->joinable()) {
            thread_ptr->ptr->detach();
        }
        this->worker_threads_.emplace_back(std::move(thread_ptr));


    }
        // 获取正在处于等待状态的线程的个数
    int get_waiting_thread_size() { return this->waiting_thread_num_.load(); }

    // 获取线程池中当前线程的总个数
    int get_total_thread_size() { return this->worker_threads_.size(); }


private:
    static bool is_vaild_config(ThreadPoolConfig config){

        if(config.core_threads<1||config.max_threads<config.core_threads){
            return false;

        } else{

            return true;
        }
    }
    void shut_down(bool is_now) {
        if (is_available_.load()) {
            if (is_now) {
                this->is_shutdown_.store(true);
            } else {
                this->is_shutdown_.store(true);
            }
            this->task_cv_.notify_all();
            is_available_.store(false);
        }
    }

private:
    ThreadPoolConfig config_;
    // 线程链表
    std::list<ThreadWrapperPtr> worker_threads_;
    // 函数队列
    std::queue<std::function<void()>> tasks_;
    std::mutex task_mutex_;
    std::condition_variable  task_cv_;

    std::atomic<int> total_function_num_{};
    std::atomic<int> waiting_thread_num_{};
    std::atomic<int> thread_id_{};

//    std::atomic<bool> is_shutdown_now_;
    std::atomic<bool> is_shutdown_{};
    std::atomic<bool> is_available_{};



};


std::string   func1(){


    sleep(4);
    std::string strs="hello func1";
//    cout << strs<<endl;
    return strs;

}

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
     auto config=ThreadPools::ThreadPoolConfig();


    ThreadPools* pool =new ThreadPools(config);
    auto f1=pool->commit(func1);
    std::string  res=f1->get();
    cout<< "f1 res:  "<<res<<endl;


//    std::future<void> f1 =pools->commit(print1,1);
//    std::future<void> f2 =pools->commit(print2,2);
//    std::future<void> f3 =pools->commit(print3,3);

    delete(pool);

    return 0;
}