#pragma once  // 预处理指令，防止头文件的多重包含，与 #ifndef #define #endif 作用相同

// author: 闫世航
// date: 2023-10-04
// version: 0.0.1

#define YSH_THREAD_POOL_VERSION "v0.0.1 (2023-10-04)"

#include <future>
#include <functional>
#include "type_traits"
#include "vector"
#include "utility"
#include "queue"
#include "thread"
#include "iostream"
#include "chrono"

namespace YSH {

// 用于thread_pool类，表示系统中表示并行运行线程的数量的类型
using concurrency_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;
// 下面的方式同样能够获得上面的结果（一种类型）
//using concurrency_t = decltype(std::thread::hardware_concurrency())

// begin class multi_future

template <typename T>
class multi_future {
public:
//    构造一个能容纳给定数量的multi_future对象
    explicit multi_future(const size_t num_future=0) : futures(num_future){};

// 等待异步线程都执行完毕后，返回所有的future结果
    [[nodiscard]] std::conditional_t<std::is_void_v<T>,void,std::vector<T>> get() {
//        判断异步执行的各函数是否有返回值，若有则返回包含所有返回值的vector
        if constexpr (std::is_void_v<T>) {
            for (auto &item:futures) {
                item.wait();
            }
            return;
        }else {
            std::vector<T> res;
            for (auto &item:futures) {
                res.push_back(item.get());
//                res.push_back(std::move(item));  // 通过std::move将左值转换为右值引用，不用copy变量了，提高了性能
            }
            return res;
        }
    }

//    重载下标运算符
    [[nodiscard]] std::future<T>& operator[](const size_t index) {
//        判断给定的下标是否超出异步线程的数量
        if (index>=futures.size()) {

        }else {
            return futures[index];
        }
    }

//    向multi_future中添加future
    void push_back(std::future<T> future) {
        futures.push_back(std::move(future));
    }

//    等待所有异步的future执行完毕
    void wait() const{
        for (auto &item : futures) {
            item.wait();
        }
    }

//    返回multi_future的大小，即含有多少个future
    [[nodiscard]] size_t size(){
        return futures.size();
    }


private:
    std::vector<std::future<T>> futures;  // 保存future的vector对象
};

// end class multi_future



// begin class blocks
// 用于将给定范围划分为几个块
template <typename T1,typename T2,typename T = std::common_type_t<T1,T2>>
class blocks {
public:
    blocks(const T1 first_index_,const T2 after_last_index_,const size_t num_blocks_) : first_index(static_cast<T>(first_index_)),after_last_index(static_cast<T>(after_last_index_)),num_blocks(num_blocks_) {
        if (first_index>after_last_index)
            std::swap(first_index,after_last_index);
        if (num_blocks <= 0)
            num_blocks = 1;
        total_size =static_cast<size_t>(after_last_index-first_index);
        block_size = static_cast<size_t>(total_size/num_blocks);
        if (block_size == 0) {
            block_size = 1;
            num_blocks = (total_size > 1)? total_size:1;
        }
    };

    [[nodiscard]] T get_block_start(const size_t i) const {
        return static_cast<T>(i*block_size) + first_index;
    }

    [[nodiscard]] T get_block_end(const size_t i) const {
        return (i == num_blocks -1) ? after_last_index : (static_cast<T>(i*block_size) + first_index);
    }

    [[nodiscard]] size_t get_num_blocks() const {
        return num_blocks;
    }

    [[nodiscard]] size_t get_total_size() const {
        return total_size;
    }

//    [[nodiscard]] size_t get_block_size() const {
//        return block_size;
//    }

private:
    // 块的大小--一个块包含多大的范围
    size_t block_size = 0;
    // 整个范围的第一个元素值
    T first_index = 0;
    // 整个范围的尾后元素值
    T after_last_index = 0;
    // 块的数量
    size_t num_blocks = 0;
    // 整个范围的范围大小
    size_t total_size = 0;
};

// end class blocks



// begin class thread_pool

class thread_pool {
public:
    // 公有成员函数
    thread_pool(const concurrency_t thread_num_ = 0) : thread_num(determine_thread_num(thread_num_)),threads(std::make_unique<std::thread[]>(thread_num)) {
        create_threads();
    }

    ~thread_pool() {
        destory_threads();
    }



    [[nodiscard]] size_t get_tasks_queued() const {
        return tasks.size();
    }

    [[nodiscard]] size_t get_tasks_running_num() const {
        return task_running_num;
    }

    [[nodiscard]] size_t get_total_tasks_num() const {
        return tasks.size() + task_running_num;
    }

    [[nodiscard]] concurrency_t get_threads_num() const {
        return thread_num;
    }

    [[nodiscard]] bool ispaused() const {
        return pause_pool;
    }

    template<typename F,typename T1,typename T2,typename T = std::common_type_t<T1,T2>,typename R = std::invoke_result_t<std::decay<F>,T,T>>
    [[nodiscard]] multi_future<R> parallelize_loop(const T1 first_index,const T2 after_last_index,F&& loop,const size_t blocks_num = 0) {
        blocks blk(first_index,after_last_index,blocks_num? blocks_num:thread_num);
//        blocks blk{first_index,after_last_index,blocks_num? blocks_num:thread_num};
        if(blk.get_total_size()>0){
            multi_future<R> mf(blk.get_num_blocks());
            for (size_t i = 0; i < blk.get_num_blocks(); ++i) {
                mf[i] = submit(std::forward<F>(loop),blk.get_block_start(i),blk.get_block_end(i));
            }
            return mf;
        }else {
            return multi_future<R>();
        }
    }

    template<typename F,typename T,typename R = std::invoke_result_t<std::decay<F>,T,T>>
    [[nodiscard]] multi_future<R> parallelize_loop(const T after_last_index,F&& loop,const size_t blocks_num = 0) {
        return parallelize_loop(0,after_last_index,std::forward<F>(loop),blocks_num);
    }

    void pause() {
        std::unique_lock tasks_lock(tasks_mutex);
        pause_pool = true;
    }

    void purge_tasks() {
        std::unique_lock tasks_lock(tasks_mutex);
        while(!tasks.empty())
            tasks.pop();
    }

    template<typename F,typename ...T,typename R = std::invoke_result_t<std::decay_t<F>,std::decay_t<T>...>>
    [[nodiscard]] std::future<R> submit(F&& task,T&& ...args) {
        std::shared_ptr<std::promise<R>> task_promise = std::make_shared<std::promise<R>>();
        push_task(
                [task_function = std::bind(std::forward<F>(task),std::forward<T>(args)...),task_promise] {
                    try {
                        if constexpr (std::is_void_v<R>) {
                            std::invoke(task_function);
                            task_promise->set_value();
                        }
                        else {
                            task_promise->set_value(std::invoke(task_function));
                        }

                    }catch(...) {
                        try {
                            task_promise->set_exception(std::current_exception());
                        }catch(...) {
                            
                        }
                    }
        });
        return task_promise->get_future();
    }

    template<typename F,typename T1,typename T2,typename T = std::common_type_t<T1,T2>>
    void push_loop(const T1 first_index,const T2 after_last_index,F&& loop,const size_t blocks_num=0) {
        blocks blk(first_index,after_last_index,blocks_num?blocks_num:thread_num);
        if (blk.get_total_size() > 0) {
            for (size_t i = 0; i < blk.get_num_blocks(); ++i) {
                push_task(std::forward<F>(loop),blk.get_block_start(i),blk.get_block_end(i));
            }
        }
    }

    template<typename F,typename T>
    void push_loop(const T after_last_index,F&& loop,const size_t blocks_num=0) {
        push_loop(0,after_last_index,std::forward<F>(loop),blocks_num);
    }

    template<typename F,typename ...A>
    void push_task(F&& function,A&& ...args) {
        {
            const std::scoped_lock tasks_lock(tasks_mutex);
            tasks.push(std::bind(std::forward<F>(function),std::forward<A>(args)...));
        }
        task_available_cv.notify_one();
    }

    void reset(const concurrency_t  thread_num_=0) {
        std::unique_lock tasks_lock(tasks_mutex);
        const bool old_pause = pause_pool;
        pause_pool = true;
        tasks_lock.unlock();
        wait_for_tasks();
        destory_threads();
        thread_num = determine_thread_num(thread_num_);
        threads = std::make_unique<std::thread[]>(thread_num);
        pause_pool = old_pause;
        create_threads();
    }



    void unpause() {
        std::unique_lock tasks_lock(tasks_mutex);
        pause_pool = false;
    }



    void wait_for_tasks() {
        std::unique_lock tasks_lock(tasks_mutex);
        waiting = true;
        tasks_done_cv.wait(tasks_lock,[this] {return (pause_pool || !tasks.empty()) && !task_running_num;});
        waiting = false;
    }

    template<typename R,typename P>
    bool wait_for_tasks_duration(const std::chrono::duration<R,P>& duration) {
        std::unique_lock tasks_lock(tasks_mutex);
        waiting = true;
        const bool status = tasks_done_cv.wait_for(tasks_lock,duration,[this] {return (pause_pool || !tasks.empty()) && !task_running_num;});
        waiting = false;
        return status;
    }

    template<typename R,typename P>
    bool wait_for_tasks_until(const std::chrono::time_point<R,P>& time_point) {
        std::unique_lock tasks_lock(tasks_mutex);
        waiting = true;
        const bool status = tasks_done_cv.wait_until(tasks_lock,time_point,[this] {return (pause_pool || !tasks.empty()) && !task_running_num;});
        waiting = false;
        return status;
    }


private:
    // 私有成员函数

    // 创建线程池中的线程，并为每个线程分配worker
    void create_threads() {
        {
            const std::scoped_lock tasks_lock(tasks_mutex);
            stop_pool = false;
        }
        for(concurrency_t i = 0 ;i< thread_num;++i) {
            threads[i] = std::thread(&thread_pool::worker,this);
        }
    }

    // 销毁线程池中的所有线程
    void destory_threads() {
        {
            const std::scoped_lock tasks_lock(tasks_mutex);
            stop_pool = true;
        }
        task_available_cv.notify_all();
        for (concurrency_t i = 0; i < thread_num; ++i) {
            threads[i].join();
        }
    }
    // 确定线程池中的线程数量
    [[nodiscard]] concurrency_t determine_thread_num(const concurrency_t thread_num_) const{
        if (thread_num_>0) {
            return thread_num_;
        }else if (std::thread::hardware_concurrency()>0) {
            return std::thread::hardware_concurrency();
        }else {
            return 1;
        }


    }

    // worker会被分配给每个线程，worker就是线程要执行的函数（线程要完成的工作）
    void worker() {
        std::function<void()> task;
        while (true) {
            std::unique_lock tasks_lock(tasks_mutex);
            task_available_cv.wait(tasks_lock,[this] {return stop_pool||(!tasks.empty() && !pause_pool);});
            if (stop_pool) {
                break;
            }
            task = std::move(tasks.front());
            tasks.pop();
            ++task_running_num;
            tasks_lock.unlock();
            task();
            tasks_lock.lock();
            --task_running_num;
            if (waiting && !task_running_num && (pause_pool || tasks.empty()))
                tasks_done_cv.notify_all();
        }
    }


    // 私有数据成员

    // 互斥访问任务队列的互斥量
    std::mutex tasks_mutex{};

    // 这里的同步原语使用方式相当于生产者-消费者问题
    // 同步原语：是否有可运行的任务
    std::condition_variable task_available_cv{};
    // 同步原语：线程池和任务队列中的任务是否都运行完成，若被pause则忽略任务队列中的任务
    std::condition_variable tasks_done_cv{};

    // 正在运行任务数量
    size_t task_running_num = 0;
    // 线程池中的线程数量，无论是否在运行、在阻塞、在空跑
    concurrency_t thread_num = 0;

    // 用于临时暂停线程池的工作
    bool pause_pool = false;
    // 用于彻底停止线程池的工作
    bool stop_pool = true;

    // 用于表示是否类的用户是否在wait_for，即类的用户是否主动等待所有任务完成
    bool waiting = false;

    // 保存线程池中创建的多个线程
    std::unique_ptr<std::thread[]> threads= nullptr;
    // 任务列表（队列）：保存用于运行的任务
    std::queue<std::function<void()>> tasks = {};

};

// end class thread_pool


// begin class synced_stream

// 用于将不同的线程输出流进行同步输出。

class synced_out_stream{
public:
    synced_out_stream(std::ostream &out_stream_ = std::cout):out_stream(out_stream_){}

    // 保证不同线程互斥的输出
    template<typename ...T>
    void print(T&& ...args) {
        std::unique_lock stream_lock(stream_mutex);
        (out_stream << ... << std::forward<T>(args));
    }

    // 每次输出一个参数就换行
    template<typename ...T>
    void println(T&& ...args) {
        print(std::forward<T>(args)...,'\n');
    }

    // 下面的静态变量endl和flush能实现类似std::endl和std::flush的功能。
    // 使用静态变量endl和flush，而不是让用户直接使用std::endl和std::flush的原因是：
//    封装性：synced_stream类通过这种方式允许用户调用endl和flush，而不必直接依赖于std::endl和std::flush。这提高了类的封装性，因为用户可以在不了解底层细节的情况下使用synced_stream类的功能。
//    扩展性：如果将来需要自定义的行为或添加其他自定义操作，你可以在synced_stream 类中轻松修改endl和flush的行为，而不必影响到使用该类的代码。
//    一致性：通过使用synced_stream类中的endl和flush，可以确保所有输出操作都经过相同的同步控制，不会出现混合使用std::endl和std::flush的情况，提高了一致性。
    inline static std::ostream& (&endl) (std::ostream&) = static_cast<std::ostream& (&) (std::ostream&)>(std::endl);
    inline static std::ostream& (&flush) (std::ostream&) = static_cast<std::ostream& (&) (std::ostream&)>(std::flush);

private:

    // 输出流
    std::ostream &out_stream;

    // 访问输出流的互斥量
    mutable std::mutex stream_mutex{};
};

// end class synced_stream


// begin class timer

class timer {
public:
    timer() = default;

    // 开始：记录开始时间
    void start() {
        start_time = std::chrono::steady_clock::now();
    }

    // 结束：记录结束时间
    void end() {
        end_time = std::chrono::steady_clock::now();
    }

    // 返回消耗的时间（单位是ms）
    [[nodiscard]] std::chrono::milliseconds::rep get_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    }


private:
    // 用于保存开始的时间
    std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();

    // 用于保存结束的时间
    std::chrono::time_point<std::chrono::steady_clock> end_time = std::chrono::steady_clock::now();

};

// end class timer

}