project(noid_tests)

set(SOURCE_FILES
        FileHeaderTests.cpp
        FreelistTests.cpp
        TreeHeaderTests.cpp
        InternalNodeTests.cpp
        LeafNodeTests.cpp)

target_sources(noid_tests PUBLIC ${SOURCE_FILES})