cmake_minimum_required(VERSION 3.24)
project(noid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -Wall)

find_package(Catch2 REQUIRED)

add_executable(noid main.cpp)

include_directories(src)
add_subdirectory(src)
add_subdirectory(test)