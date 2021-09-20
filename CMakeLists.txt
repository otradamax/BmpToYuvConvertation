cmake_minimum_required(VERSION 3.0.0)

project(TASK)

add_library(Lib STATIC Lib.cpp Lib.h)

add_executable(TASK task.cpp)

target_link_libraries(TASK PRIVATE Lib)