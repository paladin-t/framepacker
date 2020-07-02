cmake_minimum_required(VERSION 3.8.2)

add_library (framepacker_third_party
  stb_image.h
  stb_image.cpp
  stb_image_write.h
  stb_image_write.cpp)

target_include_directories(framepacker_third_party PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
