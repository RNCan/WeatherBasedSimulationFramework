cmake_minimum_required(VERSION 3.24)

project(hxGrid)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MFC_FLAG 2)

add_definitions(-D_AFXDLL)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./Common/*.h ./Common/*.cpp ./Interface/*.h ./Interface/*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../msvc/stdafx
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(hxGrid ${SOURCE_FILES})