add_subdirectory(unix)

set(SOURCE_FILES
        VFSFactory.cpp)

target_sources(noid_backend PUBLIC ${SOURCE_FILES})