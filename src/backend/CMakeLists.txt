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
        WAL.cpp
        page/Limits.h)

add_library(noid_backend SHARED ${SOURCE_FILES} ${HEADER_FILES})

# If there is support for large files, use it.
execute_process(COMMAND getconf FILESIZEBITS / OUTPUT_VARIABLE FILE_SIZE_BITS)
execute_process(COMMAND getconf LFS_CFLAGS OUTPUT_VARIABLE LFS_COMPILER_FLAGS)
execute_process(COMMAND getconf LFS_LDFLAGS OUTPUT_VARIABLE LFS_LINKER_FLAGS)
string(STRIP ${FILE_SIZE_BITS} FILE_SIZE_BITS)
string(STRIP ${LFS_COMPILER_FLAGS} LFS_COMPILER_FLAGS)
string(STRIP ${LFS_LINKER_FLAGS} LFS_LINKER_FLAGS)

if (NOT FILE_SIZE_BITS STREQUAL "")
    target_compile_definitions(noid_backend
            PUBLIC -D_FILE_OFFSET_BITS=${FILE_SIZE_BITS})
endif ()
if (NOT LFS_COMPILER_FLAGS STREQUAL "")
    target_compile_definitions(noid_backend
            PUBLIC ${LFS_COMPILER_FLAGS}
            PUBLIC -D_LARGEFILE_SOURCE
            PUBLIC -D_LARGEFILE64_SOURCE)
endif ()
if (NOT LFS_LINKER_FLAGS STREQUAL "")
    message(WARNING ${LFS_LINKER_FLAGS})
    target_link_options(noid_backend PUBLIC ${LFS_LINKER_FLAGS})
endif ()

add_subdirectory(concurrent)
add_subdirectory(page)
add_subdirectory(vfs)