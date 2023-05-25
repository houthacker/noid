project(noid_api)

set(HEADER_FILES
        Noid.h)

set(SOURCE_FILES
        Noid.cpp)

add_library(noid_api ${SOURCE_FILES} ${HEADER_FILES})