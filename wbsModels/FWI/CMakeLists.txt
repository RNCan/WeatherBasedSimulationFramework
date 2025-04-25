cmake_minimum_required(VERSION 3.24)

project(FBPLib)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES FWI.h FWI.cpp FBP.h FBP.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(FBPLib STATIC
  ${SOURCE_FILES}
)

######################  FBP  ######################

project(FBP)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES "FWI(new from wang2015).h" "FWI(new from wang2015).cpp" FWI-Model.h FWI-Model.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(FBP SHARED
  ${SOURCE_FILES}  
)

target_link_libraries(FBP PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	FBPLib
)

######################  FWI  ######################

project(FWI)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES "FWI(new from wang2015).h" "FWI(new from wang2015).cpp" FWI-Model.h FWI-Model.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(FWI SHARED
  ${SOURCE_FILES}  
)

target_link_libraries(FWI PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	FBPLib
)

######################  FWIDroughtCode  ######################

project(FWIDroughtCode)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES DroughtCodeModel.h DroughtCodeModel.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(FWIDroughtCode SHARED
  ${SOURCE_FILES}  
)

target_link_libraries(FWIDroughtCode PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	FBPLib
)

######################  SummerMoisture  ######################

project(SummerMoisture)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES SummerMoisture.h SummerMoisture.cpp SummerMoistureModel.h SummerMoistureModel.cpp)

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SummerMoisture SHARED
  ${SOURCE_FILES}  
)

target_link_libraries(SummerMoisture PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	FBPLib
)