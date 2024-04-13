cmake_minimum_required(VERSION 3.26)
project(new_thread_pool)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

#add_executable(
#        new_thread_pool
#        include/YSH_thread_pool.hpp
#        test/YSH_thread_pool_test.cpp
#        main.cpp
#)

add_executable(main main.cpp include/YSH_thread_pool.hpp)

