file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
        *.cpp
)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(ChePP_engine STATIC ${ENGINE_SOURCES})

target_include_directories(ChePP_engine PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(ChePP main.cpp)

target_link_libraries(ChePP PRIVATE ChePP_engine)
