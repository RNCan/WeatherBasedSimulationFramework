cmake_minimum_required(VERSION 3.24)

project(RangerLib)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.hpp ./*.cpp ./Forest/*.h ./Forest/*.cpp ./Tree/*.h ./Tree/*.cpp ./Utility/*.h ./Utility/*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(RangerLib
  ${SOURCE_FILES}
)

target_compile_options(RangerLib PRIVATE /bigobj)

