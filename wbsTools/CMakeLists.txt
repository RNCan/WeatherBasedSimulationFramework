cmake_minimum_required(VERSION 3.24)

project(wbsTools)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(BioSim)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_target_properties(BioSim PROPERTIES FOLDER "wbsTools")    
endif()

