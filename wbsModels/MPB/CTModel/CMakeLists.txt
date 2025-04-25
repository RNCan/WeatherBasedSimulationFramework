cmake_minimum_required(VERSION 3.24)

######################  MPB-ColdTolerance  ######################

project(MPB-ColdTolerance)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES MPB-CTModel.h MPB-CTModel.cpp )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src  
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(MPB-ColdTolerance SHARED
  ${SOURCE_FILES}
)

target_link_libraries(MPB-ColdTolerance PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	MPBLib
)




