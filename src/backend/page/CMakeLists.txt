set(SOURCE_FILES
        FileHeader.cpp
        Freelist.cpp
        TreeHeader.cpp
        InternalNode.cpp
        LeafNode.cpp
        NodeRecord.cpp
        Overflow.cpp)

target_sources(noid_backend PUBLIC ${SOURCE_FILES})