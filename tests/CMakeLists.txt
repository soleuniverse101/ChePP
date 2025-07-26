include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_executable(run_tests ${TEST_SOURCES})

target_link_libraries(run_tests
        gtest_main
        engine
)

include(GoogleTest)
gtest_discover_tests(run_tests)
