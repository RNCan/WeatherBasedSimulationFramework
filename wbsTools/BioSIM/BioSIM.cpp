/////////////////////////////////////////////////////////////////////////////
// version de BioSIM
// 11.0.5: 24/04/2016	Bug correction in gradients. Activate ppt gradient.
// 11.0.4: 24/04/2016	Some bug correction in the ATM module
// 11.0.3: 24/04/2016	Add climatic zone to Gribs simulation
//						Bug correction in snow and Input Weather Generator Analysis
//						Bug correction when only one element
// 11.0.2: 21/04/2016	Correction of bug in the weather generation with more than 1 replication.
// 11.0.0: 10/04/2016	Integreted with WBSF 
//						Add hourly input data   
//						Version 64 bits and UNICODE
//						Add WeatherGeneration and ModelExecution
//						replace link of Fortran DLL Powell by bobyQA (dlib)
//						Compile with GDAL 1.11
//						Add new component : Dispersal, Export, Parameterization, WeatherUpdater
//						Update of many user interface 
//						New loc generation from weather database
//						Add specific site extraction
//						Change progression winodw by a dockable pane
// 10.3.2:				Add Standard deviation (N) and some statistic in Analysis  
//						Add Copy Export component 
//						Add direct file transfer from BioSIM to models 
//						Add SetStaticData function in simulation and simulatedAnnealing
//						Bug correction in global limit in spatial interpolation
//						Add maximum number of CPU
//						Add cancel in simulation database creation 
// 10.3.1:16/05/2013    Correction of a bug in the Weather Define 
// 10.3.0:10/05/2013    Correction of hourly simulation
//						correction in Simulated Annealing 
//						New simulation init 
//						New CSV streaming
//						Correction in CTRef format of the daylight saving zone 
//						Add wetness Duration and new DD, ReverseDD et ClimaticDLL
// 10.2.5:15/02/2013	Add % of max of variable in Analysis event 
//						Bug correction in model load and unload 
//						Bug correction in Simulated Annealing
//						Bug correction with hxGrid in Simulation
//						Bug correction in multi-threaded weather optimization file 
//						Correction in hxGrid simulation
//						Add kriging local filter
//						Add mapping global filter
//						Correction of a bug in CRegistry and leak of handle 
// 10.2.4:18/09/2012	Correction of bug in Input Analysis
//						Change in Simulated Annealing  
//						Add R scrip capability
//						Add export all line event with missing data
//						Bug correction in time format before 1970 
//						Bug correction in CleanUp of the Analysis 
// 10.2.3:16/07/2012	Add missing value into function analysis  
//						New wind speed calculation (remove mean adjustment and maximum limit)
// 10.2.2:02/07/2012	Fix a problem with map without data (month outside selected period)
// 10.2.1:08/05/2012	Fix a bug in Kriging. Fix problem with PTParser.
//						Fix a bug into the mode input dialog (input file type)
// 10.2.0:26/04/2012    New Manual.
//						Bug correction in Loc generator  
// 10.1.9:21/03/2012	Correction for BioSIM manual 
//						Always return default period reference 
//						Add message in simulation when the TGInput is not good
//						Correction of initial size of windows
//						Add JDay in FunctionAnalysis
// 10.1.8:14/03/2012	New Thin Plate Splines
//						String correction 
//						Correction of a bug in sub-region generate loc
// 10.1.7:01/03/2012	Correction of a bug in location list and ";" separator
//						GDAL 1.9.0
//						String correction  
//						Add compression when creating GeoTiff file
//						Bug correction in random loc extraction with aggregation and subset
// 10.1.6:13/02/2012	TempGen to WeatherGenerator (multi-threads)
//						New random generator (std::tr1) in weatherGenerator
//						Correction of a bug in multi-analysis 
// 10.1.5:09/02/2012	Correction of problem with the French version.
//						multi-Threaded Analyses
// 10.1.4:19/01/2012	Add cleanup  
//						Good number of task for mapping 
//						Add catch exception on task execute 
//						New snow model into BioSIM. New climatic model and snow compute from BioSIM.
//						New Analysis HowPage 
//						Bug correction into cumul % analysis event 
//						Remove map edit dialog
//						Correction of bug in command line execution
//						Text corrections in French and English
//						Bug correction in event analysis
// 10.1.3:16/11/2011	Correction de CAutoEnableStatic
//						Add the real number of task
//						Correction in simulation to load daily database only once
//						Add CoCreateInstance on thread
// 10.1.2:14/11/2011	Add %c in the mapping
//						Correction of bug in the location list dialog
//						Add save as image in the graph view
//						Add openMp in simulation and add thread safe in model
//						Add new threaded progressDlg
//						Add taskbar notification
// 10.1.1:26/10/2011    Add help et download weather database 
//						Bug correction in Import component
// 10.1.0:06/10/2011	Look change before closing 
//						New ModelInput, TGInput and Loc Editor
//						New loc format and new loc extension
//						Correction of the bug of floating windows
//						New graph capability
//						New Model Input page
//						Add extract normal
//						Add weatherLoc and X validation into MatchStationDlg
//				        New DBEditor and new NormalEditor and new DailyEditor application.
//						New model input window position
//						Add text editor options 
// 10.0.9:30/05/2011	Correction of a bug in mapping name   
//						Add export as shapefile
// 10.0.8:18/05/2011	New version with new FWI model
// 10.0.7:13/05/2011	Add FillNugget in Kriging
//						Add Lambert_Conformal_Conic in projection for old compatibility
//						Correct an problem writing projection
// 10.0.6:01/05/2011	Correction of a bug in mapping cancelation
//                      Transformation of the no data from the model into the no data of BioSIM
// 10.0.5:28/04/2011	Correction of a bug in kriging
//						Add limitation in post treatment mapping
//						Add writing ESRI grid to GDAL 1.8
//						Compiled with new new GDAL 1.8
//						Correction of languages and interfaces
//						Add Thin Plate Splines in grid interpolation class
// 10.0.4:13/04/2011	Add compile time in About.
// 10.0.3:07/04/2011	Correction of crash on window XP of the advanced mapping dialog. 
//						Correction of a bug in the generate regular grid from DEM
// 10.0.2:31/03/2011	Add capability of moving element. Correction of a bug in advanced option.
// 10.0.1:30/03/2011	Correction of a bug under window XP. crash the second time BioSIM is open.
// 10.0.0:01/03/2011	First exit
// 10.0.0:15/09/2008	A brand new version 
// 9.5.2:               Ajout du lien vers BioKriging
//						
// 9.5.1: 22/04/2008	Correction d'un bug avec la description des anlayses
//						Ajout d'un code d'erreur en mode automatique
// 9.5.0: 09/01/2008	Compilation avec visual studio 2005
//						Resource en anglais par defaut
// 9.2.0: 28/10/2007    Nouvelle base de donn�es Normale, Quotidienne.
//						Utilisation de Ultimate Grid
//						Recherche en 3D 
//						Nouveau mod�le, modelInput et TGInput en XML
//						Nouveau TempGenKernel pour tenir compte des nouvelles variables climatiques
// 9.1.0: 24/09/2007	Nouveau calcul de gradient thermique en 3 couches (local, regional, defaut). 
//						Generateur de LOC pour les DEM
//						Correction d'un probl�me avec la taille des fen�tres initiales.
//						Ajout du rapport de match
// 9.0.2: 27/03/2007	Correction d'un bug dans la precipitation des donn�es quotidiennes
//						Nouveau fichier LOC: version 3
// 9.0.1: 13/02/2007	Nouveau projet BioSIM incompatible avec BioSIM8. 
//						Nouvelle analyse Average Output: possibiliter de choisir les stations � exporter
// 9.0.0: 08/12/2006    Incorporation du nouveau TempGenKernel et des nouvelles bases de donn�es normales.
//						Nouveau TempGen et �limination des zone climatiques
// 8.4.0: 02/06/2006    Ajout de l'export de la simulation
//						Correction d'un bug dans la validation des HomeDB
//						Correction d'un bug dans l'ajout d'une station forecast
//						Ajout du path dans les DBEditors
//						Correction d'un probl�me dans la progression d'une simulation
//						TempGen avec des nouveaux param�tres
// 8.3.9: 01/05/2006	Ajouts des Progress bar dans le chargement des bases de donn�es 
// 8.3.8: 19/04/2006	Correction d'un bug dans Le match station
// 8.3.7: 14/03/2006	Correction d'un bug dans le match station quotidien
// 8.3.6: 08/03/2006	Correction d'un bug dans les path relatif
// 8.3.5: 20/01/2006	Correction d'un bug dans les analyse de grosse simulation. 
//						Changement dans UtilWin et UtilMath
// 8.3.4: 31/10/2005    Enlever la possibiliter de sauvegarder les analyses en ascii
//						Nouveau fichier d'export pour Biokriging
// 8.3.3: 28/10/2005	Correction d'un bug dans la lecture des vieux LOC 
// 8.3.2: 20/10/2005    Correction d'un bug dans les analyses cumulative de date
// 8.3.1: 11/10/2005    Correction des analyses compos� (date) sub et add.
// 8.3.0: 21/09/2005	Changement de num�ro
// 8.24: 13/07/2005    Ajout du nouveau type de BD de simulation (HomeDB).
//					   Optimisation dans le calcul des r�f�rences temporelles.
//					   Ajout de la relation E2 dans les anal tMean.
//					   Correction d'un bug dans l.analyse cumulative 
//					   Ajout de la transformation des grid ESRI
//					   Par default la langue retourne l'anglais
// 8.23: 04/07/2005	   Correction d'un bug important dans le nom des BD. les sim et les anal
//					   n'utilisait pas le m�me nom.
// 8.22: 24/05/2005	   Ajout des titre dans l'export des fichiers
// 8.21: 25/04/2005	   Correction d'un bug dans la plage temporelle des analyses.
// 8.20: 21/03/2005	   Nouvelle repr�sentation temporelle dans les analyses(les deux types de BD).
// 8.12: 09/03/2005	   Bug dans les options. Bugs dans generateur de LOC.
// 8.11: 03/03/2005    Correction du bug pour changer les r�pertoires
//					   CGeoRect et CGeoPoint avec geo reference
//					   Update de ArcInfoGrid
// 8.10: 11/02/2005    Update: modificatio dans librairie mapping
//					   Lecture des float dans les fichiers .wea
// 8.08: 23/06/2004	   Correction d'un bug quand on efface la derni�re simulation 
//					   Correction des dialogues
//					   Correction d'un probl�me avec les "'" dans le nom du projet
//					   Correction du probl�me "project" et "projects"
//					   Ajout de la validation X dans le dlg des analyses
//					   Ne pas suvegarder les projets par defaut
// 8.07: 19/04/2004	   Demande de confirmation de simuler des simulation valide
//					   affiche le temp de simulation et le temps d'analyse
//					   Correction d'un bug dans la precision du mod�le output
//					   Correction d'un bug dansla creation des BD forecasts
//					   Correction d'un bug dans l'extraction d'un LOC a partir d'un Grid ESRI
//					   Accept les fichier .Wea qui ont des lignes vides
// 8.06: 23/03/2004	   Ajout d'un message pour les LOC > 58333 lignes.
//					   Creation de r�pertoire sur un import
//					   Correction d'un bud dans la sauvegarde des projets
//					   Correction d'un bud dans progress bar des analyses.
// 8.05: 22/03/2004	   Correction d'un bug dans l'enregistrement d'une station quotidiennes
//					   Correction de bugs quand les fichiers sont read-only
//					   Permettre d'importer plusieur modelInput � la fois.
// 8.04: 17/03/2004	   Correction des string.
//					   Ajout de la transforamtion logit dans les cartes.
//					   Correction d'un bud dans l'�diteur de LOC.
// 8.03: 03/03/2004	   Correction du bug dans le ModelInputDlg.  
//					   L'Ajout de nouveau model, l'ajout au bon endroit dans la liste de nouveau model
// 8.02: 27/02/2004	   Ajout des cartes Grass er ArcInfo
//					   Changement dans le noms des datum pour �tre compatible avec ArcInfo
//					   Changement dans la recherche du lag distance : Le R� peut �tre < 0
// 8.01: 19/02/2004	   Correction de bugs
//					   Ajout du dialogue de tranformation des bases de donn�es.
//					   Changement dans le format du fichier de transferes
// 8.0 : 05/02/2004	   Nouvelle base de donn�es normales(v4) et quotidiennes(v2)
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
//                     Correction d'un probl�me avec les ann�es n�gative
// 7.12 : 11/07/2003   Enlever la possibiliter "Best" dans la cr�ation des cartes
//                     Correction d'un bug dans les analyses moyennes
// 7.11 : 10/07/2003   Correction de texte, correction d'un bug dans l'appel de PLT
// 7.10 : 02/07/2003   Ajout dans les r�sultats de la pente et de l'orientation.
//                     Ajout dans le match dialog du graphique jour par jour
//                     Ajout dans le match dialog du combo box de l'ann�es
//                     Ajout du krigeage dans les analyses cartographiques
// 7.05 : 16/06/2003   Correction d'un bug dans la lecture des bases de donn�es temps r�elles
// 7.04 : 01/05/2003   Correction d'un bug dans la composition de la base de donn�s de simulation.
// 7.03 : 03/04/2003   Correction d'un bug dans le calcul de l'exposition (roundoff)
// 7.02 : 17/03/2003   Correction d'un bug dans les BD qui ne lisait pas correctement les entiers
//                     Correction d'un bug dans tempGen prenait un mauvais pour le calcul de la radiation
// 7.01 : 04/03/2003   Correction d'un bug dans les settings pour les projections.
// 7.00 : 17/02/2003   Nouveau DB Editor
//                     Utilisation de fichier de ressources s�par�s
//                     Nouveau type de fichier LOC : Decimal Degree
// 7.00  : 04/12/2003 : Correction d'un bug dans tempGen avec le MAX_DAY = 366
//                     Correction d'un bug dans TempGen sur pr�cipitation en temp r�el
//                     Nouveau mod�le avec TGInput et Documentation
//                     Possibiliter de mod�le bilingue
//                     Nouveau point d'entr� dans les dll DoSimulation2
// 6.61 : 18/11/2002 : ???
// 6.60 : 24/10/2002 : Ajout de la radiation dans tempGen
// 6.54 : 17/10/2002 : nouvelle version 
// 6.53 : 06/09/2002 : nouveau CBinaryMap pour prendre les ShortGrids
//                     Correction d'un bug dans la lecture des msb 
// 6.53 : 15/08/2002 : Support pour les fichier .prj   
// 6.52 : Dans TempGen, inverser realMin et realMax si min plus grand que max.
// 6.51 : Modification de TempGenKernel par Jaques.
// 6.5  : Ajout du nombre d'ann�e � simuler dans le model. modif de tempgen
//        pour pouvoir simuler sur plusieur ann�es.
// 6.4  : Ajout de deux options dans l'�laboration des models: Le point de localisation et le num�ro de l'it�ration
//        BioSIM prend maintenant les ex�cutable DOS sans afficher de console    
// 6.33 : J'ai changer la valeur du R� pour ajouter un terme � la r�gression
//        La valeur est passer de 0.01 � 0.005. De plus un champ a �t� ajout� dans les options.
// 6.32 : Correction d'un bug dans UtilWin::Equal, corrige un probl�me dans la s�lection des analyses
// 6.31 : Radio au lieu de checkbox dans le dialog des matchs stations

#include "stdafx.h" 
//#include "VisualLeakDetector\include\vld.h"


#include "ANN/ANN.h"
#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "Geomatic/UtilGDAL.h"
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
//#include "afxres.h"




//
//int testTimeZone(void)
//{
//	//cctz::TimeZoneInfo::LoadTimeZone(name);
//
//	cctz::time_zone syd;
//	if (!cctz::load_time_zone("Australia/Sydney", &syd)) 
//		return -1;
//
//	// Neil Armstrong first walks on the moon
//	const auto tp1 = cctz::convert(cctz::civil_second(1969, 7, 21, 12, 56, 0), syd);
//
//	std::string s = cctz::format("%x %X", tp1, syd);
//	//std::cout << s << "\n";
//
//	cctz::time_zone nyc;
//	cctz::load_time_zone("America/New_York", &nyc);
//
//	const auto tp2 = cctz::convert(cctz::civil_second(1969, 7, 20, 22, 56, 0), nyc);
//	s = cctz::format("%x %X", tp2, syd);
//	
//
//	cctz::time_zone lax;
//	load_time_zone("America/Los_Angeles", &lax);
//	const auto now = std::chrono::system_clock::now();
//	const auto day = FloorDay(now, lax);
//	s = cctz::format("Now: %x %X\n", now, lax);
//	s = cctz::format("Day: %x %X\n", day, lax);
//
//
//	
//
//
//	return tp2 == tp1 ? 0 : 1;
//}

using namespace UtilWin;
using namespace WBSF;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBioSIMApp

BEGIN_MESSAGE_MAP(CBioSIMApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CBioSIMApp::OnAppAbout)
	// Standard file based document commands
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
			//to correct a old bug
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
	GetContextMenuManager()->AddMenu(_T("Popup"), IDR_POPUP);
	GetContextMenuManager()->AddMenu(_T("Edit1"), IDR_MENU_EDIT);
	
}

void CBioSIMApp::LoadCustomState()
{
	
}

void CBioSIMApp::SaveCustomState()
{
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

	if( !pDX->m_bSaveAndValidate )
	{
		
		CString filepath;
		GetModuleFileNameW(GetModuleHandle(NULL), filepath.GetBuffer(MAX_PATH), MAX_PATH);
		filepath.ReleaseBuffer();


		CString version = UtilWin::GetVersionString(filepath);
		CString date = UtilWin::GetCompilationDateString(__DATE__);
		m_versionCtrl.SetWindowText( version + _T(" (") + date + _T(") 64 bits") );
	}
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

