@echo off


xcopy /Y "D:\project\bin\Releasex64\BioSIM11.exe" "D:\NRCan\BioSIM11_x_x\BioSIM11.exe"
xcopy /Y "D:\project\wbsTools\BioSIM\Releasex64\BioSIM11.pdb" "D:\NRCan\BioSIM11_x_x\BioSIM11.pdb"
xcopy /Y "D:\project\wbsTools\BioSIM\Releasex64\BioSIM11.map" "D:\NRCan\BioSIM11_x_x\BioSIM11.map"
xcopy /Y "D:\project\bin\Releasex64\External\BioSIM11Frc.dll" "D:\NRCan\BioSIM11_x_x\External\BioSIM11Frc.dll"

xcopy /Y "D:\project\bin\Releasex64\HourlyEditor.exe" "D:\NRCan\BioSIM11_x_x\HourlyEditor.exe"
xcopy /Y "D:\project\bin\Releasex64\External\HourlyEditorFrc.dll" "D:\NRCan\BioSIM11_x_x\External\HourlyEditorFrc.dll"
xcopy /Y "D:\project\bin\Releasex64\DailyEditor.exe" "D:\NRCan\BioSIM11_x_x\DailyEditor.exe"
xcopy /Y "D:\project\bin\Releasex64\External\DailyEditorFrc.dll" "D:\NRCan\BioSIM11_x_x\External\DailyEditorFrc.dll"
xcopy /Y "D:\project\bin\Releasex64\NormalsEditor.exe" "D:\NRCan\BioSIM11_x_x\NormalsEditor.exe"
xcopy /Y "D:\project\bin\Releasex64\External\NormalsEditorFrc.dll" "D:\NRCan\BioSIM11_x_x\External\NormalsEditorFrc.dll"
xcopy /Y "D:\project\bin\Releasex64\WeatherUpdater.exe" "D:\NRCan\BioSIM11_x_x\WeatherUpdater.exe"
xcopy /Y "D:\project\bin\Releasex64\External\WeatherUpdaterFrc.dll" "D:\NRCan\BioSIM11_x_x\External\WeatherUpdaterFrc.dll"
xcopy /Y "D:\project\bin\Releasex64\MatchStation.exe" "D:\NRCan\BioSIM11_x_x\MatchStation.exe"
xcopy /Y "D:\project\bin\Releasex64\External\MatchStationFrc.dll" "D:\NRCan\BioSIM11_x_x\External\MatchStationFrc.dll"
xcopy /Y /S "D:\project\bin\Releasex64\Models\*.*" "D:\NRCan\BioSIM11_x_x\Models\*.*"

xcopy /Y "D:\project\bin\Releasex64\External\FTPTransfer.exe" "D:\NRCan\BioSIM11_x_x\External\FTPTransfer.exe"

del "D:\NRCan\BioSIM11_x_x.7z"
D:\project\bin\Releasex64\External\7z.exe a -r "D:\NRCan\BioSIM11_x_x.7z" "D:\NRCan\BioSIM11_x_x\*.*"
del "D:\NRCan\BioSIM11_x_x.zip"
D:\project\bin\Releasex64\External\7z.exe a -r "D:\NRCan\BioSIM11_x_x.zip" "D:\NRCan\BioSIM11_x_x\*.*"

::xcopy /Y "D:\NRCan\BioSIM11_x_x.7z" "ftp://ftp.cfl.scf.rncan.gc.ca/regniere/software/BioSIM/BioSIM11_x_x.7z"
