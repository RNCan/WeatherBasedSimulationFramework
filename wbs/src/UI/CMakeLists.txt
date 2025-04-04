cmake_minimum_required(VERSION 3.24)

project(wbsUI)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MFC_FLAG 2)

add_definitions(-D_AFXDLL)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp ./Common/*.h ./Common/*.cpp ./Common/HTMLTree/*.h ./Common/HTMLTree/*.cpp ./Common/TmplEx/*.h ./Common/TmplEx/*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafxUI
  ${EXTERNAL_PROJECTS_DIR}
  ${GDAL_INCLUDE_DIRS}
)

# Build as static library
add_library(wbsUI ${SOURCE_FILES})


