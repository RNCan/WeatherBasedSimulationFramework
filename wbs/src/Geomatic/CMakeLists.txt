cmake_minimum_required(VERSION 3.24)

project(Geomatic)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${DLIB_INCLUDE_DIRS}
  ${EXTERNAL_PROJECTS_DIR}  
  ${BOOST_INCLUDE_DIRS}  
  ${GDAL_INCLUDE_DIRS}
)

# Build as static library
add_library(Geomatic
  ${SOURCE_FILES}
)

add_dependencies(Geomatic boost gdal)
