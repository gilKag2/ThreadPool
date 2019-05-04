cmake_minimum_required(VERSION 3.14)
project(ex4 C)

set(CMAKE_C_STANDARD 99)

add_executable(ex4 osqueue.c osqueue.h threadPool.h threadPool.c)