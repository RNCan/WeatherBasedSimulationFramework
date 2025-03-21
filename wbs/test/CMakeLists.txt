cmake_minimum_required(VERSION 3.24)

project(WBSF_UnitTest)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)

find_package(MSUnitTestFramework REQUIRED)
find_program(VSTest_EXECUTABLE NAMES vstest.console.exe PATHS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/Extensions/TestPlatform/" REQUIRED)

# Use the GLOB command to get all the source files
file(GLOB SOURCE_FILES ./*.h ./*.cpp)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${EXTERNAL_PROJECTS_DIR}  
)

# Build as static library
add_library(WBSF_UnitTest SHARED
  ${SOURCE_FILES}
)

target_link_libraries(WBSF_UnitTest Basic MSUnitTestFramework::MSUnitTestFramework)

add_test(NAME WBSF_UnitTest COMMAND "${VSTest_EXECUTABLE}" "$<TARGET_FILE:WBSF_UnitTest>")
