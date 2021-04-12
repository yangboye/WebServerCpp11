cmake_minimum_required(VERSION 3.16)
set(PROJECT_NAME WebServerCpp11Src)
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 14)

set(INC_DIR /usr/include/mysql/)
set(LINK_DIR /usr/lib/x86_64-linux-gnu/)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})
link_libraries(mysqlclient)

find_package(Threads REQUIRED)

file(GLOB SRC_TIMER "timer/*.cpp" "timer/*.h")
#set(SRC_TIMER timer/heap_timer.cpp timer/heap_timer.h)
set(SRC_BUFFER buffer/buffer.cpp buffer/buffer.h)
set(SRC_POOL pool/thread_pool.h pool/sql_conn_pool.cpp pool/sql_conn_pool.h pool/sql_conn_raii.h)
set(SRC_LOG log/block_queue.h log/log.cpp log/log.h)
set(SRC_HTTP http/http_request.cpp http/http_request.h http/http_response.cpp http/http_response.h http/http_conn.cpp http/http_conn.h)
set(SRC_SERVER server/epoller.h server/web_server.cpp server/web_server.h)
add_executable(${PROJECT_NAME} main.cpp ${SRC_TIMER} ${SRC_BUFFER} ${SRC_POOL} ${SRC_LOG} ${SRC_HTTP} ${SRC_SERVER})

target_link_libraries(${PROJECT_NAME} mysqlclient)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

