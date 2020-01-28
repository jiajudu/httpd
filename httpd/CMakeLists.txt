cmake_minimum_required(VERSION 3.10)
project(httpd)
file(GLOB SRCS
    "*.h"
    "*.cpp"
)
include_directories(.)
add_subdirectory(server)
add_subdirectory(socket)
add_subdirectory(auxiliary)
add_subdirectory(service)
add_executable(httpd ${SRCS})
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Werror -Wconversion -Wold-style-cast -Woverloaded-virtual -Wpointer-arith -Wshadow -Wwrite-strings -march=native -std=c++17 -pthread -rdynamic")
set(EXECUTABLE_OUTPUT_PATH ./bin/)
