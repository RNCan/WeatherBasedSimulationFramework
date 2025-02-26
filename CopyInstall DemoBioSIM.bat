@echo off

cd %~dp0
echo %cd%

if not exist "..\Install" mkdir "..\Install"
if not exist "..\Install\NRCan" mkdir "..\Install\NRCan"
if not exist "..\Install\DemoBioSIM" mkdir "..\Install\DemoBioSIM"

del /?

if exist ".\Examples\Demo BioSIM\tmp" RMDIR /Q /S ".\Examples\Demo BioSIM\tmp
if exist ".\Examples\Demo BioSIM\MapOutput" del /Q ".\Examples\Demo BioSIM\MapOutput\*.*
if exist ".\Examples\Demo BioSIM\Update\tmp" RMDIR /Q /S ".\Examples\Demo BioSIM\Update\tmp


xcopy /Y /S ".\Examples\Demo BioSIM\*.*" "..\Install\DemoBioSIM"


if exist "..\Install\DemoBioSIM.zip" del "..\Install\DemoBioSIM.zip"
.\bin\Releasex64\External\7za.exe a -r "..\Install\DemoBioSIM.zip" "..\Install\DemoBioSIM"





