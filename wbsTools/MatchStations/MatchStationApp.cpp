
// MatchStation.cpp : Définit les comportements de classe pour l'application.
// 
// 4.0.0	Rémi Saint-Amant	Initial version   

#include "stdafx.h"
//#include "..\VisualLeakDetector\include\vld.h" 
#include <gdiplus.h>
#include <afxwinappex.h>
#include <afxdialogex.h>

#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "UI/Common/AboutDlg.h"

#include "MatchStationApp.h" 
#include "MainFrm.h"
#include "MatchStationDoc.h"
#include "LocationsListView.h"

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
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Inscrire les modèles de document de l'application.  Ces modèles
	//  lient les documents, fenêtres frame et vues entre eux
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMatchStationDoc),
		RUNTIME_CLASS(CMainFrame),       // fenêtre frame SDI principale
		RUNTIME_CLASS(CLocationsListView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Analyser la ligne de commande pour les commandes shell standard, DDE, ouverture de fichiers
//	CMatchStationCmdLine cmdInfo;
	ParseCommandLine(m_cmdInfo);

	// Activer les ouvertures d'exécution DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// Commandes de dispatch spécifiées sur la ligne de commande.  Retournent FALSE si
	// l'application a été lancée avec /RegServer, /Register, /Unregserver ou /Unregister.
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
	GdiplusShutdown(m_nGdiplusToken);
	return CWinAppEx::ExitInstance();
}
