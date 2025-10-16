include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(test_basic test_basic.cpp)
add_executable(run_tests
    test_parser.cpp
)
target_include_directories(test_basic PRIVATE ROOT_DIR/include)
target_link_libraries(test_basic PRIVATE cppre)

target_include_directories(run_tests PRIVATE ROOT_DIR/include)
target_link_libraries(run_tests
    PRIVATE cppre
    PRIVATE GTest::gtest_main
)

gtest_discover_tests(run_tests)

#add_test(NAME test_basic COMMAND test_basic)
