cmake_minimum_required(VERSION 3.24)

project(WeatherBasedSimulation)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src/Geomatic)
add_subdirectory(src/Basic)
add_subdirectory(src/FileManager)
add_subdirectory(src/ModelBase)
add_subdirectory(src/RangerLib)
add_subdirectory(src/Simulation)
add_subdirectory(src/UI)
add_subdirectory(test)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_target_properties(Geomatic PROPERTIES FOLDER "wbs")  
  set_target_properties(Basic PROPERTIES FOLDER "wbs")  
  set_target_properties(FileManager PROPERTIES FOLDER "wbs")  
  set_target_properties(ModelBase PROPERTIES FOLDER "wbs")  
  set_target_properties(RangerLib PROPERTIES FOLDER "wbs")  
  set_target_properties(Simulation PROPERTIES FOLDER "wbs")  
  set_target_properties(wbsUI PROPERTIES FOLDER "wbs")  
  set_target_properties(WBSF_UnitTest PROPERTIES FOLDER "wbs")  
endif()

