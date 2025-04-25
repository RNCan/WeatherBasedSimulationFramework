cmake_minimum_required(VERSION 3.24)

project(Basic)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp ./ANN/*.h ./ANN/*.cpp ./ARModel/*.h ./ARModel/*.cpp ./ERMsg/*.h ./ERMsg/*.cpp ./json/*.hpp ./json/*.cpp ./xml/zen/*.h ./xml/zenxml/*.h)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(Basic
  ${SOURCE_FILES}
)

