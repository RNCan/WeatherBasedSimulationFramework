# WeatherBasedSimulationFramework

The Weather-Based Simulation Framework (WBSF) is a set of C++ classes to help the creation and the execution of weather-driven simulation models.

1- Open external solution
	- Set to release instead of debug
	- Build solution
2- Open wbs solution
	- Set to release instead of debug
	- Build solution
3- Open wbsModels/AllModels.sln 
	- Set to release instead of debug
	- Build solution
4- Download ExternalExec.7z from googledrive and extract in ./bin/Releasex64/External

5- Open ./wbsTools/BioSIM/BioSIM.sln	
	- Set to release instead of debug then build
	- Build solution
6- Open BioSIM project properties in BioSIM solution	
	- Build solution
	- Click on Debugging in the left-hand side panel
	- In the first combobox, select All Configurations
	- In the second combobox, select Active(x64)
	- In the main panel, copy and paste the following line alongside the "Command" propery:
		..\..\bin\$(Configuration)$(Platform)\$(TargetName)$(TargetExt)

