cmake_minimum_required(VERSION 3.24)

project(wbsModels)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(ActiaInterrupta)
add_subdirectory(ActiaInterrupta_OBL_SBW)
add_subdirectory(ObliqueBandedLeafroller)
add_subdirectory(SpruceBudworm)
add_subdirectory(SpuceBudwormManitoba)	#TODO : fix typo in directory name here
add_subdirectory(AllenWave)
add_subdirectory(AprocerosLeucopoda)
add_subdirectory(Bagworm)
add_subdirectory(Climatic)
add_subdirectory("Biophysical site indices (Ecoloap)")
add_subdirectory(BlueStain)
add_subdirectory(BudBurst)
add_subdirectory(ClimateMoistureIndex)
add_subdirectory(Climdex)
add_subdirectory(EmeraldAshBorer)
add_subdirectory(EuropeanElmScale)
add_subdirectory(Cankerworms)
add_subdirectory(FWI)
add_subdirectory(ForestTentCaterpillar)
add_subdirectory(GypsyMoth)
add_subdirectory(HemlockLooper)
add_subdirectory(HemlockLooperRemi)
add_subdirectory(HemlockWoollyAdelgid)
add_subdirectory(Agriculture)
add_subdirectory(JackpineBudworm)
add_subdirectory("Popillia Japonica")
add_subdirectory(LaricobiusNigrinus)
add_subdirectory(LaricobiusOsakensis)
add_subdirectory("Leucotaraxis spp")
add_subdirectory(LeucotaraxisArgenticollis)
add_subdirectory(LeucotaraxisPiniperda)
add_subdirectory(MeteorusTrachynotus)
add_subdirectory(MeteorusTrachynotus_OBL_SBW)
add_subdirectory(MPB)
add_subdirectory(PlantHardiness)
add_subdirectory(SiteIndexClimate)
add_subdirectory(SoilMoistureIndex)
add_subdirectory(Solar)
add_subdirectory(SpruceBeetle)
add_subdirectory("Standardized Precipitation Evapotranspiration Index")
add_subdirectory(Tranosema)
add_subdirectory(Tranosema_OBL_SBW)
add_subdirectory(UnderbarkTemperature)
add_subdirectory(WesternSpruceBudworm)
add_subdirectory(WhitemarkedTussockMoth)
add_subdirectory(WhitePineWeevil)
add_subdirectory(YellowheadedSpruceSawfly)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_target_properties(ActiaInterruptaBiology PROPERTIES FOLDER "wbsModels")
  set_target_properties(ActiaInterruptaLib PROPERTIES FOLDER "wbsModels")
  set_target_properties(ActiaInterrupta-OBL-SBW PROPERTIES FOLDER "wbsModels")
  set_target_properties(ObliqueBandedLeafrollerLib PROPERTIES FOLDER "wbsModels")
  set_target_properties(ObliqueBandedLeafroller PROPERTIES FOLDER "wbsModels")
  set_target_properties(SpruceBudwormBiology PROPERTIES FOLDER "wbsModels")
  set_target_properties(SpruceBudwormLib PROPERTIES FOLDER "wbsModels")
  set_target_properties(SpruceBudwormDispersal PROPERTIES FOLDER "wbsModels")  
  set_target_properties(SpruceBudwormLaboratory PROPERTIES FOLDER "wbsModels")  
  set_target_properties(SpruceBudwormEldon PROPERTIES FOLDER "wbsModels")  
  set_target_properties(SpruceBudwormLysyk PROPERTIES FOLDER "wbsModels")  
  set_target_properties(SpruceBudwormManitoba PROPERTIES FOLDER "wbsModels")  
  set_target_properties(AllenWave PROPERTIES FOLDER "wbsModels")  
  set_target_properties(AprocerosLeucopoda PROPERTIES FOLDER "wbsModels")  
  set_target_properties(Bagworm PROPERTIES FOLDER "wbsModels")  
  set_target_properties(ASCE-ETc PROPERTIES FOLDER "wbsModels")  
  set_target_properties(ASCE-ETsz PROPERTIES FOLDER "wbsModels")  
  set_target_properties(BiophysicalSiteIndicesEcoloap PROPERTIES FOLDER "wbsModels")  
  set_target_properties(BlueStainVariablesLib PROPERTIES FOLDER "wbsModels")    
  set_target_properties(BlueStainIndex PROPERTIES FOLDER "wbsModels")  
  set_target_properties(BlueStainVariables PROPERTIES FOLDER "wbsModels")    
  set_target_properties(BudBurst PROPERTIES FOLDER "wbsModels")    
  set_target_properties(BudBurstSaintAmant PROPERTIES FOLDER "wbsModels")    
  set_target_properties(BudBurstSBWHost PROPERTIES FOLDER "wbsModels")    
  set_target_properties(CCBio PROPERTIES FOLDER "wbsModels")
  set_target_properties(ClimateMoistureIndex PROPERTIES FOLDER "wbsModels")
  set_target_properties(Climatic PROPERTIES FOLDER "wbsModels")
  set_target_properties(ClimaticQc PROPERTIES FOLDER "wbsModels")
  set_target_properties(ClimaticWind PROPERTIES FOLDER "wbsModels")
  set_target_properties(Climdex PROPERTIES FOLDER "wbsModels")  
  set_target_properties(CornHeatUnits PROPERTIES FOLDER "wbsModels")  
  set_target_properties(DailyVsHourly PROPERTIES FOLDER "wbsModels")  
  set_target_properties(DailyVsNormals PROPERTIES FOLDER "wbsModels")  
  set_target_properties(DegreeDay PROPERTIES FOLDER "wbsModels")  
  set_target_properties(DegreeHour PROPERTIES FOLDER "wbsModels")  
  set_target_properties(DewPeriod PROPERTIES FOLDER "wbsModels")    
  set_target_properties(EmeraldAshBorer PROPERTIES FOLDER "wbsModels")      
  set_target_properties(EmeraldAshBorerColdHardiness PROPERTIES FOLDER "wbsModels")        
  set_target_properties(EuropeanElmScale PROPERTIES FOLDER "wbsModels")
  set_target_properties(FallCankerworms PROPERTIES FOLDER "wbsModels")  
  set_target_properties(SpringCankerworms PROPERTIES FOLDER "wbsModels")  
  set_target_properties(SpringFrost PROPERTIES FOLDER "wbsModels")  
  set_target_properties(FBPLib PROPERTIES FOLDER "wbsModels")  
  set_target_properties(FBP PROPERTIES FOLDER "wbsModels")    
  set_target_properties(FWI PROPERTIES FOLDER "wbsModels")    
  set_target_properties(ForestTentCaterpillar PROPERTIES FOLDER "wbsModels")   
  set_target_properties(FWIDroughtCode PROPERTIES FOLDER "wbsModels")    
  set_target_properties(GrowingSeason PROPERTIES FOLDER "wbsModels")    
  set_target_properties(GypsyMothLib PROPERTIES FOLDER "wbsModels")    
  set_target_properties(GypsyMothSeasonality PROPERTIES FOLDER "wbsModels")    
  set_target_properties(GypsyMothStability PROPERTIES FOLDER "wbsModels")    
  set_target_properties(HemlockLooper PROPERTIES FOLDER "wbsModels")      
  set_target_properties(HemlockLooperRemi PROPERTIES FOLDER "wbsModels")        
  set_target_properties(HemlockWoollyAdelgid PROPERTIES FOLDER "wbsModels")        
  set_target_properties(HourlyGenerator PROPERTIES FOLDER "wbsModels")        
  set_target_properties(HWAPhenology PROPERTIES FOLDER "wbsModels")        
  set_target_properties(InsectDevelopmentDatabaseII PROPERTIES FOLDER "wbsModels")        
  set_target_properties(InsectDevelopmentDatabaseIII PROPERTIES FOLDER "wbsModels")        
  set_target_properties(MagareyInfection PROPERTIES FOLDER "wbsModels")          
  set_target_properties(ClimaticLib PROPERTIES FOLDER "wbsModels")
  set_target_properties(JackpineBudworm PROPERTIES FOLDER "wbsModels")  
  set_target_properties(JapaneseBeetle PROPERTIES FOLDER "wbsModels")    
  set_target_properties(LAI PROPERTIES FOLDER "wbsModels")   
  set_target_properties(LaricobiusNigrinus PROPERTIES FOLDER "wbsModels")   
  set_target_properties(LaricobiusOsakensis PROPERTIES FOLDER "wbsModels")
  set_target_properties(LeucotaraxisSpp PROPERTIES FOLDER "wbsModels")
  set_target_properties(LeucotaraxisArgenticollis PROPERTIES FOLDER "wbsModels")
  set_target_properties(LeucotaraxisPiniperda PROPERTIES FOLDER "wbsModels")
  set_target_properties(MeteorusTrachynotusLib PROPERTIES FOLDER "wbsModels")  
  set_target_properties(MeteorusTrachynotus_OBL_SBW PROPERTIES FOLDER "wbsModels")  
  set_target_properties(MPBLib PROPERTIES FOLDER "wbsModels")  
  set_target_properties(MPB-ColdTolerance PROPERTIES FOLDER "wbsModels")  
  set_target_properties(MPB-iModel PROPERTIES FOLDER "wbsModels")  
  set_target_properties(MPB-SLR PROPERTIES FOLDER "wbsModels")  
  set_target_properties(PlantHardiness PROPERTIES FOLDER "wbsModels")    
  set_target_properties(PotentialEvapotranspiration PROPERTIES FOLDER "wbsModels")    
  set_target_properties(ReverseDegreeDay PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SiteIndexClimate PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SnowMelt PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SoilMoistureIndex PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SoilMoistureIndexQL PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SoilTemperature PROPERTIES FOLDER "wbsModels")    
  set_target_properties(Solar PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SpruceBeetle PROPERTIES FOLDER "wbsModels")    
  set_target_properties(StandardisedPrecipitationEvapotranspirationIndex PROPERTIES FOLDER "wbsModels")    
  set_target_properties(SummerMoisture PROPERTIES FOLDER "wbsModels")    
  set_target_properties(TminTairTmax PROPERTIES FOLDER "wbsModels")      
  set_target_properties(TranosemaLib PROPERTIES FOLDER "wbsModels")      
  set_target_properties(TranosemaBiology PROPERTIES FOLDER "wbsModels")      
  set_target_properties(Tranosema-OBL-SBW PROPERTIES FOLDER "wbsModels")      
  set_target_properties(UnderbarkTemperature PROPERTIES FOLDER "wbsModels")      
  set_target_properties(VaporPressureDeficit PROPERTIES FOLDER "wbsModels")      
  set_target_properties(WesternSpruceBudworm PROPERTIES FOLDER "wbsModels")      
  set_target_properties(WetnessDuration PROPERTIES FOLDER "wbsModels")        
  set_target_properties(WhitemarkedTussockMoth PROPERTIES FOLDER "wbsModels")        
  set_target_properties(WhitePineWeevil PROPERTIES FOLDER "wbsModels")        
  set_target_properties(WinterThaw PROPERTIES FOLDER "wbsModels")       
  set_target_properties(WorldClimVars PROPERTIES FOLDER "wbsModels")         
  set_target_properties(YellowheadedSpruceSawfly PROPERTIES FOLDER "wbsModels") 
endif()

