cmake_minimum_required(VERSION 3.10)

project(loopback-linux)

set (CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCES main.cpp serial.hpp serial.cpp prng.hpp prng.cpp main.cpp)

add_executable(loopback-linux ${SOURCES})
target_link_libraries(loopback-linux Threads::Threads)

