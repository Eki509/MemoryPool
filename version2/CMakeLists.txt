cmake_minimum_required(VERSION 3.10)
project(myMemoryPool)

set(CMAKE_BUILD_TYPE Debug)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 编译选项
add_compile_options(-Wall -O2)

# 查找pthread库
find_package(Threads REQUIRED)

# 设置目录
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)

# 源文件
file(GLOB SOURCES "${SRC_DIR}/*.cpp")

# 添加头文件目录
include_directories(${INC_DIR})

# 创建测试可执行文件
add_executable(test
    ${SOURCES}
    test.cpp
)

# 链接pthread库
target_link_libraries(test PRIVATE Threads::Threads)

# 添加测试命令
add_custom_target(perf
    COMMAND ./test
    DEPENDS test
)