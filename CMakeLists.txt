
# ==============================================

cmake_minimum_required(VERSION 3.15)

# ==============================================

set (CMAKE_CXX_STANDARD 23)

project(basic_json)

# ==============================================

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

enable_testing()

file(GLOB_RECURSE BASIC_JSON_TESTS_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${BASIC_JSON_TESTS_SOURCE})

add_executable(basic_json_tests ${BASIC_JSON_TESTS_SOURCE})
add_dependencies(basic_json_tests gtest)
target_include_directories(basic_json_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(basic_json_tests GTest::GTest)

add_test(basic_json_tests_gtests basic_json_tests)

# ==============================================
