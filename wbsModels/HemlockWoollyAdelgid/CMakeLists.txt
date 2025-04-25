cmake_minimum_required(VERSION 3.24)

project(HemlockWoollyAdelgid)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES HemlockWoollyAdelgid.h HemlockWoollyAdelgid.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(HemlockWoollyAdelgid SHARED
  ${SOURCE_FILES}
)

target_link_libraries(HemlockWoollyAdelgid PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
)

######################  HWAPhenology  ######################

project(HWAPhenology)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES HWAPhenology.h HWAPhenology.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(HWAPhenology SHARED
  ${SOURCE_FILES}
)

target_link_libraries(HWAPhenology PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
)

