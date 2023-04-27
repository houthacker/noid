project(noid_storage)

set(HEADER_FILES
        KeyBearer.h
        BPlusTreeNode.h
        BPlusTreeLeafNode.h
        BPlusTreeInternalNode.h
        BPlusTree.h
        BPlusTreeRecord.h
        BPlusTreeKey.h
        Shared.h
        Algorithm.h)

set(SOURCE_FILES
        BPlusTreeLeafNode.cpp
        BPlusTreeInternalNode.cpp
        BPlusTreeRecord.cpp
        BPlusTreeKey.cpp
        BPlusTree.cpp)

add_library(noid_storage ${SOURCE_FILES} ${HEADER_FILES})