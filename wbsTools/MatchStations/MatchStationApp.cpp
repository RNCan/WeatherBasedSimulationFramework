// MatchStation.cpp : Définit les comportements de classe pour l'application. 
// 4.4.0	13/09/2018	Rémi Saint-Amant	Compile with WBSF 1.1.0 (BioSIM 11.5)
// 4.3.2	20/06/2018	Rémi Saint-Amant	bug fix with shore message
// 4.3.1	19/03/2018	Rémi Saint-Amant	Compile with VS 2017
//											Add color in weight chart
// 4.3.0	09/02/2018	Rémi Saint-Amant	Add shore distance and prcp gradient
// 4.2.2	09/01/2018	Rémi Saint-Amant	Remove LANGUAGE 9, 1. 
// 4.2.1    10/10/2017  Rémi Saint-Amant	Recompilation from backup after hard drive crash
// 4.2.0    23/11/2016	Rémi Saint-Amant	New database 
//											Add SearchRadius and SkipVerify options
// 4.1.1    01/11-2016	Rémi Saint-Amant	New database with .csv file 
// 4.1.0    20/09/2016	Rémi Saint-Amant	Replace Tair and Trng by Tmin and Tmax 
// 4.0.1				Rémi Saint-Amant	Bug correction in gradient 
// 4.0.0	            Rémi Saint-Amant	Initial version 

#include "stdafx.h"
//#include "..\VisualLeakDetector\include\vld.h" 
#include <gdiplus.h>
#include <afxwinappex.h>
#include <afxdialogex.h>

#include "Basic/Registry.h"
#include "Basic/ANN/Ann.h"
#include "Basic/DynamicRessource.h"
#include "Basic/Shore.h"
#include "UI/Common/AboutDlg.h"
#include "UI/Common/SYShowMessage.h"

#include "MatchStationApp.h" 
#include "MainFrm.h"
#include "MatchStationDoc.h"
#include "OutputView.h"

using namespace Gdiplus;
using namespace WBSF;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

	// CMatchStationApp

BEGIN_MESSAGE_MAP(CMatchStationApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CMatchStationApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// construction CMatchStationApp.h

CMatchStationApp::CMatchStationApp() :
	m_nGdiplusToken(0)
{
	SetDllDirectory(CString((GetApplicationPath() + "External").c_str()));
	m_bHiColorIcons = TRUE;
	SetAppID(_T("NRCan.MatchStation.4"));

}

// Seul et unique objet CMatchStationApp.h

CMatchStationApp theApp;


// initialisation de CMatchStationApp

BOOL CMatchStationApp::InitInstance()
{
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&m_nGdiplusToken, &gdiplusStartupInput, NULL);

	CRegistry registre;

	int lang = registre.GetLanguage();
	HINSTANCE hInst = NULL;

	if (lang == CRegistry::FRENCH)
	{
		hInst = LoadLibraryW(L"MatchStationFrc.dll");

		if (hInst != NULL)
			AfxSetResourceHandle(hInst);
	}

	CDynamicResources::set(AfxGetResourceHandle());

	// InitCommonControlsEx() est requis sur Windows XP si le manifeste de l'application
	// spécifie l'utilisation de ComCtl32.dll version 6 ou ultérieure pour activer les
	// styles visuels.  Dans le cas contraire, la création de fenêtres échouera.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	EnableTaskbarInteraction(FALSE);
	SetRegistryKey(_T("NRCan"));
	LoadStdProfileSettings(8);  // Charge les options de fichier INI standard (y compris les derniers fichiers utilisés)



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
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Inscrire les modèles de document de l'application.  Ces modèles
	//  lient les documents, fenêtres frame et vues entre eux
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMatchStationDoc),
		RUNTIME_CLASS(CMainFrame),       // fenêtre frame SDI principale
		RUNTIME_CLASS(COutputView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);



	// Activer les ouvertures d'exécution DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	ERMsg msg = CShore::SetShore(GetApplicationPath() + "Layers/shore.ann");
		
	if (!msg)
		UtilWin::SYShowMessage(msg, NULL);


	// Commandes de dispatch spécifiées sur la ligne de commande.  Retournent FALSE si
	// l'application a été lancée avec /RegServer, /Register, /Unregserver ou /Unregister.
	ParseCommandLine(m_cmdInfo);
	if (!ProcessShellCommand(m_cmdInfo))
		return FALSE;

	// La seule fenêtre a été initialisée et peut donc être affichée et mise à jour
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();

	return TRUE;
}


// Commande App pour exécuter la boîte de dialogue
void CMatchStationApp::OnAppAbout()
{
	UtilWin::CAboutDlg aboutDlg(AFX_IDS_APP_TITLE);
	aboutDlg.m_lastBuild = CStringA(GetCompilationDateString(__DATE__).c_str());
	aboutDlg.DoModal();
}

// CMatchStationApp, méthodes de chargement/d'enregistrement de la personnalisation

void CMatchStationApp::PreLoadState()
{
	GetContextMenuManager()->AddMenu(_T("Edit1"), IDR_MENU_EDIT);
}

void CMatchStationApp::LoadCustomState()
{}

void CMatchStationApp::SaveCustomState()
{}


int CMatchStationApp::ExitInstance()
{
	annClose();
	CMFCToolBar::CleanUpImages();
	Gdiplus::GdiplusShutdown(m_nGdiplusToken);

	return CWinApp::ExitInstance();
}
