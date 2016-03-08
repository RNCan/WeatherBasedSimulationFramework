//5.0.0 01/03/2016	New version with the WBSF and new user interface
//4.1.2 13/11/2015	Add use of TRng in weatehr database, Add BC PAWS and Snowtel
//4.1.1	21/07/2015	Add historical Radar image download
//4.1.0	15/07/2015	Add Radar image download
//4.0.9	24/06/2015	Add Extract loc from weather database and generalyze loc
//4.0.8 07/05/2015  Add loop in the Mesonet-Quebec updater, use dot by default as decimal séparator
//					Many bugs correction in the NormalFromDaily algo
//4.0.6	07/04/2015	Bug correction with oe
//4.0.5 29/03/2015  Bug correction in weather Québec. Now load more than 1 year.
//4.0.4 17/03/2015	Many bugs corrections
//4.0.3 15/01/2015	Bug correction in relative path 
//4.0.0	15/01/2015	New WeatherStations struct. New name. WeatherUpdater
//3.0.5 15/10/2014	Bug correction in EnvCan elevation highert than 1000 meters
//3.0.5 15/10/2014	Bug correction in ISD-Lite and GSOD
//3.0.4 06/10/2014  New ISD-Lite and GSOD station list
//3.0.3 08/08/2014	New province abreviation for Canada
//3.0.2 16/07/2014	Add GCM10km climatic change task. some bug correction.
//3.0.1 06/06/2014	Add new Env. Can. hourly forecast.  
//3.0.0 16/03/2014	Completly new version in 64 bits with VS 2013.
//2.5.4 30/07/2013	Update with New Environment Canada site.
//2.5.2 22/05/2013	Add date in the forecast info
//2.5.1 06/05/2013	Add, SMEX et SWEB. New forecast.
//2.5.0 27/03/2013  Add IDS-Lite 
//2.4.2 24/07/2012  Add loc extractor 
//2.4.1 09/07/2012  New CRU TS 3.1 extractor
//2.4.0 07/06/2012  New MMG file and OURANOS data extraction.
//2.3.1 07/05/2012  Correction of a bug into the forecast of UIRNQue
//2.3.0 05/12/2011  New progress bar dialog. Add 20th reanalysis.
//2.2.1 26/09/2011  Some little change. Add UpdateCoordinate to get accurate coordinate.
//2.2.0	03/06/2011	New Environment Canada database. Downloading .csv format
//2.1.4 18/05/2011  Copy MRCQ and SOPFIM file event if the zip exist.
//2.1.3 17/05/2011  Correcting a problem with EnvCan page. TODO : hourly
//1.8.2 23/04/2010  Follow change in EnvCan latitude and longitude change
//1.8.1 07/04/2010	Correction of a bug in NOAA.
//1.8.0 06/02/2010	Add FAOCLim and GHCND task. New normal merge algo.
//1.7.3 27/05/2009	Add Europe paleoClimatic task
//1.7.2 14/05/2009  Add Zipping and copy to FTP server task
//1.7.1 12/05/2009	Add forecast to MRNF québec weatehr station. Chnage structure of the list file
//1.7.0 28/04/2009	Add forecast to Environnement Canada. EC HTML Update.
//1.6.4	24/03/2009	OURANOS Climatic change MRCC
//1.6.3 26/01/2009	Correction of a bug in the donwload of Québec weather
//1.6.2 06/01/2009	Put the FTP actif and not passif. 
//1.6.1 09/10/2008  Compile with Visual studio 2008.
//1.6.0 01/02/2008  New NOAA download from FTP site and packDB
//					Correction of a bug in the EnvCan pack DB and the complet flag
//1.4.1 13/09/2007  New SnowTel download
//					Corection of a problem with focus window and menu
//					Add Copy/Paste functionnality
//					Solve memory leak problem with vld.h
//1.3.0	05/04/2007	New LOC file format

#include "stdafx.h"
//#include "VisualLeakDetector\include\vld.h" 
#include "afxwinappex.h" 
#include "afxdialogex.h"
#include "WeatherUpdater.h" 
#include "MainFrm.h"

#include "WeatherUpdaterDoc.h"
#include "ProjectView.h"
#include "UI/Common/AboutDlg.h"
#include "basic/Registry.h"
#include "basic/DynamicRessource.h"

#include <gdiplus.h>
using namespace Gdiplus;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

 
// CWeatherUpdaterApp

BEGIN_MESSAGE_MAP(CWeatherUpdaterApp, CWinAppEx) 
	ON_COMMAND(ID_APP_ABOUT, &CWeatherUpdaterApp::OnAppAbout)
	// Commandes de fichier standard
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Commande standard de configuration de l'impression
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// construction CWeatherUpdaterApp

CWeatherUpdaterApp::CWeatherUpdaterApp() :m_nGdiplusToken(0)
{
	m_bHiColorIcons = TRUE;

	// prend en charge le Gestionnaire de redémarrage
	//m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// Si l'application est créée à l'aide de la prise en charge Common Language Runtime (/clr):
	//     1) Ce paramètre supplémentaire est nécessaire à la prise en charge du Gestionnaire de redémarrage.
	//     2) Dans votre study, vous devez ajouter une référence à System.Windows.Forms pour la génération.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: remplacer la chaîne d'ID de l'application ci-dessous par une chaîne ID unique ; le format recommandé
	// pour la chaîne est CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("NRCan.WeatherUpdater.11"));
	
}

// Seul et unique objet CWeatherUpdaterApp

CWeatherUpdaterApp theApp;


// initialisation de CWeatherUpdaterApp

BOOL CWeatherUpdaterApp::InitInstance()
{

	WBSF::CRegistry registre;

	int lang = registre.GetLanguage();
	HINSTANCE hInst = NULL;

	if (lang == WBSF::CRegistry::FRENCH)
	{
		hInst = LoadLibraryW(L"WeatherUpdaterFrc.dll");

		if (hInst != NULL)
			AfxSetResourceHandle(hInst);
	}

	CDynamicResources::set(AfxGetResourceHandle());

	// InitCommonControlsEx() est requis sur Windows XP si le manifeste de l'application
	// spécifie l'utilisation de ComCtl32.dll version 6 ou ultérieure pour activer les
	// styles visuels.  Dans le cas contraire, la création de fenêtres échouera.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// À définir pour inclure toutes les classes de contrôles communs à utiliser
	// dans votre application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() est requis pour utiliser un contrôle RichEdit	
	// AfxInitRichEdit2();

	// Initialisation standard
	// Si vous n'utilisez pas ces fonctionnalités et que vous souhaitez réduire la taille
	// de votre exécutable final, vous devez supprimer ci-dessous
	// les routines d'initialisation spécifiques dont vous n'avez pas besoin.
	// Changez la clé de Registre sous laquelle nos paramètres sont enregistrés
	// TODO: modifiez cette chaîne avec des informations appropriées,
	// telles que le nom de votre société ou organisation
	SetRegistryKey(_T("NRCan"));
	LoadStdProfileSettings(8);  // Charge les options de fichier INI standard (y compris les derniers fichiers utilisés)

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_nGdiplusToken, &gdiplusStartupInput, NULL);

	//set local to default operating system
	static std::locale THE_LOCALE(std::locale(".ACP"), std::locale::classic(), std::locale::numeric);
	std::locale::global(THE_LOCALE);

	setlocale(LC_ALL, ".ACP");
	setlocale(LC_NUMERIC, "English");

	InitContextMenuManager();
	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Inscrire les modèles de document de l'application.  Ces modèles
	//  lient les documents, fenêtres frame et vues entre eux
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CWeatherUpdaterDoc),
		RUNTIME_CLASS(CMainFrame),       // fenêtre frame SDI principale
		RUNTIME_CLASS(CProjectView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Analyser la ligne de commande pour les commandes shell standard, DDE, ouverture de fichiers
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Activer les ouvertures d'exécution DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// Commandes de dispatch spécifiées sur la ligne de commande.  Retournent FALSE si
	// l'application a été lancée avec /RegServer, /Register, /Unregserver ou /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// La seule fenêtre a été initialisée et peut donc être affichée et mise à jour
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	// appelle DragAcceptFiles uniquement s'il y a un suffixe
	//  Dans une application SDI, cet appel doit avoir lieu juste après ProcessShellCommand
	// Activer les ouvertures via glisser-déplacer
	m_pMainWnd->DragAcceptFiles();
	return TRUE;
}


// Commande App pour exécuter la boîte de dialogue
void CWeatherUpdaterApp::OnAppAbout()
{
	UtilWin::CAboutDlg aboutDlg(AFX_IDS_APP_TITLE); 
	aboutDlg.m_lastBuild = CStringA(WBSF::GetCompilationDateString(__DATE__).c_str());
	//aboutDlg.m_extra = _T("Collaboration : \t\tPierre Duval");
	aboutDlg.DoModal();
}

// CWeatherUpdaterApp, méthodes de chargement/d'enregistrement de la personnalisation

void CWeatherUpdaterApp::PreLoadState()
{
	///BOOL bNameValid;
	//CString strName;
	//bNameValid = strName.LoadString(IDS_EDIT_MENU);
	//ASSERT(bNameValid);
	//GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CWeatherUpdaterApp::LoadCustomState()
{
}

void CWeatherUpdaterApp::SaveCustomState()
{
}

// gestionnaires de messages pour CWeatherUpdaterApp





int CWeatherUpdaterApp::ExitInstance()
{
	GdiplusShutdown(m_nGdiplusToken);
	return CWinAppEx::ExitInstance();
}

