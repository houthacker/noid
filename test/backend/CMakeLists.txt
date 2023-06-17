project(noid_tests)

set(SOURCE_FILES
        BitsTests.cpp
        MD5Tests.cpp
        PagerTests.cpp
        AlgorithmTests.cpp
        BPlusTreeTests.cpp)

target_sources(noid_tests PUBLIC ${SOURCE_FILES})

add_subdirectory(page)
add_subdirectory(vfs)