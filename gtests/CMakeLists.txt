include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_executable(ChePP_tests ${TEST_SOURCES})

target_include_directories(ChePP_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ChePP_tests PRIVATE
        GTest::gtest_main
        ChePP_engine
)

include(GoogleTest)
gtest_discover_tests(ChePP_tests)
