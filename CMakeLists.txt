cmake_minimum_required(VERSION 3.16)
project(WebServerCpp11)

set(CMAKE_CXX_STANDARD 14)

set(INC_DIR /usr/include/mysql/)
set(LINK_DIR /usr/lib/x86_64-linux-gnu/)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})
link_libraries(mysqlclient)

find_package(Threads REQUIRED)

add_executable(WebServerCpp11 src/main.cpp src/timer/heap_timer.cpp src/timer/heap_timer.h src/buffer/buffer.cpp src/buffer/buffer.h)

target_link_libraries(WebServerCpp11 mysqlclient)
target_link_libraries(WebServerCpp11 Threads::Threads)