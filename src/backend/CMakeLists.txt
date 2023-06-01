project(noid_backend)

set(HEADER_FILES
        Types.h
        Bits.h
        MD5.h
        UUID.h
        Pager.h
        WAL.h
        BPlusTree.h
        NoidConfig.h)

set(SOURCE_FILES
        MD5.cpp
        UUID.cpp
        Pager.cpp
        WAL.cpp
        BPlusTree.cpp page/Limits.h)

add_library(noid_backend SHARED ${SOURCE_FILES} ${HEADER_FILES})

add_subdirectory(concurrent)
add_subdirectory(page)
add_subdirectory(vfs)