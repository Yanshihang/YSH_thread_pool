## 简介

多线程是现代高性能计算必不可少的技术。自从 C++11 以来，C++ 标准库已经包含了使用 std::thread 等结构的内置低级多线程支持。然而，std::thread 每次调用都会创建一个新线程，这会带来显著的性能开销。此外，创建的线程数量可能超过硬件能够同时处理的数量，这可能导致明显的性能下降。

这里介绍的库包含一个 C++ 线程池类 YSH::thread_pool，它通过一次性创建固定数量的线程池来避免这些问题，然后在程序的生命周期内不断重复使用相同的线程来执行不同的任务。默认情况下，线程池中的线程数量等于硬件可以并行运行的最大线程数量。

用户将要执行的任务提交到队列中。每当一个线程可用时，它就会从队列中检索下一个任务并执行它。线程池会自动为每个任务生成一个 std::future，这允许用户等待任务完成执行，并/或获取其最终返回值（如果适用）。线程和任务由线程池在后台自动管理，除了提交所需的任务外，不需要用户的任何输入。

这个包的设计遵循了四个重要的原则。首先是**简洁性**: 整个库只有一个小的自包含头文件，没有其他组件或依赖项。其次是**可移植性**: 该包只使用 C++17 标准库，不依赖于任何编译器扩展或第三方库，因此与任何现代标准兼容的 C++17 编译器在任何平台上都兼容。第三是**易用性**: 该包有详尽的文档记录，任何级别的程序员都可以立即使用它。

第四个也是最后一个指导原则是**性能**: 这个库中的每一行代码都是精心设计的，以实现最佳性能，并在各种编译器和平台上进行了性能测试和验证。

其他更高级的多线程库可能提供更多功能和/或更高的性能。然而，它们通常由具有多个组件和依赖项的庞大代码库组成，并涉及需要大量时间学习的复杂 API。这个库并不是要取代那些更高级的库；相反，它是为那些不需要非常高级功能的用户设计的，他们更喜欢一个简单轻量级的包，它易于学习和使用，可以很容易地集成到现有或新的项目中。

### 功能概述

- **快速:**
  - 从零开始构建，以实现最佳性能。
  - 适用于具有大量 CPU 核心的高性能计算节点。
  - 代码紧凑，以减少编译时间和二进制大小。
  - 重用线程避免了为单个任务创建和销毁线程的开销。
  - 任务队列确保并行运行的线程数量永远不会超过硬件允许的数量。
- **轻量级:**
  - 单个头文件：只需 #include "YSH_thread_pool.hpp" 即可！
  - 仅头文件：无需安装或构建库。
  - 自包含：没有外部需求或依赖项。
  - 可移植：仅使用 C++ 标准库，并与任何 C++17 兼容的编译器一起使用。
- **易于使用:**
  - 使用少量成员函数即可完成非常简单的操作。
  - 使用 submit() 成员函数提交到队列的每个任务都会自动生成一个 std::future，可用于等待任务完成执行和/或获取其最终返回值。
  - 循环可以使用 parallelize_loop() 成员函数自动并行化为任意数量的并行任务，该函数返回一个 YSH::multi_future，可用于一次跟踪所有并行任务的执行。
  - 如果不需要future，则可以使用 push_task() 提交任务，并使用 push_loop() 并行化循环 - 牺牲便利性以获得更高的性能。
  - 代码使用 Doxygen 注释进行了彻底的记录 - 不仅是接口，还有实现，以防用户想要进行修改。
- **辅助类:**
  - 使用 YSH::multi_future 辅助类一次跟踪多个future的执行。
  - 使用 YSH::synced_stream 辅助类同步来自多个线程的输出到流。
  - 使用 YSH::timer 辅助类轻松测量执行时间以进行基准测试。
- **附加功能:**
  - 使用 wait_for_tasks()、wait_for_tasks_duration() 和 wait_for_tasks_until() 成员函数轻松等待队列中的所有任务完成。
  - 使用 reset() 成员函数根据需要安全地动态更改池中的线程数。
  - 使用 get_tasks_queued()、get_tasks_running() 和 get_tasks_total() 成员函数监控排队和/或正在运行的任务数。
  - 使用 pause()、unpause() 和 is_paused() 成员函数自由暂停和恢复池；暂停时，线程不会从队列中检索新任务。
  - 使用 purge() 成员函数清除当前在队列中等待的所有任务。
  - 通过其future从主线程捕获使用 submit() 或 parallelize_loop() 提交的任务抛出的异常。
  - 将类成员函数提交到池，可以应用于特定对象或从对象内部应用。
  - 按值、引用或常量引用将参数传递给任务。

*参考[YSHhoshany/thread-pool: YSH::thread_pool: a fast, lightweight, and easy-to-use C++17 thread pool library (github.com)](https://github.com/YSHhoshany/thread-pool)*

