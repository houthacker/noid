project(Google_tests)

add_subdirectory(gtest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

add_executable(Google_Tests_run
        noid/storage/BPlusTreeLeafNodeTests.cpp
        noid/storage/BPlusTreeTests.cpp
        noid/storage/BPlusTreeInternalNodeTests.cpp
        noid/storage/AlgorithmTests.cpp)

target_link_libraries(Google_Tests_run gtest gtest_main gmock gmock_main noid_storage)