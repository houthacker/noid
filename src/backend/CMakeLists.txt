project(noid_backend)

set(HEADER_FILES
        Types.h
        Bits.h
        MD5.h
        UUID.h
        Pager.h
        WAL.h)

set(SOURCE_FILES
        MD5.cpp
        UUID.cpp
        Pager.cpp
        WAL.cpp)

add_library(noid_backend ${SOURCE_FILES} ${HEADER_FILES})

add_subdirectory(page)
target_include_directories(noid_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/page)