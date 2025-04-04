cmake_minimum_required(VERSION 3.24)

project(MPBLib)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES MPBColdTolerance.h MPBColdTolerance.cpp MPBDevRates.h MPBDevRates.cpp DevelopementVector.h DevelopementVector.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(MPBLib STATIC
  ${SOURCE_FILES}
)

add_subdirectory(CTModel)
add_subdirectory(iModel)
add_subdirectory(SLRModel)