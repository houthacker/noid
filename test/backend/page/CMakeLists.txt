project(noid_tests)

set(SOURCE_FILES
        DatabaseHeaderTests.cpp
        FreelistTests.cpp
        TreeHeaderTests.cpp
        InternalNodeTests.cpp)

target_sources(noid_tests PUBLIC ${SOURCE_FILES})