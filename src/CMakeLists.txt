file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
        *.cpp
)

list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(engine ${ENGINE_SOURCES})
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ChePP main.cpp)
target_link_libraries(ChePP PRIVATE engine)
