cmake_minimum_required(VERSION 3.24)

project(Newmat)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../msvc/stdafx
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(Newmat ${SOURCE_FILES})