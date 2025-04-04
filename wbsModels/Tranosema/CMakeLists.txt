cmake_minimum_required(VERSION 3.24)

project(TranosemaLib)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES Tranosema.h Tranosema.cpp TranosemaEquations.h TranosemaEquations.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(TranosemaLib STATIC
  ${SOURCE_FILES}
)

######################  TranosemaBiology  ######################

project(TranosemaBiology)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES TranosemaModel.h TranosemaModel.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as shared library
add_library(TranosemaBiology SHARED
  ${SOURCE_FILES}
)

target_link_libraries(TranosemaBiology PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	TranosemaLib
)


