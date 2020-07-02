cmake_minimum_required(VERSION 3.8.2)

project(framepacker CXX)

add_subdirectory(third)

add_library(framepacker INTERFACE)

target_include_directories(framepacker INTERFACE "${PROJECT_SOURCE_DIR}")

add_executable(framepacker_cmd framepacker.cpp)

target_link_libraries(framepacker_cmd PRIVATE framepacker framepacker_third_party)

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	target_compile_options(framepacker_cmd PRIVATE -Wall -Wextra -Werror -Wfatal-errors -std=c++11)
endif((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
