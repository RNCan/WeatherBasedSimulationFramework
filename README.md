

# WeatherBasedSimulationFramework

The Weather-Based Simulation Framework (WBSF) is a set of C++ classes to help the creation and the execution of weather-driven simulation models.

## Copyright


## License


## Setting up the solution

Follow these steps to set up the solution:

1. Make sure that the environmental variable "VCINSTALLDIR" point to the VC subfolder in your Visual Studio installation in the "Program Files" folder (e.g. C:\Program Files\Microsoft Visual Studio\2022\Professional\VC).
2. Extract the content of the four .7z archives in the repository ./extLibs/ExternalCode subfolder to the ./external subfolder. IMPORTANT: The archive should be extracted directly in the subfolder. Make sure it is not extracted to a default subfolder with the name of the archive (e.g. ./external/gdal/gdal/). The expected structure is rather ./external/gdal/.
3. Open CMake GUI.
4. Set the "Where is the source code" text field to root of the repository (e.g., C:/Users/JohnDoe/Development/CppProjects/WeatherBasedSimulationFramework).
5. Create a "build" subfolder at the root of the repository.
5. Set the "Where to build the binaries" text field to this "build" subfolder you just created.
6. In the bottom panel, click on the "Configure" button.
7. In the bottom panel, click on the "Generate" button.
8. In the bottom panel, click on the "Open_Project" button. The solution should open in Visual Studio.
9. In Visual Studio, expand the "CMakePredefinedTargets" in the upper panel on the right-hand side. You should see an "ALL_BUILD" option.
10. Right-click on the "ALL_BUILD" option and select "Build" to build the solution. 
11. The unit tests can be run by clicking on Test menu item in the menu bar and selecting the "Run All Test" option.
