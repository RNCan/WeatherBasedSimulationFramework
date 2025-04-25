cmake_minimum_required(VERSION 3.24)

project(Tranosema-OBL-SBW)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES Tranosema_OBL_SBW.h Tranosema_OBL_SBW.cpp Tranosema_OBL_SBW_Model.h Tranosema_OBL_SBW_Model.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../Tranosema
  ${CMAKE_CURRENT_SOURCE_DIR}/../ObliqueBandedLeafroller
  ${CMAKE_CURRENT_SOURCE_DIR}/../SpruceBudworm
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as shared library
add_library(Tranosema-OBL-SBW SHARED
  ${SOURCE_FILES}
)

target_link_libraries(Tranosema-OBL-SBW PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	TranosemaLib
	ObliqueBandedLeafrollerLib
	SpruceBudwormLib
)

