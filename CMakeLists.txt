cmake_minimum_required(VERSION 3.24)

project(WeatherBaseSimulationFramework)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#TODO RAYS-2083: -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS flag silences all deprecation warnings because the
# warnings make debugging compiler issues extremely difficult becacuse there are hundreds of them so for now we suppress them
add_definitions(
  -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
  -D_UNICODE  
)

add_compile_options(/MP)

remove_definitions(-U_MBCS)

add_definitions("/wd4996")

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(ExternalProject)

SET(EXTERNAL_PROJECTS_DIR "${CMAKE_SOURCE_DIR}/external")
SET(EXTERNAL_LIB_DIR "${EXTERNAL_PROJECTS_DIR}/lib64")

#dlib
SET(EXTERNAL_DLIB_DIR "${EXTERNAL_PROJECTS_DIR}/dlib")
SET(EXTERNAL_DLIB_INSTALL_DIR "${EXTERNAL_DLIB_DIR}/build/install")
ExternalProject_Add(dlib
	SOURCE_DIR "${EXTERNAL_DLIB_DIR}"
	BINARY_DIR "${EXTERNAL_DLIB_DIR}/build"
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${EXTERNAL_DLIB_INSTALL_DIR}
)

SET(DLIB_INCLUDE_DIRS "${EXTERNAL_DLIB_INSTALL_DIR}/include")
SET(DLIB_LIB_DIR "${EXTERNAL_DLIB_INSTALL_DIR}/lib")
# here dlib outputs both its debug and release libraries in the same directory.  
# List them all, then keep a single entry and replace debug and release by CONFIG so it can be replaced later

SET(DLIB_LIB_PREFIX "${DLIB_LIB_DIR}/dlib19.24.0_")
SET(DLIB_LIB_SUFFIX "_64bit_msvc${MSVC_VERSION}.lib")

#boost
SET(EXTERNAL_BOOST_DIR "${EXTERNAL_PROJECTS_DIR}/boost")
ExternalProject_Add(boost
	SOURCE_DIR "${EXTERNAL_BOOST_DIR}"
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	CONFIGURE_COMMAND ""
)
EXTERNALPROJECT_GET_PROPERTY(boost SOURCE_DIR)
SET(BOOST_INCLUDE_DIRS "${SOURCE_DIR}/include")

#gdal
SET(EXTERNAL_GDAL_DIR "${EXTERNAL_PROJECTS_DIR}/gdal")
ExternalProject_Add(gdal
	SOURCE_DIR "${EXTERNAL_GDAL_DIR}"
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	CONFIGURE_COMMAND ""
)
EXTERNALPROJECT_GET_PROPERTY(gdal SOURCE_DIR)
SET(GDAL_INCLUDE_DIRS "${SOURCE_DIR}/include")
SET(GDAL_LIB_DIRS "${SOURCE_DIR}/lib64")

if (WIN32)
  message(STATUS "Enabling multi-processor compilation for MSVC")
  add_compile_options(/MP)
else()
  message(STATUS "Adding -pthread for posix thread support on linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

enable_testing()

add_subdirectory(External)
add_subdirectory(wbs)
add_subdirectory(wbsTools)

#if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
#  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
#  set_target_properties(wbs PROPERTIES FOLDER "wbs")  
#endif()