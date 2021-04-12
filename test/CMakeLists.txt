cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME WebServerCpp11Test)
project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_CXX_STANDARD 14)

# add gtest
if (NOT TARGET gtest_main)
    add_subdirectory(googletest)
endif ()

include_directories(${gtest_SOURCE_DIR}/include ${gmock_SOURCE_DIR}/include)

# 要测试的文件
set(SRC_ROOT ../src)
set(SRC_FILE ${SRC_ROOT}/buffer/buffer.cpp ${SRC_ROOT}/buffer/buffer.h ${SRC_ROOT}/log/block_queue.h ${SRC_ROOT}/log/log.cpp ${SRC_ROOT}/log/log.h)
# 测试文件
set(TEST_FILE buffer_unittest.cpp block_queue_unittest.cpp)

add_executable(${PROJECT_NAME} main.cpp ${TEST_FILE})
target_link_libraries(${PROJECT_NAME} gtest_main gtest gmock)
