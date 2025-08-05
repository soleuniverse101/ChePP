cmake_minimum_required(VERSION 3.14)
project(ChePP C CXX)


include(FetchContent)

FetchContent_Declare(
        fathom
        GIT_REPOSITORY https://github.com/basil00/Fathom.git
        GIT_TAG master
)
FetchContent_MakeAvailable(fathom)

FetchContent_Declare(
        highway
        GIT_REPOSITORY https://github.com/google/highway.git
        GIT_TAG master
)
FetchContent_MakeAvailable(highway)




file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
        src/*.cpp
)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

list(APPEND ENGINE_SOURCES
        ${fathom_SOURCE_DIR}/src/tbprobe.c
)

add_library(ChePP_engine STATIC ${ENGINE_SOURCES})

target_include_directories(ChePP_engine PUBLIC
        ${CMAKE_SOURCE_DIR}/engine/include
        ${fathom_SOURCE_DIR}
        ${highway_SOURCE_DIR}
)


target_compile_definitions(ChePP_engine PRIVATE TB_NO_THREADS=1)

add_executable(ChePP src/main.cpp)
target_link_libraries(ChePP PRIVATE ChePP_engine)


add_subdirectory(gtests)
