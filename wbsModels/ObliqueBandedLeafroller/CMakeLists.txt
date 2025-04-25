cmake_minimum_required(VERSION 3.24)

project(ObliqueBandedLeafrollerLib)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set (SOURCE_FILES ObliqueBandedLeafroller.h ObliqueBandedLeafroller.cpp ObliqueBandedLeafrollerEquations.h ObliqueBandedLeafrollerEquations.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

add_library(ObliqueBandedLeafrollerLib STATIC
  ${SOURCE_FILES}
)

project(ObliqueBandedLeafroller)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set (SOURCE_FILES ObliqueBandedLeafrollerModel.h ObliqueBandedLeafrollerModel.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(ObliqueBandedLeafroller SHARED
    ${SOURCE_FILES}
)

target_link_libraries(ObliqueBandedLeafroller PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	ObliqueBandedLeafrollerLib
)


