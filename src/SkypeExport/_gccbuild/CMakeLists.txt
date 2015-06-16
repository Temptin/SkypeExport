cmake_minimum_required(VERSION 2.8)

project(SkypeExport)

find_package(Boost 1.46 COMPONENTS
    program_options
    filesystem
    regex
    system
    REQUIRED
)

add_executable(SkypeExport
    ../main.cpp
    ../libs/sqlite3/sqlite3.c
    ../model/skypeparser_core.cpp
    ../model/skypeparser_parsing.cpp
)

include_directories(SkypeExport
    ../libs
    ../model
    ../resources/css_and_images
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(SkypeExport
    ${Boost_LIBRARIES}
)
