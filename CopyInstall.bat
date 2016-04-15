@echo off

cd %~dp0
echo %cd%

mkdir "..\NRCan"
mkdir "..\Install\NRCan"
mkdir "..\Install\NRCan\External"
mkdir "..\Install\NRCan\Models"
mkdir "..\Install\NRCan\Layers"
::mkdir "..\Install\NRCan\Maps"
mkdir "..\Install\NRCan\Palette"


copy /Y ".\bin\Releasex64\BioSIM11.exe" "..\Install\NRCan\BioSIM11.exe"
copy /Y ".\bin\Releasex64\External\BioSIM11Frc.dll" "..\Install\NRCan\External\BioSIM11Frc.dll"

copy /Y ".\bin\Releasex64\HourlyEditor.exe" "..\Install\NRCan\HourlyEditor.exe"
copy /Y ".\bin\Releasex64\DailyEditor.exe" "..\Install\NRCan\DailyEditor.exe"
copy /Y ".\bin\Releasex64\NormalsEditor.exe" "..\Install\NRCan\NormalsEditor.exe"
copy /Y ".\bin\Releasex64\WeatherUpdater.exe" "..\Install\NRCan\WeatherUpdater.exe"
copy /Y ".\bin\Releasex64\MatchStation.exe" "..\Install\NRCan\MatchStation.exe"
copy /Y ".\bin\Releasex64\ShowMap.exe" "..\Install\NRCan\ShowMap.exe"
copy /Y ".\bin\Releasex64\ShowMap.exe.manifest" "..\Install\NRCan\ShowMap.exe.manifest"
copy /Y ".\bin\Releasex64\vcomp120.dll" "..\Install\NRCan\vcomp120.dll"
copy /Y ".\bin\Releasex64\gdal19.dll" "..\Install\NRCan\gdal19.dll"


xcopy /Y /S ".\bin\Releasex64\Models\*.*" "..\Install\NRCan\Models\*.*"
xcopy /Y /S ".\bin\Releasex64\External\*.*" "..\Install\NRCan\External\*.*"
xcopy /Y /S ".\bin\Releasex64\Layers\*.*" "..\Install\NRCan\Layers\*.*"
xcopy /Y /S ".\bin\Releasex64\Palette\*.*" "..\Install\NRCan\Palette\*.*"

::copy /Y ".\bin\Releasex64\External\FTPTransfer.exe" "..\Install\NRCan\External\FTPTransfer.exe"

del "..\Install\BioSIM11_x_x.7z"
.\bin\Releasex64\External\7z.exe a -r "..\Install\BioSIM11_x_x.7z" "..\Install\NRCan"
del "..\Install\NRCan.zip"
.\bin\Releasex64\External\7z.exe a -r "..\Install\BioSIM11_x_x.zip" "..\Install\NRCan"

REM ::xcopy /Y "..\Install\NRCan.7z" "ftp://ftp.cfl.scf.rncan.gc.ca/regniere/software/BioSIM/BioSIM11_x_x.7z"

