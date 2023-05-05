cmake_minimum_required(VERSION 3.24)
project(noid)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -Wall)

include_directories(src)
add_subdirectory(src)
add_subdirectory(Google_Tests)

add_executable(noid main.cpp)
target_include_directories(noid PUBLIC src)
target_include_directories(Google_Tests_run PUBLIC src)
target_link_libraries(noid noid_algorithm noid_backend)