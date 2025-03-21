add_library(MSUnitTestFramework::MSUnitTestFramework SHARED IMPORTED)
set_target_properties(MSUnitTestFramework::MSUnitTestFramework PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "$ENV{VCInstallDir}/Auxiliary/VS/UnitTest/include"
  IMPORTED_IMPLIB "$ENV{VCInstallDir}/Auxiliary/VS/UnitTest/lib/x86/Microsoft.VisualStudio.TestTools.CppUnitTestFramework.lib"
)
message("Found VCInstallDir to be $ENV{VCInstallDir}")
set(MSUnitTestFramework_FOUND TRUE)
