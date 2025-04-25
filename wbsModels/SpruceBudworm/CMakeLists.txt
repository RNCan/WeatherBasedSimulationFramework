cmake_minimum_required(VERSION 3.24)

project(SpruceBudwormBiology)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES 
  SpruceBudworm.cpp
  SpruceBudworm.h
  SpruceBudwormEquations.cpp
  SpruceBudwormEquations.h
  )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SpruceBudwormLib STATIC
  ${SOURCE_FILES}
)

project(SpruceBudwormBiology)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES 
  SpruceBudwormModel.cpp
  SpruceBudwormModel.h
  )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SpruceBudwormBiology SHARED    
  ${SOURCE_FILES}
)

target_link_libraries(SpruceBudwormBiology PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	SpruceBudwormLib
)

######################  SpruceBudwormDispersal  ######################

project(SpruceBudwormDispersal)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES 
  SpruceBudwormDispersal.cpp
  SpruceBudwormDispersal.h
  )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SpruceBudwormDispersal SHARED
  ${SOURCE_FILES}
)

target_link_libraries(SpruceBudwormDispersal PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	SpruceBudwormLib
)

######################  SpruceBudwormLaboratory  ######################

project(SpruceBudwormLaboratory)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES 
  SpruceBudwormLaboratory.cpp
  SpruceBudwormLaboratory.h
  SpruceBudwormLaboratoryEquations.cpp
  SpruceBudwormLaboratoryEquations.h
  )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SpruceBudwormLaboratory SHARED
  ${SOURCE_FILES}
)

target_link_libraries(SpruceBudwormLaboratory PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	SpruceBudwormLib
)

######################  SpruceBudwormEldon  ######################

project(SpruceBudwormEldon)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES 
  SBW-Eldon-Model.cpp
  SBW-Eldon-Model.h
  )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SpruceBudwormEldon SHARED
  ${SOURCE_FILES}
)

target_link_libraries(SpruceBudwormEldon PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	SpruceBudwormLib
)

######################  SpruceBudwormEldon  ######################

project(SpruceBudwormLysyk)

set(CMAKE_CXX_STANDARD 17)

# Use the GLOB command to get all the source files
set(SOURCE_FILES 
  SBW-Lysyk-Model.cpp
  SBW-Lysyk-Model.h
  )

include_directories( 
  ${CMAKE_CURRENT_SOURCE_DIR}  
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafx
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as dynamic library
add_library(SpruceBudwormLysyk SHARED
  ${SOURCE_FILES}
)

target_link_libraries(SpruceBudwormLysyk PUBLIC
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	version.lib
	Basic
	ModelBase
	SpruceBudwormLib
)

