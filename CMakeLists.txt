cmake_minimum_required(VERSION 3.24)

project(WeatherBaseSimulationFramework)

include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#TODO RAYS-2083: -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS flag silences all deprecation warnings because the
# warnings make debugging compiler issues extremely difficult becacuse there are hundreds of them so for now we suppress them
add_definitions(
  -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
  -D_UNICODE  
)

remove_definitions(-U_MBCS)

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

find_package(GDAL CONFIG REQUIRED)

# Boost settings
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     ON)  # link Boost Static libraries
find_package(Boost CONFIG REQUIRED)

find_package(dlib CONFIG REQUIRED)

find_package(Eigen3 CONFIG REQUIRED)

# Fetch the proj4 library
include(ExternalProject)
ExternalProject_Add(proj4
  GIT_REPOSITORY  https://github.com/OSGeo/PROJ.git
  GIT_TAG         4.9.3
  PREFIX ./packages  
)
EXTERNALPROJECT_GET_PROPERTY(proj4 SOURCE_DIR)
SET(PROJ4_INCLUDE_DIRS "${SOURCE_DIR}/src")
EXTERNALPROJECT_GET_PROPERTY(proj4 INSTALL_DIR)
SET(PROJ4_LIBRARY_DIRS "${INSTALL_DIR}")
message("Install dir of proj4 = ${INSTALL_DIR}/src/proj4-build/lib")

if (WIN32)
  message(STATUS "Enabling multi-processor compilation for MSVC")
  add_compile_options(/MP)
else()
  message(STATUS "Adding -pthread for posix thread support on linux")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_subdirectory(wbs/src/Geomatic)