add_executable(noid_tests noid_tests.cpp)
target_compile_definitions(noid_tests PUBLIC NOID_TEST_BUILD)
target_link_libraries(noid_tests
        PUBLIC noid_backend
        PUBLIC Catch2::Catch2)

set(SOURCE_FILES
        backend/vfs/UnixFileTests.cpp)

target_sources(noid_tests PUBLIC ${SOURCE_FILES})

include(CTest)
include(Catch)
catch_discover_tests(noid_tests)