project(noid_api)

set(HEADER_FILES
        Noid.h
        Error.h)

set(SOURCE_FILES
        Noid.cpp)

add_library(noid_api SHARED ${SOURCE_FILES} ${HEADER_FILES})