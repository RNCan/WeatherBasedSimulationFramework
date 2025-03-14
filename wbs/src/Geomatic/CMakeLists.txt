cmake_minimum_required(VERSION 3.24)

project(Geomatic)

set(CMAKE_CXX_STANDARD 17)

#set (BOOST_ROOT "W:/NRCan/cpp/boost_1_84_0")
#set (BOOST_LIBRARY_DIR "W:/NRCan/cpp/boost_1_84_0/lib64-msvc-14.3")

#find_package(Boost COMPONENTS system REQUIRED)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${DLIB_INCLUDE_DIRS}
  ${GDAL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${Eigen3_INCLUDE_DIRS}
  ${PROJ4_INCLUDE_DIRS}
)

# Build as static library
add_library(Geomatic
  ${SOURCE_FILES}
)

add_dependencies(Geomatic dlib::dlib GDAL::GDAL proj4)
