cmake_minimum_required(VERSION 3.24)

project(External)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(UltimateGrid)
add_subdirectory(MTParser)
add_subdirectory(KMeanLocal)
add_subdirectory(hxGrid)
add_subdirectory(Newmat)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_target_properties(UltimateGrid PROPERTIES FOLDER "External")
  set_target_properties(MTParser PROPERTIES FOLDER "External")
  set_target_properties(KMeanLocal PROPERTIES FOLDER "External")
  set_target_properties(hxGrid PROPERTIES FOLDER "External")
  set_target_properties(Newmat PROPERTIES FOLDER "External")
endif()

