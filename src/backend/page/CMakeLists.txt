set(SOURCE_FILES
        DatabaseHeader.cpp
        Freelist.cpp
        TreeHeader.cpp
        InternalNode.cpp)

target_sources(noid_backend PUBLIC ${SOURCE_FILES})