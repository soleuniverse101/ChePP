cmake_minimum_required(VERSION 3.16)
project(ChePP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  

add_compile_options(-O3 -march=native -Wall -Wextra -Wpedantic)
file(GLOB SRC_FILES src/*.cpp)
add_executable(ChePP ${SRC_FILES}
        src/move.h
        src/zobrist.h
        src/zobrist.cpp)
set_property(TARGET ChePP PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

