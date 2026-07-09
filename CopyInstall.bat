@echo on

cd %~dp0
echo %cd%

if not exist "..\Install" mkdir "..\Install"
if not exist "..\Install\NRCan" mkdir "..\Install\NRCan"
if not exist "..\Install\NRCan\bin" mkdir "..\Install\NRCan\bin"
if not exist "..\Install\NRCan\Models" mkdir "..\Install\NRCan\Models"
if not exist "..\Install\NRCan\Layers" mkdir "..\Install\NRCan\Layers"
if not exist "..\Install\NRCan\Palette" mkdir "..\Install\NRCan\Palette"



@echo off

xcopy /Q /Y ".\bin\Releasex64\Models\AllenWave (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\AllenWave.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\AprocerosLeucopoda.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\AprocerosLeucopoda.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ETc (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ETc.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ETcEx (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ETsz (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ETsz (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ETsz.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ASCE-ET2005.pdf" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\BudBurst.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\BudBurst.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\BudBurstSaintAmant.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\BudBurstSaintAmant.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\BlueStainIndex.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\BlueStainIndex.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\BlueStainVariables.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\BlueStainVariables.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\CCBio (Monthly).mdl"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\CCBio.dll"                 "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\CCBio (Annual).mdl"        "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Climatic (Hourly).mdl"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climatic (Daily).mdl"        "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climatic (Monthly).mdl"      "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climatic (Annual).mdl"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticEx (Daily).mdl"      "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticEx (Hourly).mdl"     "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climatic.dll"                "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticWind (Annual).mdl"   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticWind (Monthly).mdl"  "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticWind.dll"            "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Climdex (Annual).mdl"      "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climdex (Monthly).mdl"     "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climdex.dll"               "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climate Moisture Index (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climate Mosture Index (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Climate Moisture Index.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticQc (Annual).mdl"   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ClimaticQc.dll"            "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\CornHeatUnits.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\CornHeatUnits.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\CreateNormalsDatabase.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\CreateBioSIMDatabase.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Daily vs Hourly (H 2 D).mdl"    "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Daily vs Hourly (D 2 H).mdl"    "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Daily vs Hourly.dll"    "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Daily vs Normals.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Daily vs Normals.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\DegreeDay (Annual).mdl"    "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\DegreeDay (Monthly).mdl"   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\DegreeDay (Daily).mdl"     "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\DegreeDay.dll"             "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\DegreeDay.pdf"             "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\DegreeHour (Hourly).mdl"   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\DegreeHour.dll"            "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\EmeraldAshBorer.mdl"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\EmeraldAshBorer.dll"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\EmeraldAshBorerColdHardiness (Annual).mdl"  "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\EmeraldAshBorerColdHardiness.dll"  "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\EuropeanElmScale.mdl"      "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\EuropeanElmScale.dll"      "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\FallCankerworms.mdl"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FallCankerworms.dll"       "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SpringCankerworms.mdl"     "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SpringCankerworms.dll"     "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\ForestTentCaterpillar.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ForestTentCaterpillar.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\FBP (Hourly).mdl"          "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FBP.dll"                   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI (Daily).mdl"           "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI (Hourly).mdl"          "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI (Monthly).mdl"         "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI (Annual).mdl"          "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI-Fixed (Daily).mdl"     "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI-Fixed (Monthly).mdl"   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI-Fixed (Annual).mdl"    "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI.dll"                   "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI Drought Code (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI Drought Code (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI Drought Code-Fixe (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI Drought Code-Fixe (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\FWI Drought Code.dll"      "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SummerMoisture (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SummerMoisture.dll"        "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\GrowingSeason.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\GrowingSeason.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Gypsy Moth Seasonality.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Gypsy Moth Seasonality.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Gypsy Moth Stability.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Gypsy Moth Stability.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\HemlockLooper.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HemlockLooper.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HemlockLooperRemi.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HemlockLooperRemi.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HemlockWoollyAdelgid.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HemlockWoollyAdelgid (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HemlockWoollyAdelgid (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HWA Phenology.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HWA Phenology.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\HourlyGenerator (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\HourlyGenerator.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\Insect Development Database II.mdl"  "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Insect Development Database III.csv" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Insect Development Database III.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Insect Development Database III.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Insect Development Database II.csv"  "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Insect Development Database II.dll"  "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Jackpine Budworm.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Jackpine Budworm.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\JapaneseBeetle.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\JapaneseBeetle.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\LaricobiusNigrinus.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LaricobiusNigrinus.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LaricobiusNigrinus_LarvalSampling.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\LaricobiusOsakensis.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LaricobiusOsakensis.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Leucotaraxis spp.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Leucotaraxis spp.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LeucotaraxisArgenticollis.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LeucotaraxisArgenticollis.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LeucotaraxisPiniperda.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\LeucotaraxisPiniperda.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\MPB Cold Tolerance (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\MPB Cold Tolerance (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\MPB-ColdTolerance.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\MPB-SLR.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\MPB-SLR.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\ObliqueBandedLeafroller.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ObliqueBandedLeafroller.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\PlantHardiness.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\PlantHardinessCanada.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\PlantHardinessUSA.mdl" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration Ex (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Potential Evapotranspiration.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ReverseDegreeDay (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ReverseDegreeDay (Overall years).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ReverseDegreeDay.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\ReverseDegreeDay.pdf" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\SnowMelt (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SnowMelt.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index QL(Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index QL(Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index QL(Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index QL.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Soil Moisture Index.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SoilTemperature.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SoilTemperature.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Solar.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Solar.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SpringFrost.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SpringFrost.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SiteIndexClimate.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SiteIndexClimate.mdl" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\SpruceBeetle.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\SpruceBeetle.mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Biology.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Biology.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Biology (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Dispersal.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Dispersal.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Laboratory.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Laboratory.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Manitoba.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Spruce Budworm Manitoba.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\StdPrcpETIndex.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\StdPrcpETIndex.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\StdPrcpETIndexEx.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\TminTairTmax (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\TminTairTmax (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\TminTairTmax.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Tranosema-OBL-SBW (daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Tranosema-OBL-SBW.dll" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Monthly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Daily).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\VaporPressureDeficit (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\VaporPressureDeficit.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\WaterBalance.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WaterBalance (Monthly).mdl" "..\Install\NRCan\Models"

xcopy /Q /Y ".\bin\Releasex64\Models\Western Spruce Budworm (annual).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Western Spruce Budworm.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Western Spruce Budworm.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WetnessDuration (Hourly).mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WetnessDuration.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WhitemarkedTussockMoth.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WhitemarkedTussockMoth.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WhitePineWeevil.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WhitePineWeevil.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WinterThaw.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WinterThaw.dll" "..\Install\NRCan\Models"


xcopy /Q /Y ".\bin\Releasex64\Models\WorldClimVars.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\WorldClimVars.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Yellowheaded Spruce Sawfly.dll" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Yellowheaded Spruce Sawfly.mdl" "..\Install\NRCan\Models"
xcopy /Q /Y ".\bin\Releasex64\Models\Yellowheaded Spruce Sawfly.pdf" "..\Install\NRCan\Models"


@echo on


xcopy /Y /S ".\bin\Releasex64\bin\*.*" "..\Install\NRCan\bin\*.*"
xcopy /Y /S ".\bin\Releasex64\Layers\*.*" "..\Install\NRCan\Layers\*.*"
xcopy /Y /S ".\bin\Releasex64\Palette\*.*" "..\Install\NRCan\Palette\*.*"
xcopy /Y /S ".\bin\Releasex64\zoneinfo\*.*" "..\Install\NRCan\zoneinfo\*.*"




if exist "..\Install\BioSIM11_x_x.7z" del "..\Install\BioSIM11_x_x.7z"
.\bin\Releasex64\bin\7za.exe a -r "..\Install\BioSIM11_x_x.7z" "..\Install\NRCan"
if exist "..\Install\BioSIM11_x_x.zip" del "..\Install\BioSIM11_x_x.zip"
.\bin\Releasex64\bin\7za.exe a -r "..\Install\BioSIM11_x_x.zip" "..\Install\NRCan"





