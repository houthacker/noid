cmake_minimum_required(VERSION 3.24)
project(noid)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
add_subdirectory(src)
add_subdirectory(Google_Tests)

add_executable(noid main.cpp)
target_link_libraries(noid noid_storage)