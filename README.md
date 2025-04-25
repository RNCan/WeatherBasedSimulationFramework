[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) 
[![CMake on a single platform](https://github.com/RNCan/WeatherBasedSimulationFramework/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/RNCan/WeatherBasedSimulationFramework/actions/workflows/build-and-test.yml)

# WeatherBasedSimulationFramework

The Weather-Based Simulation Framework (WBSF) is a set of C++ classes to help the creation and the execution of weather-driven simulation models.

## Copyright


## License

This software is licensed under the MIT license. 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


## Setting up the solution

Follow these steps to set up the solution:

1. Make sure that the environmental variable "VCINSTALLDIR" points to the "VC" subfolder in your Visual Studio installation in the "Program Files" folder (e.g., C:\Program Files\Microsoft Visual Studio\2022\Professional\VC).
2. Extract the content of the four .7z archives in the ./extLibs/ExternalCode subfolder of the repository to the ./external subfolder. IMPORTANT: The archives should be extracted directly in the subfolder. Make sure they are not extracted to default subfolders with the name of the archive (e.g. ./external/gdal/gdal/). The expected structure is rather ./external/gdal/.
3. Open CMake GUI.
4. Set the "Where is the source code" text field to the root of the repository (e.g., C:/Users/JohnDoe/Development/CppProjects/WeatherBasedSimulationFramework).
5. Create a "build" subfolder at the root of the repository.
5. Set the "Where to build the binaries" text field to this "build" subfolder you just created.
6. In the bottom panel, click on the "Configure" button.
7. In the bottom panel, click on the "Generate" button.
8. In the bottom panel, click on the "Open_Project" button. The solution should open in Visual Studio.
9. In Visual Studio, expand the "CMakePredefinedTargets" in the upper panel on the right-hand side. You should see an "ALL_BUILD" option.
10. Right-click on the "ALL_BUILD" option and select "Build" to build the solution. 
11. The unit tests can be run by clicking on Test menu item in the menu bar and selecting the "Run All Test" option.
