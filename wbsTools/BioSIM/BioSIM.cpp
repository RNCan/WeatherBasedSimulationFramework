///////////////////////////////////////////////////////////////////////////// 
// version de BioSIM 
// 11.5.3 (1.1.3): 23/11/2018 Rémi Saint-Amant	Add completeness in WGInputAnalysis
//												Able to simulate next year without error
// 11.5.2 (1.1.2): 24/10/2018 Rémi Saint-Amant	Change mapping interpolation result
// 11.5.1 (1.1.1): 24/10/2018 Rémi Saint-Amant	Change the default interpolation spatial drift param from Expo to Elev. 
// 11.5.0 (1.1.0): 12/09/2018 Rémi Saint-Amant	Bug correction in unit of vapor pressure.
//												Bug correction in generation of hourly precipitation from daily values
//												Remove "2" in the temporary name H_TMIN2, H_TAIR2, H_TMAX2 and H_SRAD2
// 11.4.9 (1.0.1): 30/08/2018 Rémi Saint-Amant	Major update in spatial interpolation.
// 11.4.8: 11/07/2018	Rémi Saint-Amant	Optimization of memory in ATM
//											Correction of bug in sub-hourly output and time step
// 11.4.7: 05/07/2018	Rémi Saint-Amant	Bug correction ATM (no end loop)
// 11.4.6: 20/06/2018	Rémi Saint-Amant	Add match station export (with station name)  
// 11.4.5: 15/05/2018	Rémi Saint-Amant	New dispersal
// 11.4.4: 13/04/2018	Rémi Saint-Amant	Resize FileManager correcly
//											New SSI extraction dlg
// 11.4.3: 28/03/2018	Rémi Saint-Amant	Add snow gradient
// 11.4.2: 21/03/2018	Rémi Saint-Amant	Compile with Visual Studio 2017 
//											Bug correction in computation of solar radiation
// 11.4.1: 22/02/2018	Rémi Saint-Amant	Bug correction in merge component
//											Change context menu when language change
// 11.4.0: 08/02/2018	Rémi Saint-Amant	Activation of precipitation gradients. New default gradient and new shore file. 
// 11.3.8: 31/01/2018	Rémi Saint-Amant	Bug correction in the parameters variation
// 11.3.7: 21/12/2017   Rémi Saint-Amant	Remove graph. Bug correction with component comparison. Remove LANGUAGE 9, 1. 
// 11.3.6: 10/10/2017   Rémi Saint-Amant	Recompilation from backup after hard drive crash
// 11.3.5: 30/08/2017	Rémi Saint-Amant	Bug correction in reading model output and missing values
//											Bug correction in dispersal multi-processor activation
// 11.3.4: 18/07/2017	Rémi Saint-Amant	Bug correction in memory allocation in the dispersal component 
//											Add optimization when only missing data into the DBBase
// 11.3.3: 06/06/2017 	Rémi Saint-Amant	Add egg density mapping in dispersal  
// 11.3.2: 06/05/2017 	Rémi Saint-Amant	Update of the hourly temperature generation
//											Compile with GDAL 2.02 
//											Bug correction in merge name and title
// 11.3.1: 02/05/2017	Rémi Saint-Amant	Bug correction into the merge component
//											Automaticly expand parent component when we add children
//											modification of the CSV reader. Read the last line event without CRLF
// 11.2.7: 22/04/2017	Rémi Saint-Amant	Tair hourly change to AllenWave 
// 11.2.6: 05/04/2017	Rémi Saint-Amant	Bug correction in mapping Xvalidation with noData and VMISS
//											Bug correction model file transfer 
// 11.2.5: 02/03/2016	Rémi Saint-Amant	Simplification of the ATM module 
// 11.2.4: 16/02/2016	Rémi Saint-Amant	Bug correction atmospheric pressure  
//											Change in callback  
// 11.2.3: 10/01/2017	Rémi Saint-Amant	New dispersal 
// 11.2.2: 03/01/2017   Rémi Saint-Amant	New GDAL options in mapping 
// 11.2.1: 09/12/2016	Rémi Saint-Amant	New dispersal module  
// 11.2.0: 23/11/2016	Rémi Saint-Amant	New database format  
// 11.1.3: 01/11/2016	Rémi Saint-Amant	New database with .csv file 
// 11.1.2: 13/10/2016   Rémi Saint-Amant	Change in ATM to accept NAM gribs 
// 11.1.1: 27/09/2016	Rémi Saint-Amant	Bug correction in merge 
// 11.1.0: 20/09/2016   Rémi Saint-Amant	Replace Tair and Trng by Tmin and Tmax 
// 11.0.9: 18/08/2016   Rémi Saint-Amant	Bug correction in weather database
//						Rémi Saint-Amant	Bug correction in gradient and hourly data
// 11.0.8: 14/08/2016	Rémi Saint-Amant	Add "AddName" in MergeExecutable
//						Rémi Saint-Amant	Bug correction in col width
//						Rémi Saint-Amant	Some bug correction in the ModelInputDlg
//						Rémi Saint-Amant	Add new input type: staticText
//						Rémi Saint-Amant	Bug correction in functionAnalysis
// 11.0.7: 28/06/2016	Rémi Saint-Amant	Bug correction in weather generation of hourly data.
//						Rémi Saint-Amant	Add fillGap into daily and hourly weather data
// 11.0.6: 17/06/2016	Rémi Saint-Amant	Bug correction in models. Change in result. return binary representation. 
//						Rémi Saint-Amant	Bug correction in mapping and exposition
// 11.0.5: 10/06/2016	Rémi Saint-Amant	Bug correction in gradients. Activate ppt gradient. 
// 11.0.4: 24/05/2016	Rémi Saint-Amant	Some bug correction in the ATM module
// 11.0.3: 24/04/2016	Rémi Saint-Amant	Add climatic zone to Gribs simulation
//						Rémi Saint-Amant	Bug correction in snow and Input Weather Generator Analysis
//						Rémi Saint-Amant	Bug correction when only one element
// 11.0.2: 21/04/2016	Rémi Saint-Amant	Correction of bug in the weather generation with more than 1 replication.
// 11.0.0: 10/04/2016	Rémi Saint-Amant	Integreted with WBSF 
//						Rémi Saint-Amant	Add hourly input data   
//						Rémi Saint-Amant	Version 64 bits and UNICODE
//						Rémi Saint-Amant	Add WeatherGeneration and ModelExecution
//						Rémi Saint-Amant	replace link of Fortran DLL Powell by bobyQA (dlib)
//						Rémi Saint-Amant	Compile with GDAL 1.11
//						Rémi Saint-Amant	Add new component : Dispersal, Export, Parameterization, WeatherUpdater
//						Rémi Saint-Amant	Update of many user interface 
//						Rémi Saint-Amant	New loc generation from weather database
//						Rémi Saint-Amant	Add specific site extraction
//						Rémi Saint-Amant	Change progression window by a dockable pane
// 10.3.2:				Rémi Saint-Amant	Add Standard deviation (N) and some statistic in Analysis  
//						Rémi Saint-Amant	Add Copy Export component 
//						Rémi Saint-Amant	Add direct file transfer from BioSIM to models 
//						Rémi Saint-Amant	Add SetStaticData function in simulation and simulatedAnnealing
//						Rémi Saint-Amant	Bug correction in global limit in spatial interpolation
//						Rémi Saint-Amant	Add maximum number of CPU
//						Rémi Saint-Amant	Add cancel in simulation database creation 
// 10.3.1:16/05/2013    Rémi Saint-Amant	Correction of a bug in the Weather Define 
// 10.3.0:10/05/2013    Rémi Saint-Amant	Correction of hourly simulation
//						Rémi Saint-Amant	correction in Simulated Annealing 
//						Rémi Saint-Amant	New simulation init 
//						Rémi Saint-Amant	New CSV streaming
//						Rémi Saint-Amant	Correction in CTRef format of the daylight saving zone 
//						Rémi Saint-Amant	Add wetness Duration and new DD, ReverseDD et ClimaticDLL 
// 10.2.5:15/02/2013	Rémi Saint-Amant	Add % of max of variable in Analysis event 
//						Rémi Saint-Amant	Bug correction in model load and unload 
//						Rémi Saint-Amant	Bug correction in Simulated Annealing
//						Rémi Saint-Amant	Bug correction with hxGrid in Simulation
//						Rémi Saint-Amant	Bug correction in multi-threaded weather optimization file 
//						Rémi Saint-Amant	Correction in hxGrid simulation
//						Rémi Saint-Amant	Add kriging local filter
//						Rémi Saint-Amant	Add mapping global filter
//						Rémi Saint-Amant	Correction of a bug in CRegistry and leak of handle 
// 10.2.4:18/09/2012	Rémi Saint-Amant	Correction of bug in Input Analysis
//						Rémi Saint-Amant	Change in Simulated Annealing  
//						Rémi Saint-Amant	Add R scrip capability
//						Rémi Saint-Amant	Add export all line event with missing data
//						Rémi Saint-Amant	Bug correction in time format before 1970 
//						Rémi Saint-Amant	Bug correction in CleanUp of the Analysis 
// 10.2.3:16/07/2012	Rémi Saint-Amant	Add missing value into function analysis  
//						Rémi Saint-Amant	New wind speed calculation (remove mean adjustment and maximum limit)
// 10.2.2:02/07/2012	Rémi Saint-Amant	Fix a problem with map without data (month outside selected period)
// 10.2.1:08/05/2012	Rémi Saint-Amant	Fix a bug in Kriging. Fix problem with PTParser.
//						Rémi Saint-Amant	Fix a bug into the mode input dialog (input file type)
// 10.2.0:26/04/2012    Rémi Saint-Amant	New Manual.
//						Rémi Saint-Amant	Bug correction in Loc generator  
// 10.1.9:21/03/2012	Rémi Saint-Amant	Correction for BioSIM manual 
//						Rémi Saint-Amant	Always return default period reference 
//						Rémi Saint-Amant	Add message in simulation when the TGInput is not good
//						Rémi Saint-Amant	Correction of initial size of windows
//						Rémi Saint-Amant	Add JDay in FunctionAnalysis
// 10.1.8:14/03/2012	Rémi Saint-Amant	New Thin Plate Splines
//						Rémi Saint-Amant	String correction 
//						Rémi Saint-Amant	Correction of a bug in sub-region generate loc
// 10.1.7:01/03/2012	Rémi Saint-Amant	Correction of a bug in location list and ";" separator
//						Rémi Saint-Amant	GDAL 1.9.0
//						Rémi Saint-Amant	String correction  
//						Rémi Saint-Amant	Add compression when creating GeoTiff file
//						Rémi Saint-Amant	Bug correction in random loc extraction with aggregation and subset
// 10.1.6:13/02/2012	Rémi Saint-Amant	TempGen to WeatherGenerator (multi-threads)
//						Rémi Saint-Amant	New random generator (std::tr1) in weatherGenerator
//						Rémi Saint-Amant	Correction of a bug in multi-analysis 
// 10.1.5:09/02/2012	Rémi Saint-Amant	Correction of problem with the French version.
//						Rémi Saint-Amant	multi-Threaded Analyses
// 10.1.4:19/01/2012	Rémi Saint-Amant	Add cleanup  
//						Rémi Saint-Amant	Good number of task for mapping 
//						Rémi Saint-Amant	Add catch exception on task execute 
//						Rémi Saint-Amant	New snow model into BioSIM. New climatic model and snow compute from BioSIM.
//						Rémi Saint-Amant	New Analysis HowPage 
//						Rémi Saint-Amant	Bug correction into cumul % analysis event 
//						Rémi Saint-Amant	Remove map edit dialog
//						Rémi Saint-Amant	Correction of bug in command line execution
//						Rémi Saint-Amant	Text corrections in French and English
//						Rémi Saint-Amant	Bug correction in event analysis
// 10.1.3:16/11/2011	Rémi Saint-Amant	Correction de CAutoEnableStatic
//						Rémi Saint-Amant	Add the real number of task
//						Rémi Saint-Amant	Correction in simulation to load daily database only once
//						Rémi Saint-Amant	Add CoCreateInstance on thread
// 10.1.2:14/11/2011	Rémi Saint-Amant	Add %c in the mapping
//						Rémi Saint-Amant	Correction of bug in the location list dialog
//						Rémi Saint-Amant	Add save as image in the graph view
//						Rémi Saint-Amant	Add openMp in simulation and add thread safe in model
//						Rémi Saint-Amant	Add new threaded progressDlg
//						Rémi Saint-Amant	Add taskbar notification
// 10.1.1:26/10/2011    Rémi Saint-Amant	Add help et download weather database 
//						Rémi Saint-Amant	Bug correction in Import component
// 10.1.0:06/10/2011	Rémi Saint-Amant	Look change before closing 
//						Rémi Saint-Amant	New ModelInput, TGInput and Loc Editor
//						Rémi Saint-Amant	New loc format and new loc extension
//						Rémi Saint-Amant	Correction of the bug of floating windows
//						Rémi Saint-Amant	New graph capability
//						Rémi Saint-Amant	New Model Input page
//						Rémi Saint-Amant	Add extract normal
//						Rémi Saint-Amant	Add weatherLoc and X validation into MatchStationDlg
//				        Rémi Saint-Amant	New DBEditor and new NormalEditor and new DailyEditor application.
//						Rémi Saint-Amant	New model input window position
//						Rémi Saint-Amant	Add text editor options 
// 10.0.9:30/05/2011	Rémi Saint-Amant	Correction of a bug in mapping name   
//						Rémi Saint-Amant	Add export as shapefile
// 10.0.8:18/05/2011	Rémi Saint-Amant	New version with new FWI model
// 10.0.7:13/05/2011	Rémi Saint-Amant	Add FillNugget in Kriging
//						Rémi Saint-Amant	Add Lambert_Conformal_Conic in projection for old compatibility
//						Rémi Saint-Amant	Correct an problem writing projection
// 10.0.6:01/05/2011	Rémi Saint-Amant	Correction of a bug in mapping cancelation
//                      Rémi Saint-Amant	Transformation of the no data from the model into the no data of BioSIM
// 10.0.5:28/04/2011	Rémi Saint-Amant	Correction of a bug in kriging
//						Rémi Saint-Amant	Add limitation in post treatment mapping
//						Rémi Saint-Amant	Add writing ESRI grid to GDAL 1.8
//						Rémi Saint-Amant	Compiled with new new GDAL 1.8
//						Rémi Saint-Amant	Correction of languages and interfaces
//						Rémi Saint-Amant	Add Thin Plate Splines in grid interpolation class
// 10.0.4:13/04/2011	Rémi Saint-Amant	Add compile time in About.
// 10.0.3:07/04/2011	Rémi Saint-Amant	Correction of crash on window XP of the advanced mapping dialog. 
//						Rémi Saint-Amant	Correction of a bug in the generate regular grid from DEM
// 10.0.2:31/03/2011	Rémi Saint-Amant	Add capability of moving element. Correction of a bug in advanced option.
// 10.0.1:30/03/2011	Rémi Saint-Amant	Correction of a bug under window XP. crash the second time BioSIM is open.
// 10.0.0:01/03/2011	Rémi Saint-Amant	First exit
// 10.0.0:15/09/2008	Rémi Saint-Amant	A brand new version 
// 9.5.2:               Rémi Saint-Amant	Ajout du lien vers BioKriging
//						Rémi Saint-Amant	
// 9.5.1: 22/04/2008	Rémi Saint-Amant	Correction d'un bug avec la description des anlayses
//						Rémi Saint-Amant	Ajout d'un code d'erreur en mode automatique
// 9.5.0: 09/01/2008	Rémi Saint-Amant	Compilation avec visual studio 2005
//						Rémi Saint-Amant	Resource en anglais par defaut
// 9.2.0: 28/10/2007    Rémi Saint-Amant	Nouvelle base de données Normale, Quotidienne.
//						Rémi Saint-Amant	Utilisation de Ultimate Grid
//						Rémi Saint-Amant	Recherche en 3D 
//						Rémi Saint-Amant	Nouveau modèle, modelInput et TGInput en XML
//						Rémi Saint-Amant	Nouveau TempGenKernel pour tenir compte des nouvelles variables climatiques
// 9.1.0: 24/09/2007	Rémi Saint-Amant	Nouveau calcul de gradient thermique en 3 couches (local, regional, defaut). 
//						Rémi Saint-Amant	Generateur de LOC pour les DEM
//						Rémi Saint-Amant	Correction d'un problème avec la taille des fenêtres initiales.
//						Rémi Saint-Amant	Ajout du rapport de match
// 9.0.2: 27/03/2007	Rémi Saint-Amant	Correction d'un bug dans la precipitation des données quotidiennes
//						Rémi Saint-Amant	Nouveau fichier LOC: version 3
// 9.0.1: 13/02/2007	Rémi Saint-Amant	Nouveau projet BioSIM incompatible avec BioSIM8. 
//						Rémi Saint-Amant	Nouvelle analyse Average Output: possibiliter de choisir les stations à exporter
// 9.0.0: 08/12/2006    Rémi Saint-Amant	Incorporation du nouveau TempGenKernel et des nouvelles bases de données normales.
//						Rémi Saint-Amant	Nouveau TempGen et élimination des zone climatiques
// 8.4.0: 02/06/2006    Rémi Saint-Amant	Ajout de l'export de la simulation
//						Rémi Saint-Amant	Correction d'un bug dans la validation des HomeDB
//						Rémi Saint-Amant	Correction d'un bug dans l'ajout d'une station forecast
//						Rémi Saint-Amant	Ajout du path dans les DBEditors
//						Rémi Saint-Amant	Correction d'un problème dans la progression d'une simulation
//						Rémi Saint-Amant	TempGen avec des nouveaux paramètres
// 8.3.9: 01/05/2006	Rémi Saint-Amant	Ajouts des Progress bar dans le chargement des bases de données 
// 8.3.8: 19/04/2006	Rémi Saint-Amant	Correction d'un bug dans Le match station
// 8.3.7: 14/03/2006	Rémi Saint-Amant	Correction d'un bug dans le match station quotidien
// 8.3.6: 08/03/2006	Rémi Saint-Amant	Correction d'un bug dans les path relatif
// 8.3.5: 20/01/2006	Rémi Saint-Amant	Correction d'un bug dans les analyse de grosse simulation. 
//						Rémi Saint-Amant	Changement dans UtilWin et UtilMath
// 8.3.4: 31/10/2005    Rémi Saint-Amant	Enlever la possibiliter de sauvegarder les analyses en ascii
//						Rémi Saint-Amant	Nouveau fichier d'export pour Biokriging
// 8.3.3: 28/10/2005	Rémi Saint-Amant	Correction d'un bug dans la lecture des vieux LOC 
// 8.3.2: 20/10/2005    Rémi Saint-Amant	Correction d'un bug dans les analyses cumulative de date
// 8.3.1: 11/10/2005    Rémi Saint-Amant	Correction des analyses composé (date) sub et add.
// 8.3.0: 21/09/2005	Rémi Saint-Amant	Changement de numéro
// 8.24: 13/07/2005    Ajout du nouveau type de BD de simulation (HomeDB).
//					   Optimisation dans le calcul des références temporelles.
//					   Ajout de la relation E2 dans les anal tMean.
//					   Correction d'un bug dans l.analyse cumulative 
//					   Ajout de la transformation des grid ESRI
//					   Par default la langue retourne l'anglais
// 8.23: 04/07/2005	   Correction d'un bug important dans le nom des BD. les sim et les anal
//					   n'utilisait pas le même nom.
// 8.22: 24/05/2005	   Ajout des titre dans l'export des fichiers
// 8.21: 25/04/2005	   Correction d'un bug dans la plage temporelle des analyses.
// 8.20: 21/03/2005	   Nouvelle représentation temporelle dans les analyses(les deux types de BD).
// 8.12: 09/03/2005	   Bug dans les options. Bugs dans generateur de LOC.
// 8.11: 03/03/2005    Correction du bug pour changer les répertoires
//					   CGeoRect et CGeoPoint avec geo reference
//					   Update de ArcInfoGrid
// 8.10: 11/02/2005    Update: modificatio dans librairie mapping
//					   Lecture des float dans les fichiers .wea
// 8.08: 23/06/2004	   Correction d'un bug quand on efface la dernière simulation 
//					   Correction des dialogues
//					   Correction d'un problème avec les "'" dans le nom du projet
//					   Correction du problème "project" et "projects"
//					   Ajout de la validation X dans le dlg des analyses
//					   Ne pas suvegarder les projets par defaut
// 8.07: 19/04/2004	   Demande de confirmation de simuler des simulation valide
//					   affiche le temp de simulation et le temps d'analyse
//					   Correction d'un bug dans la precision du modèle output
//					   Correction d'un bug dansla creation des BD forecasts
//					   Correction d'un bug dans l'extraction d'un LOC a partir d'un Grid ESRI
//					   Accept les fichier .Wea qui ont des lignes vides
// 8.06: 23/03/2004	   Ajout d'un message pour les LOC > 58333 lignes.
//					   Creation de répertoire sur un import
//					   Correction d'un bud dans la sauvegarde des projets
//					   Correction d'un bud dans progress bar des analyses.
// 8.05: 22/03/2004	   Correction d'un bug dans l'enregistrement d'une station quotidiennes
//					   Correction de bugs quand les fichiers sont read-only
//					   Permettre d'importer plusieur modelInput à la fois.
// 8.04: 17/03/2004	   Correction des string.
//					   Ajout de la transforamtion logit dans les cartes.
//					   Correction d'un bud dans l'éditeur de LOC.
// 8.03: 03/03/2004	   Correction du bug dans le ModelInputDlg.  
//					   L'Ajout de nouveau model, l'ajout au bon endroit dans la liste de nouveau model
// 8.02: 27/02/2004	   Ajout des cartes Grass er ArcInfo
//					   Changement dans le noms des datum pour être compatible avec ArcInfo
//					   Changement dans la recherche du lag distance : Le R² peut être < 0
// 8.01: 19/02/2004	   Correction de bugs
//					   Ajout du dialogue de tranformation des bases de données.
//					   Changement dans le format du fichier de transferes
// 8.0 : 05/02/2004	   Nouvelle base de données normales(v4) et quotidiennes(v2)
//					   Nouveau FileManager(utilisation du DirectoryManager )
//					   Nouveau TGInput
//					   Nouveau format du fichier de transfer pour l'utilisation de la classe CBioSIMModelBase
//					   Nouvelle exposition et calcul du deficit de pression de vapeur(dans CBioSIMModelBase)
//					   Nouveau TempGenKernel
//					   Correction de bugs
//					   Elimination des fichiers avec les string(CResString)
// 7.2 : 03/10/2003	   Compilation avec VC7	
// 7.13 : 03/09/2003   Correction d'un bug dans les analyses de fichier
//                     Correction d'un bug dans le graphique des poids journaliers 
//                     Correction d'un problème avec les années négative
// 7.12 : 11/07/2003   Enlever la possibiliter "Best" dans la création des cartes
//                     Correction d'un bug dans les analyses moyennes
// 7.11 : 10/07/2003   Correction de texte, correction d'un bug dans l'appel de PLT
// 7.10 : 02/07/2003   Ajout dans les résultats de la pente et de l'orientation.
//                     Ajout dans le match dialog du graphique jour par jour
//                     Ajout dans le match dialog du combo box de l'années
//                     Ajout du krigeage dans les analyses cartographiques
// 7.05 : 16/06/2003   Correction d'un bug dans la lecture des bases de données temps réelles
// 7.04 : 01/05/2003   Correction d'un bug dans la composition de la base de donnés de simulation.
// 7.03 : 03/04/2003   Correction d'un bug dans le calcul de l'exposition (roundoff)
// 7.02 : 17/03/2003   Correction d'un bug dans les BD qui ne lisait pas correctement les entiers
//                     Correction d'un bug dans tempGen prenait un mauvais pour le calcul de la radiation
// 7.01 : 04/03/2003   Correction d'un bug dans les settings pour les projections.
// 7.00 : 17/02/2003   Nouveau DB Editor
//                     Utilisation de fichier de ressources séparés
//                     Nouveau type de fichier LOC : Decimal Degree
// 7.00  : 04/12/2003 : Correction d'un bug dans tempGen avec le MAX_DAY = 366
//                     Correction d'un bug dans TempGen sur précipitation en temp réel
//                     Nouveau modèle avec TGInput et Documentation
//                     Possibiliter de modèle bilingue
//                     Nouveau point d'entré dans les dll DoSimulation2
// 6.61 : 18/11/2002 : ???
// 6.60 : 24/10/2002 : Ajout de la radiation dans tempGen
// 6.54 : 17/10/2002 : nouvelle version 
// 6.53 : 06/09/2002 : nouveau CBinaryMap pour prendre les ShortGrids
//                     Correction d'un bug dans la lecture des msb 
// 6.53 : 15/08/2002 : Support pour les fichier .prj   
// 6.52 : Dans TempGen, inverser realMin et realMax si min plus grand que max.
// 6.51 : Modification de TempGenKernel par Jaques.
// 6.5  : Ajout du nombre d'année à simuler dans le model. modif de tempgen
//        pour pouvoir simuler sur plusieur années.
// 6.4  : Ajout de deux options dans l'élaboration des models: Le point de localisation et le numéro de l'itération
//        BioSIM prend maintenant les exécutable DOS sans afficher de console    
// 6.33 : J'ai changer la valeur du R² pour ajouter un terme à la régression
//        La valeur est passer de 0.01 à 0.005. De plus un champ a été ajouté dans les options.
// 6.32 : Correction d'un bug dans UtilWin::Equal, corrige un problème dans la sélection des analyses
// 6.31 : Radio au lieu de checkbox dans le dialog des matchs stations

#include "stdafx.h" 
//#include "VisualLeakDetector\include\vld.h"


#include "Basic/ANN/ANN.h"
#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "Basic/Shore.h"
#include "Geomatic/UtilGDAL.h"
#include "Geomatic/TimeZones.h"
#include "FileManager/FileManager.h"
#include "UI/Common/StandardCmdLine.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/SYShowMessage.h"
#include "BioSIM.h"
#include "MainFrm.h"
#include "BioSIMDoc.h"
#include "OutputView.h"
#include "WeatherBasedSimulationUI.h"
#include "WeatherBasedSimulationString.h"
#include "wbs_version.h"


using namespace UtilWin;
using namespace WBSF;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBioSIMApp

BEGIN_MESSAGE_MAP(CBioSIMApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CBioSIMApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// CBioSIMApp construction

CBioSIMApp::CBioSIMApp():
	m_gdiplusToken(0)
{
	SetDllDirectory(CString((GetApplicationPath() + "External").c_str()));
	EnableHtmlHelp();

	m_bHiColorIcons = TRUE;
	SetAppID(_T("NRCan.BioSIM.11"));
	m_exitCode=0;
}

CBioSIMApp::~CBioSIMApp()
{
}
// The one and only CBioSIMApp object

CBioSIMApp theApp;

BOOL CBioSIMApp::InitInstance()
{
	CRegistry registre; 

    short language = registre.GetLanguage();
    HINSTANCE hInst = NULL;

	if( language == CRegistry::FRENCH )
	{
        hInst = LoadLibrary(_T("BioSIM11Frc.dll"));
		if (hInst != NULL)
			AfxSetResourceHandle(hInst);
	}

	ASSERT(AfxGetResourceHandle());
	CDynamicResources::set(AfxGetResourceHandle());


	//set local to default operating system
	static std::locale THE_LOCALE(std::locale(".ACP"), std::locale::classic(), std::locale::numeric);
	std::locale::global(THE_LOCALE);

	setlocale(LC_ALL, ".ACP");
	setlocale(LC_NUMERIC, "English");
	


	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	// Set this to include all the common control classes you want to use
	// in your application.

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);


	CWinAppEx::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	

	//init gdiplus to avoir crash when BioSIM close
	Gdiplus::GdiplusStartupInput    gdiplusStartupInput;
	if (Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
	{
		MessageBox(NULL, TEXT("GDI+ failed to start up!"), TEXT("Error!"), MB_ICONERROR);
		return FALSE;
	}


	//This DLL must be in the BioSIM directory for french version
//	VERIFY( MTTools::registerCOMObject(_T("MTParserInfoFile.dll")) );
	//try
	//{
	//	
	//
	//	// load plug-ins...	to load solve 
	//	//if( !MTTools::registerCOMObject(NUMALGOPLUGINFILE) )
	//	//pParser->loadAllPlugins((LPCTSTR)GetCurrentAppPath(), _T("*.xml"));				
	//	//MTParserLocalizer::getInstance()->registerAllLibraries((LPCTSTR)(GetApplicationPath()+"MTParser\\"), _T("*.xml"));
	//	//MTParserLocalizer::getInstance()->setLocale(_T("en"));
	//	//MTParserLocalizer::getInstance()->setLocale(_T("fr"));

	//}
	//catch( ... )
	//{
	//	//MessageBox(getAllExceptionString(e).c_str(), RToS(IDS_TITLE_INDEXUNAVAILABLE), MB_OK|MB_ICONSTOP);		
	//}
	//

	ERMsg msg = CShore::SetShore(GetApplicationPath() + "Layers/Shore.ann");
	msg += CTimeZones::Load(GetApplicationPath() + "zoneinfo/time_zones.shp");

	if (!msg)
		SYShowMessage(msg, ::AfxGetMainWnd());


	SetRegistryKey(_T("NRCan"));
	LoadStdProfileSettings(15);  // Load standard INI file options (including MRU)

	//intialise at the beggining of the application the random number for 
	//the generation of the internalName
	WBSF::Randomize();
	RegisterGDAL();




	//Set temporal format
	CRegistry registry("Time Format");
	
	CTRefFormat format;
	
	for(int t=0; t<CTM::NB_REFERENCE; t++)
	{
		for(int m=0; m<CTM::NB_MODE; m++)
		{
			CTM tm(t,m);
			format.SetHeader(tm, registry.GetProfileString(std::string(tm.GetTypeModeName()) + "[header]", CTRefFormat::GetDefaultHeader(tm)).c_str() );
			format.SetFormat(tm, registry.GetProfileString(std::string(tm.GetTypeModeName()) + "[format]", CTRefFormat::GetDefaultFormat(tm)).c_str());
		}
	}

	CTRef::SetFormat(format);

	// Parse command line for standard shell commands, DDE, file open
	CStdCmdLine cmdInfo;
	ParseCommandLine(cmdInfo);
	// Dispatch commands specified on the command line.  
	if( cmdInfo.Is(CStdCmdLine::EXECUTE) )
	{
		ERMsg msg;

		//CSCCallBack callback;
		CProgressStepDlg dlg;
		if( cmdInfo.Is(CStdCmdLine::SHOW) )
			dlg.Create(m_pMainWnd);

		std::string absolutePath = CStringA(UtilWin::GetAbsolutePath( GetCurrentDirectory(), cmdInfo.m_strFileName));
		CExecutablePtr projectPtr;
		CBioSIMProject* pProject = new CBioSIMProject;
		pProject->SetMyself(&projectPtr);
		projectPtr.reset(pProject);

		
		projectPtr->LoadDefaultCtrl();

		msg += pProject->Load(absolutePath);
		if( msg)
		{
			GetFM().SetProjectPath( GetPath(absolutePath) );

			CProgressStepDlgParam param(pProject, NULL, &GetFM());
			msg += dlg.Execute(CBioSIMDoc::ExecuteTask, &param);
		}
			

		if( cmdInfo.Is(CStdCmdLine::LOG) )
		{
			CStdioFileEx file;

			ERMsg msgTmp = file.Open(cmdInfo.GetParam(CStdCmdLine::LOG), CFile::modeCreate|CFile::modeWrite);
			if( msgTmp)
			{
				CString logText = SYGetOutputCString(msg, dlg.GetCallback() );
				file.WriteString(logText);
				file.Close();
			}
			msg += msgTmp;
		}
		
		if (!msg && cmdInfo.Is(CStdCmdLine::SHOW))
			::AfxMessageBox(UtilWin::SYGetText(msg));


		m_exitCode = msg?0:-1;
		return FALSE;
	}

	
	InitCommonControls();
	InitShellManager();
	InitContextMenuManager();
	InitKeyboardManager();
	InitTooltipManager();
	
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);
	//CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	//CMFCButton::EnableWindowsTheming(TRUE);
	

	
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CBioSIMDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(COutputView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	
	

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister. 
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;


	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	
	return TRUE;
}

void CBioSIMApp::ParseCommandLine(CCommandLineInfo& rCmdInfo)
{
	for (int i = 1; i < __argc; i++)
	{
		LPCTSTR pszParam = __targv[i];
		BOOL bFlag = FALSE;
		BOOL bLast = ((i + 1) == __argc);
		if (pszParam[0] == '/' || pszParam[0] == '-')
		{
			// remove flag specifier
			bFlag = TRUE;
			++pszParam;
		}
		rCmdInfo.ParseParam(pszParam, bFlag, bLast);
	}
}

int CBioSIMApp::ExitInstance() 
{
	//clear ann global data to avoid memory leak
	annClose();
	int exitCode = CWinApp::ExitInstance();
	if( exitCode == 0 )
		exitCode = m_exitCode;


	GetKeyboardManager()->CleanUp();
	CMFCToolBar::CleanUpImages();
	CMFCVisualManager::DestroyInstance();
//	GDALDestroyDriverManager();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return exitCode;
}

// App command to run the dialog
void CBioSIMApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CBioSIMApp customization load/save methods

void CBioSIMApp::PreLoadState()
{
	VERIFY(GetContextMenuManager()->AddMenu(_T("Popup"), IDR_POPUP));
	VERIFY(GetContextMenuManager()->AddMenu(_T("Edit1"), IDR_MENU_EDIT));
	
}

BOOL CBioSIMApp::InitContextMenuManager()
{
	if (afxContextMenuManager != NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	afxContextMenuManager = new CContextMenuManagerEx;
	m_bContextMenuManagerAutocreated = TRUE;

	return TRUE;

}

void CBioSIMApp::LoadCustomState()
{
	
}

void CBioSIMApp::SaveCustomState()
{
}

//*********************************************************************************

BOOL CContextMenuManagerEx::RestoreOriginalState()
{
	POSITION pos = NULL;

	for (pos = m_Menus.GetStartPosition(); pos != NULL;)
	{
		UINT uiResId;
		HMENU hMenu;

		m_Menus.GetNextAssoc(pos, uiResId, hMenu);
		::DestroyMenu(hMenu);
	}

	m_Menus.RemoveAll();

	for (pos = m_MenuOriginalItems.GetStartPosition(); pos != NULL;)
	{
		UINT uiResId;
		CObList* pLstOrginItems = NULL;

		m_MenuOriginalItems.GetNextAssoc(pos, uiResId, pLstOrginItems);
		ASSERT_VALID(pLstOrginItems);

		while (!pLstOrginItems->IsEmpty())
		{
			delete pLstOrginItems->RemoveHead();
		}

		delete pLstOrginItems;
	}

	m_MenuOriginalItems.RemoveAll();
	m_MenuNames.RemoveAll();


	VERIFY(AddMenu(_T("Popup"), IDR_POPUP));
	VERIFY(AddMenu(_T("Edit1"), IDR_MENU_EDIT));


	return TRUE;
}


//*********************************************************************************
// CAboutDlg dialog used for App About
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_VERSION, m_versionCtrl);
	DDX_Control(pDX, IDC_WBSF_VERSION, m_WBSFversionCtrl);
	

	if( !pDX->m_bSaveAndValidate )
	{
		
		CString filepath;
		GetModuleFileNameW(GetModuleHandle(NULL), filepath.GetBuffer(MAX_PATH), MAX_PATH);
		filepath.ReleaseBuffer();

		CString version = UtilWin::GetVersionString(filepath);
		CString date = UtilWin::GetCompilationDateString(__DATE__);
		m_versionCtrl.SetWindowText( version + _T(" (") + date + _T(") 64 bits") );

		m_WBSFversionCtrl.SetWindowText(CString(WBSF_VERSION) );
		
	}
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

