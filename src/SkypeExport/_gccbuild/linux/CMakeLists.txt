cmake_minimum_required(VERSION 2.8)

project(SkypeExport)

find_package(Boost 1.46 COMPONENTS
    program_options
    filesystem
    system
    regex
    REQUIRED
)

add_executable(SkypeExport
    ../../main.cpp
    ../../libs/sqlite3/sqlite3.c
    ../../model/skypeparser_core.cpp
    ../../model/skypeparser_parsing.cpp
)

include_directories(SkypeExport
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(SkypeExport
    ${Boost_LIBRARIES}
)

# disable sqlite3 threading and dynamic extension loader to avoid pthreads/dl dependencies
add_definitions(-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION)
