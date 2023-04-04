project(noid_storage)

set(HEADER_FILES
        BPlusTreeNode.h
        BPlusTreeLeafNode.h)

set(SOURCE_FILES
        BPlusTreeLeafNode.cpp)

add_library(noid_storage ${SOURCE_FILES} ${HEADER_FILES})