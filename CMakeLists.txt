cmake_minimum_required(VERSION 3.15)
project(cppre VERSION 1.0 LANGUAGES CXX)

option(CPPRE_USE_PARSER_COMB "Use the functional parser combinator engine (ON) or the recursive descent parser (more performant) (OFF)" OFF)

if(CPPRE_USE_PARSER_COMB)
    include(FetchContent)
    FetchContent_Declare(
        parser_comb
        GIT_REPOSITORY "https://github.com/i-m-ag-e/parser_comb.git"
        GIT_TAG "main"
    )
    set(PARSERCOMB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(parser_comb)
endif()

set(CMAKE_CXX_STANDARD 20)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTING "Build tests" ON)

add_library(cppre src/AST.cpp src/VM.cpp src/Regex.cpp)
target_compile_options(cppre PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(cppre PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -ggdb>
)
if (CPPRE_USE_PARSER_COMB)
    target_sources(cppre PRIVATE src/ParseParserComb.cpp)
    target_link_libraries(cppre PRIVATE ParserComb::ParserComb)
else()
    target_sources(cppre PRIVATE src/ParseHandwritten.cpp)
endif()
    
target_include_directories(cppre
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif ()
