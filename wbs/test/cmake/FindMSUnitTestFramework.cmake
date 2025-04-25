add_library(MSUnitTestFramework::MSUnitTestFramework SHARED IMPORTED)
set_target_properties(MSUnitTestFramework::MSUnitTestFramework PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{VCInstallDir}/Auxiliary/VS/UnitTest/include"
  IMPORTED_IMPLIB "$ENV{VCInstallDir}/Auxiliary/VS/UnitTest/lib/x86/Microsoft.VisualStudio.TestTools.CppUnitTestFramework.lib"
)
if (DEFINED ENV{VCInstallDir})
	message(STATUS "Found VCInstallDir to be $ENV{VCInstallDir}")
	set(MSUnitTestFramework_FOUND TRUE)
else()
	message(FATAL_ERROR "VCInstallDir not found.  Please set VCINSTALLDIR environment variable correctly (see readme) and relaunch cmake")
	set(MSUnitTestFramework_FOUND FALSE)
endif()

