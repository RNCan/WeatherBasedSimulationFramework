@echo off

cd %~dp0
echo %cd%

mkdir "..\Install"
mkdir "..\Install\NRCan"
mkdir "..\Install\NRCan\External"
mkdir "..\Install\NRCan\Models"
mkdir "..\Install\NRCan\Layers"
mkdir "..\Install\NRCan\Palette"


copy /Y ".\bin\Releasex64\BioSIM11.exe" "..\Install\NRCan\BioSIM11.exe"
copy /Y ".\bin\Releasex64\HourlyEditor.exe" "..\Install\NRCan\HourlyEditor.exe"
copy /Y ".\bin\Releasex64\DailyEditor.exe" "..\Install\NRCan\DailyEditor.exe"
copy /Y ".\bin\Releasex64\NormalsEditor.exe" "..\Install\NRCan\NormalsEditor.exe"
copy /Y ".\bin\Releasex64\WeatherUpdater.exe" "..\Install\NRCan\WeatherUpdater.exe"
copy /Y ".\bin\Releasex64\MatchStation.exe" "..\Install\NRCan\MatchStation.exe"
copy /Y ".\bin\Releasex64\TDate.exe" "..\Install\NRCan\TDate.exe"
copy /Y ".\bin\Releasex64\ShowMap.exe" "..\Install\NRCan\ShowMap.exe"
copy /Y ".\bin\Releasex64\ShowMap.exe.manifest" "..\Install\NRCan\ShowMap.exe.manifest"
copy /Y ".\bin\Releasex64\vcomp140.dll" "..\Install\NRCan\vcomp140.dll"
copy /Y ".\bin\Releasex64\gdal19.dll" "..\Install\NRCan\gdal19.dll"

copy /Y ".\bin\Releasex64\Models\AllenWave (Hourly).mdl" "..\Install\NRCan\Models\AllenWave (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\AllenWave.dll" "..\Install\NRCan\Models\AllenWave.dll"
copy /Y ".\bin\Releasex64\Models\AprocerosLeucopoda.mdl" "..\Install\NRCan\Models\AprocerosLeucopoda.mdl"
copy /Y ".\bin\Releasex64\Models\AprocerosLeucopoda.dll" "..\Install\NRCan\Models\AprocerosLeucopoda.dll"


copy /Y ".\bin\Releasex64\Models\ASCE-ETc (Daily).mdl" "..\Install\NRCan\Models\ASCE-ETc (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\ASCE-ETc.dll" "..\Install\NRCan\Models\ASCE-ETc.dll"
copy /Y ".\bin\Releasex64\Models\ASCE-ETcEx (Daily).mdl" "..\Install\NRCan\Models\ASCE-ETcEx (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\ASCE-ETsz (Daily).mdl" "..\Install\NRCan\Models\ASCE-ETsz (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\ASCE-ETsz (Hourly).mdl" "..\Install\NRCan\Models\ASCE-ETsz (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\ASCE-ETsz.dll" "..\Install\NRCan\Models\ASCE-ETsz.dll"
copy /Y ".\bin\Releasex64\Models\ASCE-ET2005.pdf" "..\Install\NRCan\Models\ASCE-ET2005.pdf"

::copy /Y ".\bin\Releasex64\Models\Biophysical site indices (Ecoloap).mdl" "..\Install\NRCan\Models\Biophysical site indices (Ecoloap).mdl"
::copy /Y ".\bin\Releasex64\Models\Biophysical site indices (Ecoloap).dll" "..\Install\NRCan\Models\Biophysical site indices (Ecoloap).dll"
copy /Y ".\bin\Releasex64\Models\SiteIndexClimate.dll" "..\Install\NRCan\Models\SiteIndexClimate.dll"
copy /Y ".\bin\Releasex64\Models\SiteIndexClimate.mdl" "..\Install\NRCan\Models\SiteIndexClimate.mdl"
::copy /Y ".\bin\Releasex64\Models\BudBurst.mdl" "..\Install\NRCan\Models\BudBurst.mdl"
::copy /Y ".\bin\Releasex64\Models\BudBurst.dll" "..\Install\NRCan\Models\BudBurst.dll"

copy /Y ".\bin\Releasex64\Models\BlueStainIndex.mdl" "..\Install\NRCan\Models\BlueStainIndex.mdl"
copy /Y ".\bin\Releasex64\Models\BlueStainIndex.dll" "..\Install\NRCan\Models\BlueStainIndex.dll"
copy /Y ".\bin\Releasex64\Models\BlueStainVariables.mdl" "..\Install\NRCan\Models\BlueStainVariables.mdl"
copy /Y ".\bin\Releasex64\Models\BlueStainVariables.dll" "..\Install\NRCan\Models\BlueStainVariables.dll"

copy /Y ".\bin\Releasex64\Models\CCBio (Monthly).mdl"       "..\Install\NRCan\Models\CCBio (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\CCBio.dll"                 "..\Install\NRCan\Models\CCBio.dll"
copy /Y ".\bin\Releasex64\Models\CCBio (Annual).mdl"        "..\Install\NRCan\Models\CCBio (Annual).mdl"

copy /Y ".\bin\Releasex64\Models\Climatic (Hourly).mdl"       "..\Install\NRCan\Models\Climatic (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\Climatic (Daily).mdl"        "..\Install\NRCan\Models\Climatic (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\Climatic (Monthly).mdl"      "..\Install\NRCan\Models\Climatic (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Climatic (Annual).mdl"       "..\Install\NRCan\Models\Climatic (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\ClimaticEx (Daily).mdl"      "..\Install\NRCan\Models\ClimaticEx (Daily).mdl"    
copy /Y ".\bin\Releasex64\Models\ClimaticEx (Hourly).mdl"     "..\Install\NRCan\Models\ClimaticEx (Hourly).mdl"   
copy /Y ".\bin\Releasex64\Models\Climatic.dll"                "..\Install\NRCan\Models\Climatic.dll"
copy /Y ".\bin\Releasex64\Models\ClimaticWind (Annual).mdl"   "..\Install\NRCan\Models\ClimaticWind (Annual).mdl" 
copy /Y ".\bin\Releasex64\Models\ClimaticWind (Monthly).mdl"  "..\Install\NRCan\Models\ClimaticWind (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\ClimaticWind.dll"            "..\Install\NRCan\Models\ClimaticWind.dll"          

copy /Y ".\bin\Releasex64\Models\Climdex (Annual).mdl"      "..\Install\NRCan\Models\Climdex (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Climdex (Monthly).mdl"     "..\Install\NRCan\Models\Climdex (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Climdex.dll"               "..\Install\NRCan\Models\Climdex.dll"
copy /Y ".\bin\Releasex64\Models\Climate Moisture Index (Monthly).mdl" "..\Install\NRCan\Models\Climate Moisture Index (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Climate Mosture Index (Annual).mdl" "..\Install\NRCan\Models\Climate Mosture Index (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Climate Moisture Index.dll" "..\Install\NRCan\Models\Climate Moisture Index.dll"
copy /Y ".\bin\Releasex64\Models\ClimaticQc (Annual).mdl"   "..\Install\NRCan\Models\ClimaticQc (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\ClimaticQc.dll"            "..\Install\NRCan\Models\ClimaticQc.dll"

copy /Y ".\bin\Releasex64\Models\CornHeatUnits.mdl" "..\Install\NRCan\Models\CornHeatUnits.mdl"
copy /Y ".\bin\Releasex64\Models\CornHeatUnits.dll" "..\Install\NRCan\Models\CornHeatUnits.dll"

copy /Y ".\bin\Releasex64\Models\Daily vs Hourly (H 2 D).mdl"    "..\Install\NRCan\Models\Daily vs Hourly (H 2 D).mdl"
copy /Y ".\bin\Releasex64\Models\Daily vs Hourly (D 2 H).mdl"    "..\Install\NRCan\Models\Daily vs Hourly (D 2 H).mdl"
copy /Y ".\bin\Releasex64\Models\Daily vs Hourly.dll"    "..\Install\NRCan\Models\Daily vs Hourly.dll"
copy /Y ".\bin\Releasex64\Models\Daily vs Normals.mdl" "..\Install\NRCan\Models\Daily vs Normals.mdl"
copy /Y ".\bin\Releasex64\Models\Daily vs Normals.dll" "..\Install\NRCan\Models\Daily vs Normals.dll"


copy /Y ".\bin\Releasex64\Models\DegreeDay (Annual).mdl"    "..\Install\NRCan\Models\DegreeDay (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\DegreeDay (Monthly).mdl"   "..\Install\NRCan\Models\DegreeDay (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\DegreeDay (Daily).mdl"     "..\Install\NRCan\Models\DegreeDay (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\DegreeDay.dll"             "..\Install\NRCan\Models\DegreeDay.dll"
copy /Y ".\bin\Releasex64\Models\DegreeDay.pdf"             "..\Install\NRCan\Models\DegreeDay.pdf"
copy /Y ".\bin\Releasex64\Models\DegreeHour (Hourly).mdl"   "..\Install\NRCan\Models\DegreeHour (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\DegreeHour.dll"            "..\Install\NRCan\Models\DegreeHour.dll"


copy /Y ".\bin\Releasex64\Models\EmeraldAshBorer.mdl"  "..\Install\NRCan\Models\EmeraldAshBorer.mdl"
copy /Y ".\bin\Releasex64\Models\EmeraldAshBorer.dll"  "..\Install\NRCan\Models\EmeraldAshBorer.dll"
copy /Y ".\bin\Releasex64\Models\EmeraldAshBorerColdHardiness (Annual).mdl"  "..\Install\NRCan\Models\EmeraldAshBorerColdHardiness (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\EmeraldAshBorerColdHardiness.dll"  "..\Install\NRCan\Models\EmeraldAshBorerColdHardiness.dll"
copy /Y ".\bin\Releasex64\Models\EuropeanElmScale.mdl" "..\Install\NRCan\Models\EuropeanElmScale.mdl"
copy /Y ".\bin\Releasex64\Models\EuropeanElmScale.dll" "..\Install\NRCan\Models\EuropeanElmScale.dll"


copy /Y ".\bin\Releasex64\Models\FallCankerworms.mdl" "..\Install\NRCan\Models\FallCankerworms.mdl"
copy /Y ".\bin\Releasex64\Models\FallCankerworms.dll" "..\Install\NRCan\Models\FallCankerworms.dll"
copy /Y ".\bin\Releasex64\Models\SpringCankerworms.mdl" "..\Install\NRCan\Models\SpringCankerworms.mdl"
copy /Y ".\bin\Releasex64\Models\SpringCankerworms.dll" "..\Install\NRCan\Models\SpringCankerworms.dll"

copy /Y ".\bin\Releasex64\Models\ForestTentCaterpillar.mdl" "..\Install\NRCan\Models\ForestTentCaterpillar.mdl"
copy /Y ".\bin\Releasex64\Models\ForestTentCaterpillar.dll" "..\Install\NRCan\Models\ForestTentCaterpillar.dll"


copy /Y ".\bin\Releasex64\Models\FBP (Hourly).mdl" "..\Install\NRCan\Models\FBP (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\FBP.dll" "..\Install\NRCan\Models\FBP.dll"
copy /Y ".\bin\Releasex64\Models\FWI (Daily).mdl" "..\Install\NRCan\Models\FWI (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\FWI (Hourly).mdl" "..\Install\NRCan\Models\FWI (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\FWI (Monthly).mdl" "..\Install\NRCan\Models\FWI (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\FWI (Annual).mdl" "..\Install\NRCan\Models\FWI (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\FWI-Fixed (Daily).mdl" "..\Install\NRCan\Models\FWI-Fixed (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\FWI-Fixed (Monthly).mdl" "..\Install\NRCan\Models\FWI-Fixed (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\FWI-Fixed (Annual).mdl" "..\Install\NRCan\Models\FWI-Fixed (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\FWI.dll" "..\Install\NRCan\Models\FWI.dll"
copy /Y ".\bin\Releasex64\Models\FWI Drought Code (Daily).mdl" "..\Install\NRCan\Models\FWI Drought Code (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\FWI Drought Code (Monthly).mdl" "..\Install\NRCan\Models\FWI Drought Code (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\FWI Drought Code-Fixe (Daily).mdl" "..\Install\NRCan\Models\FWI Drought Code-Fixe (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\FWI Drought Code-Fixe (Monthly).mdl" "..\Install\NRCan\Models\FWI Drought Code-Fixe (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\FWI Drought Code.dll" "..\Install\NRCan\Models\FWI Drought Code.dll"
copy /Y ".\bin\Releasex64\Models\SummerMoisture (Monthly).mdl" "..\Install\NRCan\Models\SummerMoisture (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\SummerMoisture.dll" "..\Install\NRCan\Models\SummerMoisture.dll"

copy /Y ".\bin\Releasex64\Models\GrowingSeason.mdl" "..\Install\NRCan\Models\GrowingSeason.mdl"
copy /Y ".\bin\Releasex64\Models\GrowingSeason.dll" "..\Install\NRCan\Models\GrowingSeason.dll"
copy /Y ".\bin\Releasex64\Models\Gypsy Moth Seasonality.dll" "..\Install\NRCan\Models\Gypsy Moth Seasonality.dll"
copy /Y ".\bin\Releasex64\Models\Gypsy Moth Seasonality.mdl" "..\Install\NRCan\Models\Gypsy Moth Seasonality.mdl"
copy /Y ".\bin\Releasex64\Models\Gypsy Moth Stability.dll" "..\Install\NRCan\Models\Gypsy Moth Stability.dll"
copy /Y ".\bin\Releasex64\Models\Gypsy Moth Stability.mdl" "..\Install\NRCan\Models\Gypsy Moth Stability.mdl"

copy /Y ".\bin\Releasex64\Models\HemlockLooper.dll" "..\Install\NRCan\Models\HemlockLooper.dll"
copy /Y ".\bin\Releasex64\Models\HemlockLooper.mdl" "..\Install\NRCan\Models\HemlockLooper.mdl"
copy /Y ".\bin\Releasex64\Models\HemlockLooperRemi.dll" "..\Install\NRCan\Models\HemlockLooperRemi.dll"
copy /Y ".\bin\Releasex64\Models\HemlockLooperRemi.mdl" "..\Install\NRCan\Models\HemlockLooperRemi.mdl"
copy /Y ".\bin\Releasex64\Models\HemlockWoollyAdelgid.dll" "..\Install\NRCan\Models\HemlockWoollyAdelgid.dll"
copy /Y ".\bin\Releasex64\Models\HemlockWoollyAdelgid (Annual).mdl" "..\Install\NRCan\Models\HemlockWoollyAdelgid (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\HemlockWoollyAdelgid (Daily).mdl" "..\Install\NRCan\Models\HemlockWoollyAdelgid (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\HWA Phenology.mdl" "..\Install\NRCan\Models\HWA Phenology.mdl"
copy /Y ".\bin\Releasex64\Models\HWA Phenology.dll" "..\Install\NRCan\Models\HWA Phenology.dll"


copy /Y ".\bin\Releasex64\Models\HourlyGenerator (Hourly).mdl" "..\Install\NRCan\Models\HourlyGenerator (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\HourlyGenerator.dll" "..\Install\NRCan\Models\HourlyGenerator.dll"


copy /Y ".\bin\Releasex64\Models\Insect Development Database II.mdl"  "..\Install\NRCan\Models\Insect Development Database II.mdl" 
copy /Y ".\bin\Releasex64\Models\Insect Development Database III.csv" "..\Install\NRCan\Models\Insect Development Database III.csv"
copy /Y ".\bin\Releasex64\Models\Insect Development Database III.dll" "..\Install\NRCan\Models\Insect Development Database III.dll"
copy /Y ".\bin\Releasex64\Models\Insect Development Database III.mdl" "..\Install\NRCan\Models\Insect Development Database III.mdl"
copy /Y ".\bin\Releasex64\Models\Insect Development Database II.csv"  "..\Install\NRCan\Models\Insect Development Database II.csv" 
copy /Y ".\bin\Releasex64\Models\Insect Development Database II.dll"  "..\Install\NRCan\Models\Insect Development Database II.dll" 
copy /Y ".\bin\Releasex64\Models\Jackpine Budworm.dll" "..\Install\NRCan\Models\Jackpine Budworm.dll"
copy /Y ".\bin\Releasex64\Models\Jackpine Budworm.mdl" "..\Install\NRCan\Models\Jackpine Budworm.mdl"
copy /Y ".\bin\Releasex64\Models\LaricobiusNigrinus.dll" "..\Install\NRCan\Models\LaricobiusNigrinus.dll"
copy /Y ".\bin\Releasex64\Models\LaricobiusNigrinus.mdl" "..\Install\NRCan\Models\LaricobiusNigrinus.mdl"
copy /Y ".\bin\Releasex64\Models\Leucopis.dll" "..\Install\NRCan\Models\Leucopis.dll"
copy /Y ".\bin\Releasex64\Models\Leucopis spp.mdl" "..\Install\NRCan\Models\Leucopis spp.mdl"


copy /Y ".\bin\Releasex64\Models\MPB Cold Tolerance (Daily).mdl" "..\Install\NRCan\Models\MPB Cold Tolerance (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\MPB Cold Tolerance (Annual).mdl" "..\Install\NRCan\Models\MPB Cold Tolerance (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\MPB-ColdTolerance.dll" "..\Install\NRCan\Models\MPB-ColdTolerance.dll"
copy /Y ".\bin\Releasex64\Models\MPB-SLR.dll" "..\Install\NRCan\Models\MPB-SLR.dll"
copy /Y ".\bin\Releasex64\Models\MPB-SLR.mdl" "..\Install\NRCan\Models\MPB-SLR.mdl"

copy /Y ".\bin\Releasex64\Models\ObliqueBandedLeafroller.dll" "..\Install\NRCan\Models\ObliqueBandedLeafroller.dll"
copy /Y ".\bin\Releasex64\Models\ObliqueBandedLeafroller.mdl" "..\Install\NRCan\Models\ObliqueBandedLeafroller.mdl"

copy /Y ".\bin\Releasex64\Models\PlantHardiness.dll" "..\Install\NRCan\Models\PlantHardiness.dll"
copy /Y ".\bin\Releasex64\Models\PlantHardinessCanada.mdl" "..\Install\NRCan\Models\PlantHardinessCanada.mdl"
copy /Y ".\bin\Releasex64\Models\PlantHardinessUSA.mdl" "..\Install\NRCan\Models\PlantHardinessUSA.mdl"


copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration (Annual).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration (Monthly).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration (Daily).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Annual).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration Ex (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Monthly).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration Ex (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Daily).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration Ex (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Hourly).mdl" "..\Install\NRCan\Models\Potential Evapotranspiration Ex (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\Potential Evapotranspiration.dll" "..\Install\NRCan\Models\Potential Evapotranspiration.dll"
copy /Y ".\bin\Releasex64\Models\ReverseDegreeDay (Annual).mdl" "..\Install\NRCan\Models\ReverseDegreeDay (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\ReverseDegreeDay (Overall years).mdl" "..\Install\NRCan\Models\ReverseDegreeDay (Overall years).mdl"
copy /Y ".\bin\Releasex64\Models\ReverseDegreeDay.dll" "..\Install\NRCan\Models\ReverseDegreeDay.dll"
copy /Y ".\bin\Releasex64\Models\ReverseDegreeDay.pdf" "..\Install\NRCan\Models\ReverseDegreeDay.pdf"

copy /Y ".\bin\Releasex64\Models\SnowMelt (Monthly).mdl" "..\Install\NRCan\Models\SnowMelt (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\SnowMelt.dll" "..\Install\NRCan\Models\SnowMelt.dll"

copy /Y ".\bin\Releasex64\Models\Soil Moisture Index (Annual).mdl" "..\Install\NRCan\Models\Soil Moisture Index (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index (Monthly).mdl" "..\Install\NRCan\Models\Soil Moisture Index (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index (Daily).mdl" "..\Install\NRCan\Models\Soil Moisture Index (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index QL(Annual).mdl" "..\Install\NRCan\Models\Soil Moisture Index QL(Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index QL(Monthly).mdl" "..\Install\NRCan\Models\Soil Moisture Index QL(Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index QL(Daily).mdl" "..\Install\NRCan\Models\Soil Moisture Index QL(Daily).mdl"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index QL.dll" "..\Install\NRCan\Models\Soil Moisture Index QL.dll"
copy /Y ".\bin\Releasex64\Models\Soil Moisture Index.dll" "..\Install\NRCan\Models\Soil Moisture Index.dll"


copy /Y ".\bin\Releasex64\Models\Solar.mdl" "..\Install\NRCan\Models\Solar.mdl"
copy /Y ".\bin\Releasex64\Models\Solar.dll" "..\Install\NRCan\Models\Solar.dll"
copy /Y ".\bin\Releasex64\Models\StringFrost.mdl" "..\Install\NRCan\Models\StringFrost.mdl"
copy /Y ".\bin\Releasex64\Models\StringFrost.dll" "..\Install\NRCan\Models\StringFrost.dll"


copy /Y ".\bin\Releasex64\Models\SpruceBeetle.dll" "..\Install\NRCan\Models\SpruceBeetle.dll"
copy /Y ".\bin\Releasex64\Models\SpruceBeetle.mdl" "..\Install\NRCan\Models\SpruceBeetle.mdl"

copy /Y ".\bin\Releasex64\Models\Spruce Budworm Biology.dll" "..\Install\NRCan\Models\Spruce Budworm Biology.dll"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Biology.mdl" "..\Install\NRCan\Models\Spruce Budworm Biology.mdl"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Biology (Annual).mdl" "..\Install\NRCan\Models\Spruce Budworm Biology (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Dispersal.dll" "..\Install\NRCan\Models\Spruce Budworm Dispersal.dll"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Dispersal.mdl" "..\Install\NRCan\Models\Spruce Budworm Dispersal.mdl"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Laboratory.mdl" "..\Install\NRCan\Models\Spruce Budworm Laboratory.mdl"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Laboratory.dll" "..\Install\NRCan\Models\Spruce Budworm Laboratory.dll"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Manitoba.mdl" "..\Install\NRCan\Models\Spruce Budworm Manitoba.mdl"
copy /Y ".\bin\Releasex64\Models\Spruce Budworm Manitoba.dll" "..\Install\NRCan\Models\Spruce Budworm Manitoba.dll"

copy /Y ".\bin\Releasex64\Models\Standardised Precipitation Evapotranspiration Index.mdl" "..\Install\NRCan\Models\Standardised Precipitation Evapotranspiration Index.mdl"
copy /Y ".\bin\Releasex64\Models\Standardised Precipitation Evapotranspiration Index.dll" "..\Install\NRCan\Models\Standardised Precipitation Evapotranspiration Index.dll"
copy /Y ".\bin\Releasex64\Models\Standardised Precipitation Evapotranspiration Index Ex.mdl" "..\Install\NRCan\Models\Standardised Precipitation Evapotranspiration Index Ex.dll"
copy /Y ".\bin\Releasex64\Models\TminTairTmax (Hourly).mdl" "..\Install\NRCan\Models\TminTairTmax (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\TminTairTmax (Daily).mdl" "..\Install\NRCan\Models\TminTairTmax (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\TminTairTmax.dll" "..\Install\NRCan\Models\TminTairTmax.dll"

copy /Y ".\bin\Releasex64\Models\Tranosema-OBL-SBW (daily).mdl" "..\Install\NRCan\Models\Tranosema-OBL-SBW (daily).mdl"
copy /Y ".\bin\Releasex64\Models\Tranosema-OBL-SBW.dll" "..\Install\NRCan\Models\Tranosema-OBL-SBW.dll"

copy /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Annual).mdl" "..\Install\NRCan\Models\VaporPressureDeficit (Annual).mdl"
copy /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Monthly).mdl" "..\Install\NRCan\Models\VaporPressureDeficit (Monthly).mdl"
copy /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Daily).mdl" "..\Install\NRCan\Models\VaporPressureDeficit (Daily).mdl"
copy /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Hourly).mdl" "..\Install\NRCan\Models\VaporPressureDeficit (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\VaporPressureDeficit.dll" "..\Install\NRCan\Models\VaporPressureDeficit.dll"

copy /Y ".\bin\Releasex64\Models\Western Spruce Budworm (annual).mdl" "..\Install\NRCan\Models\Western Spruce Budworm (annual).mdl"
copy /Y ".\bin\Releasex64\Models\Western Spruce Budworm.dll" "..\Install\NRCan\Models\Western Spruce Budworm.dll"
copy /Y ".\bin\Releasex64\Models\Western Spruce Budworm.mdl" "..\Install\NRCan\Models\Western Spruce Budworm.mdl"
copy /Y ".\bin\Releasex64\Models\WetnessDuration (Hourly).mdl" "..\Install\NRCan\Models\WetnessDuration (Hourly).mdl"
copy /Y ".\bin\Releasex64\Models\WetnessDuration.dll" "..\Install\NRCan\Models\WetnessDuration.dll"
copy /Y ".\bin\Releasex64\Models\WhitemarkedTussockMoth.mdl" "..\Install\NRCan\Models\WhitemarkedTussockMoth.mdl"
copy /Y ".\bin\Releasex64\Models\WhitemarkedTussockMoth.dll" "..\Install\NRCan\Models\WhitemarkedTussockMoth.dll"
copy /Y ".\bin\Releasex64\Models\WhitePineWeevil.mdl" "..\Install\NRCan\Models\WhitePineWeevil.mdl"
copy /Y ".\bin\Releasex64\Models\WhitePineWeevil.dll" "..\Install\NRCan\Models\WhitePineWeevil.dll"
copy /Y ".\bin\Releasex64\Models\WinterThaw.mdl" "..\Install\NRCan\Models\WinterThaw.mdl"
copy /Y ".\bin\Releasex64\Models\WinterThaw.dll" "..\Install\NRCan\Models\WinterThaw.dll"



copy /Y ".\bin\Releasex64\Models\Yellowheaded Spruce Sawfly.dll" "..\Install\NRCan\Models\Yellowheaded Spruce Sawfly.dll"
copy /Y ".\bin\Releasex64\Models\Yellowheaded Spruce Sawfly.mdl" "..\Install\NRCan\Models\Yellowheaded Spruce Sawfly.mdl"
copy /Y ".\bin\Releasex64\Models\Yellowheaded Spruce Sawfly.pdf" "..\Install\NRCan\Models\Yellowheaded Spruce Sawfly.pdf"


xcopy /Y /S ".\bin\Releasex64\External\*.*" "..\Install\NRCan\External\*.*"
xcopy /Y /S ".\bin\Releasex64\Layers\*.*" "..\Install\NRCan\Layers\*.*"
xcopy /Y /S ".\bin\Releasex64\Palette\*.*" "..\Install\NRCan\Palette\*.*"
xcopy /Y /S ".\bin\Releasex64\zoneinfo\*.*" "..\Install\NRCan\zoneinfo\*.*"

::copy /Y ".\bin\Releasex64\External\FTPTransfer.exe" "..\Install\NRCan\External\FTPTransfer.exe"

del "..\Install\BioSIM11_x_x.7z"
.\bin\Releasex64\External\7za.exe a -r "..\Install\BioSIM11_x_x.7z" "..\Install\NRCan"
del "..\Install\BioSIM11_x_x.zip"
.\bin\Releasex64\External\7za.exe a -r "..\Install\BioSIM11_x_x.zip" "..\Install\NRCan"

REM ::xcopy /Y "..\Install\NRCan.7z" "ftp://ftp.cfl.scf.rncan.gc.ca/regniere/software/BioSIM/BioSIM11_x_x.7z"



