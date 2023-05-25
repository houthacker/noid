project(noid_backend)

set(SOURCE_FILES
        UnixVFS.cpp
        UnixFile.cpp
        Inode.cpp
        )

target_sources(noid_backend PUBLIC ${SOURCE_FILES})