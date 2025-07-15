cmake_minimum_required(VERSION 3.10)
project(MemoryPool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

file(GLOB SRC_FILES "src/*.cpp")
file(GLOB TEST_FILES "test.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES} ${TEST_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE -g -pthread)

target_link_libraries(${PROJECT_NAME} pthread)
