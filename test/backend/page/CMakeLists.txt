project(noid_tests)

set(SOURCE_FILES
        FileHeaderTests.cpp
        FreelistTests.cpp
        InternalNodeTests.cpp
        LeafNodeTests.cpp
        NodeRecordTests.cpp
        OverflowTests.cpp
        TreeHeaderTests.cpp)

target_sources(noid_tests PUBLIC ${SOURCE_FILES})