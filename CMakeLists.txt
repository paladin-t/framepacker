cmake_minimum_required(VERSION 3.8.2)

project(framepacker_prj CXX)

add_subdirectory(third)

add_library(framepacker_prj INTERFACE)

target_include_directories(framepacker_prj INTERFACE "${PROJECT_SOURCE_DIR}")

add_executable(framepacker framepacker.cpp)

target_link_libraries(framepacker PRIVATE framepacker_prj framepacker_third_party)

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
	target_compile_options(framepacker PRIVATE -Wall -Wextra -Werror -Wfatal-errors -std=c++11)
endif((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
