cmake_minimum_required(VERSION 3.24)

project(WBSF_UnitTest)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(WBSF_UnitTest SHARED
  ${SOURCE_FILES}
)

target_link_libraries(WBSF_UnitTest Basic)
