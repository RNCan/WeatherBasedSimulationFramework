echo on

del /S /Q *.sdf,*.sbr,*.ilk,*.pdb,*.aps 
RMDIR /S /Q .\GeomaticTools\Releasex64
RMDIR /S /Q .\GeomaticTools\Debugx64
RMDIR /S /Q .\wbs\msvc\Releasex64
RMDIR /S /Q .\wbs\msvc\Debugx64
RMDIR /S /Q .\Models\Releasex64
RMDIR /S /Q .\Models\Debugx64
RMDIR /S /Q .\wbsTools\BioSIM\Releasex64
RMDIR /S /Q .\wbsTools\BioSIM\Debugx64
RMDIR /S /Q .\wbsTools\DailyEditor\Releasex64
RMDIR /S /Q .\wbsTools\DailyEditor\Debugx64
RMDIR /S /Q .\wbsTools\HourlyEditor\Releasex64
RMDIR /S /Q .\wbsTools\HourlyEditor\Debugx64
RMDIR /S /Q .\wbsTools\NormalsEditor\Releasex64
RMDIR /S /Q .\wbsTools\NormalsEditor\Debugx64
RMDIR /S /Q .\wbsTools\WeatherUpdater\Releasex64
RMDIR /S /Q .\wbsTools\WeatherUpdater\Debugx64
RMDIR /S /Q .\wbsTools\xml2csv\Releasex64
RMDIR /S /Q .\wbsTools\xml2csv\Debugx64

del .\wbs\lib64\wbsUI.lib
del .\wbs\lib64\wbsUI_d.lib
del .\wbs\lib64\wbs.lib
del .\wbs\lib64\wbs_d.lib

pause


