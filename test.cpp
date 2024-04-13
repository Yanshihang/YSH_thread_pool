//
// Created by 24958 on 2024/4/13.
//

#include "include/YSH_thread_pool.hpp"
#include <cassert>
#include <numeric>


void test_thread_pool() {
    YSH::thread_pool pool;
    auto future = pool.submit([]() { return 1 + 1; });
    assert(future.get() == 2);
}

void test_push_loop() {
    YSH::thread_pool pool;
    std::vector<int> data(100, 1);
//    std::mutex data_mutex;
    pool.push_loop(0, data.size(), [&](int start, int end) {
        for (int i = start; i < end; ++i) {
//            std::lock_guard<std::mutex> lock(data_mutex);
            data[i] += 1;
        }
    });
    pool.wait_for_tasks();
    std::cout << std::accumulate(data.begin(), data.end(), 0) << std::endl;
    assert(std::accumulate(data.begin(), data.end(), 0) == 200);
}

void test_parallelize_loop() {
    YSH::thread_pool pool;
    std::vector<int> data(100, 1);
    auto mf = pool.parallelize_loop(static_cast<size_t>(0), data.size(), [&](size_t start, size_t end) {
        int sum = 0;
        for (size_t i = start; i < end; ++i) {
            sum += data[i];
        }
        return sum;
    });
    auto results = mf.get();
    assert(std::accumulate(results.begin(), results.end(), 0) == 100);
}

void test_pause_unpause() {
    YSH::thread_pool pool;
    pool.pause();
    auto future = pool.submit([]() { return 1 + 1; });
    pool.unpause();
    assert(future.get() == 2);
}

void test_wait_for_tasks() {
    YSH::thread_pool pool;
    auto future = pool.submit([]() { return 1 + 1; });
    pool.wait_for_tasks();
    assert(future.get() == 2);
}

void test_multi_future() {
    YSH::multi_future<int> mf;
    std::future<int> future = std::async(std::launch::async, []() { return 1 + 1; });
    mf.push_back(std::move(future));
    assert(mf[0].get() == 2);
}

//void test_synced_out_stream() {
//    YSH::synced_out_stream out;
//    out.print("Hello, world!");
//}

int test() {
    test_thread_pool();
    test_push_loop();
//    test_parallelize_loop();
    test_pause_unpause();
    test_wait_for_tasks();
    test_multi_future();
//    test_synced_out_stream();
    return 0;
}