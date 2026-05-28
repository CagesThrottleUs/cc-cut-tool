cmake_minimum_required(VERSION 4.0.0)
project(cc-cut-tool VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------- Find Packages -----------------------------

find_package(Boost REQUIRED)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/cut/config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/cut/config.hpp"
    @ONLY
)

# ----------------------------- Main Executable -----------------------------

set(SRC_FILES
  src/list_parser.cpp
)

add_executable(${PROJECT_NAME} src/main.cpp ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

# ----------------------------- Testing -----------------------------

set(TEST_PROJECT_NAME ${PROJECT_NAME}-test)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/8736d2cd5c1dcba41170ed2fddca14021d4916c3.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(${TEST_PROJECT_NAME}
  tests/cut/mode_test.cpp
  tests/cut/list_test.cpp
  tests/cut/options_test.cpp
  tests/cut/file_source_test.cpp
  tests/cut/list_parser_test.cpp
  ${SRC_FILES}
)

target_include_directories(${TEST_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

target_link_libraries(${TEST_PROJECT_NAME} PRIVATE
  GTest::gtest_main
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(${TEST_PROJECT_NAME} PRIVATE -fsanitize=address)
  target_link_options(${TEST_PROJECT_NAME} PRIVATE -fsanitize=address)
endif()


include(GoogleTest)
gtest_discover_tests(${TEST_PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


# Integration Tests

add_subdirectory(integrations)
