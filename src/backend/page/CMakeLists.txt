set(SOURCE_FILES
        DatabaseHeader.cpp
        Freelist.cpp
        TreeHeader.cpp
        InternalNode.cpp
        LeafNode.cpp)

target_sources(noid_backend PUBLIC ${SOURCE_FILES})