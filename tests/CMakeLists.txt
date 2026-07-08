include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
include(CTest)

add_executable(test_basic test_basic.cpp)

if (CPPRE_USE_PARSER_COMB)
    set(TARGET_PREFIX "parser_comb_")
else()
    set(TARGET_PREFIX "handwritten_")
endif()

add_executable(run_tests
    test_parser.cpp
    test_match.cpp
    test_search.cpp
)
target_include_directories(test_basic PRIVATE ROOT_DIR/include)
target_link_libraries(test_basic PRIVATE cppre)

target_include_directories(run_tests PRIVATE ROOT_DIR/include)
target_link_libraries(run_tests
    PRIVATE cppre
    PRIVATE GTest::gtest_main
)

gtest_discover_tests(run_tests
    TEST_PREFIX ${TARGET_PREFIX}
)

#add_test(NAME test_basic COMMAND test_basic)
