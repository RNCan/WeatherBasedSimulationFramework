// HourlyEditor.cpp : Définit les comportements de classe pour l'application.  
// 4.3.2	12/06/2018	Rémi Saint-Amant	Show data event when elev = -999
// 4.3.1	10/03/2018	Rémi Saint-Amant	Compile with VS 2017
// 4.3.0	09/02/2018	Rémi Saint-Amant	Add shore distance
// 4.2.2	09/01/2018	Rémi Saint-Amant	Remove LANGUAGE 9, 1.
// 4.2.1    10/10/2017  Rémi Saint-Amant	Recompilation from backup after hard drive crash
// 4.2.0	17/11/2016	Rémi Saint-Amant	Add 'H' to data directory 
// 4.1.1	01/11/2016	Rémi Saint-Amant	New databse with .csv file  
// 4.1.0	20/09/2016	Rémi Saint-Amant	Change Tair and Trng by Tmin and Tmax
// 4.0.3				Rémi Saint-Amant	Compile with WBSF 
// 4.0.2				Rémi Saint-Amant	unordered SSI in location. Bug correction in weather station format.
// 4.0.1				Rémi Saint-Amant	Change in CLocation.   
// 4.0.0				Rémi Saint-Amant	Initial version       

#include "stdafx.h"
//#include "..\VisualLeakDetector\include\vld.h" 
#include "HourlyEditor.h" 
#include "MainFrm.h"
#include "OutputView.h"

#include "HourlyEditorDoc.h"
#include "Basic/Shore.h"
#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/AboutDlg.h"
#include "WeatherBasedSimulationUI.h"

#include <gdiplus.h>


using namespace Gdiplus;
using namespace WBSF;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

 
// CHourlyEditorApp

BEGIN_MESSAGE_MAP(CHourlyEditorApp, CWinAppEx) 
	ON_COMMAND(ID_APP_ABOUT, &CHourlyEditorApp::OnAppAbout)
	// Commandes de fichier standard
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Commande standard de configuration de l'impression
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// construction CHourlyEditorApp

CHourlyEditorApp::CHourlyEditorApp() :
	m_nGdiplusToken(0)
{
	SetDllDirectory(CString((GetApplicationPath() + "External").c_str()));
	m_bHiColorIcons = TRUE;
	SetAppID(_T("NRCan.HourlyEditor.4"));
}

// Seul et unique objet CHourlyEditorApp
CHourlyEditorApp theApp;


// initialisation de CHourlyEditorApp
BOOL CHourlyEditorApp::InitInstance()
{

	WBSF::CRegistry registre;

	int lang = registre.GetLanguage();
	HINSTANCE hInst = NULL;

	if (lang == WBSF::CRegistry::FRENCH)
	{
		hInst = LoadLibraryW(L"HourlyEditorFrc.dll");
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
		RUNTIME_CLASS(CHourlyEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // fenêtre frame SDI principale
		RUNTIME_CLASS(COutputView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);



	// Activer les ouvertures d'exécution DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	
	ERMsg msg;
	msg += CShore::SetShore(GetApplicationPath() + "Layers/shore.ann");

	if (!msg)
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());



	// Commandes de dispatch spécifiées sur la ligne de commande.  Retournent FALSE si
	// l'application a été lancée avec /RegServer, /Register, /Unregserver ou /Unregister.
	// Analyser la ligne de commande pour les commandes shell standard, DDE, ouverture de fichiers
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
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
void CHourlyEditorApp::OnAppAbout()
{
	UtilWin::CAboutDlg aboutDlg(AFX_IDS_APP_TITLE); 
	aboutDlg.m_lastBuild = CStringA(WBSF::GetCompilationDateString(__DATE__).c_str());
	aboutDlg.DoModal();
}

// CHourlyEditorApp, méthodes de chargement/d'enregistrement de la personnalisation

void CHourlyEditorApp::PreLoadState()
{
	GetContextMenuManager()->AddMenu(_T("Edit1"), IDR_MENU_EDIT);
}

void CHourlyEditorApp::LoadCustomState()
{
}

void CHourlyEditorApp::SaveCustomState()
{
}

// gestionnaires de messages pour CHourlyEditorApp





int CHourlyEditorApp::ExitInstance()
{
	CMFCToolBar::CleanUpImages();
	Gdiplus::GdiplusShutdown(m_nGdiplusToken);

	return CWinApp::ExitInstance();
}

