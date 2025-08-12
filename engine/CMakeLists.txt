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

set(NETWORK_BIN ${CMAKE_CURRENT_SOURCE_DIR}/resources/network.net)
set(NETWORK_HEADER ${CMAKE_CURRENT_BINARY_DIR}/network_net.h)
set(NETWORK_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/network_net.c)

add_executable(bin2c ../scripts/bin2c.c)

add_custom_command(
        OUTPUT ${NETWORK_HEADER} ${NETWORK_SOURCE}
        COMMAND $<TARGET_FILE:bin2c> ${NETWORK_BIN} ${NETWORK_HEADER} ${NETWORK_SOURCE} "network_net"
        DEPENDS ${NETWORK_BIN} ../scripts/bin2c.c bin2c
        COMMENT "Generating embedded network_net resource"
)

add_custom_target(embed_network ALL DEPENDS ${NETWORK_HEADER} ${NETWORK_SOURCE})


list(APPEND ENGINE_SOURCES
        ${fathom_SOURCE_DIR}/src/tbprobe.c
)

add_library(ChePP_engine STATIC ${ENGINE_SOURCES} ${NETWORK_SOURCE})

target_include_directories(ChePP_engine PUBLIC
        ${CMAKE_SOURCE_DIR}/engine/include
        ${fathom_SOURCE_DIR}
        ${highway_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)


target_compile_definitions(ChePP_engine PRIVATE TB_NO_THREADS=1)

add_dependencies(ChePP_engine embed_network)



add_executable(ChePP src/main.cpp)
target_link_libraries(ChePP PRIVATE ChePP_engine)


add_subdirectory(gtests)
