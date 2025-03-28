cmake_minimum_required(VERSION 3.24)

project(BioSim)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MFC_FLAG 2)

add_definitions(-D_AFXDLL)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../msvc/stdafxUI
  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src
#  ${CMAKE_CURRENT_SOURCE_DIR}/../../wbs/src/Basic
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_executable(BioSim WIN32 ${SOURCE_FILES})

add_dependencies(BioSim dlib)

target_link_libraries(BioSim 
	${EXTERNAL_LIB_DIR}/libboost_thread-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_chrono-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_filesystem-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_serialization-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_iostreams-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${EXTERNAL_LIB_DIR}/libboost_zlib-vc143-mt$<$<CONFIG:Debug>:-gd>-x64-1_84.lib 
	${GDAL_LIB_DIRS}/gdal_i.lib 
	${GDAL_LIB_DIRS}/proj_i.lib
	${DLIB_LIBS}
	version.lib
	Basic
	Geomatic
	FileManager
	Simulation
	ModelBase
	RangerLib
	wbsUI
	UltimateGrid
	MTParser
	KMeanLocal
	hxGrid
	Newmat	
)

 target_link_options(BioSim PRIVATE "/DELAYLOAD:gdal.dll")
 target_link_options(BioSim PRIVATE "/DELAYLOAD:proj_4_9.dll")