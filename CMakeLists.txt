cmake_minimum_required(VERSION 3.16)
project(WebServerCpp11)

set(CMAKE_CXX_STANDARD 14)

set(INC_DIR /usr/include/mysql/)
set(LINK_DIR /usr/lib/x86_64-linux-gnu/)
include_directories(${INC_DIR})
link_directories(${LINK_DIR})
link_libraries(mysqlclient)

AUX_SOURCE_DIRECTORY(./src DIR_SRCS)
add_executable(WebServerCpp11 ${DIR_SRCS} test/hash_table.h test/test_my_db.cpp test/test_my_db.h)

target_link_libraries(WebServerCpp11 mysqlclient)