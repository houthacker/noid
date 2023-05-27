project(noid_tests)

find_package(Catch2 REQUIRED)

add_executable(noid_tests noid_tests.cpp)
target_compile_definitions(noid_tests
        PRIVATE NOID_TEST_BUILD)

target_compile_features(noid_tests
        PUBLIC cxx_std_20)

target_link_libraries(noid_tests
        PRIVATE noid_backend
        PRIVATE Catch2::Catch2)

add_subdirectory(backend)

include(CTest)
include(Catch)
catch_discover_tests(noid_tests)