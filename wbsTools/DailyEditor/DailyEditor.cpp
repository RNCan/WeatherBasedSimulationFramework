// 4.3.2	12/06/2018	Rémi Saint-Amant	bug fix
// 4.3.1	19/03/2018	Rémi Saint-Amant	Compile with VS 2017
// 4.3.0	09/02/2018	Rémi Saint-Amant	Add shore distance
// 4.2.1    10/10/2017  Rémi Saint-Amant	Recompilation from backup after hard drive crash
// 4.2.0	27/11/2016	Rémi Saint-Amant	Add 'D' at the end of data directory and use .DailyDB 
// 4.1.1	01/11/2016	Rémi Saint-Amant	save station list in .csv file
// 4.1.0	20/09/2016	Rémi Saint-Amant	Change Tair and Trng by Tmin and Tmax   
// 4.0.4				Rémi Saint-Amant	Compile with WBSF  
// 4.0.3				Rémi Saint-Amant	Put spreadsheet and charts into CDockablePane
// 4.0.2				Rémi Saint-Amant	unordored SSI and bug correction
// 4.0.1				Rémi Saint-Amant	Change in CLocation.  
// 4.0.0				Rémi Saint-Amant	Initial version     from hourly editor 
 
#include "stdafx.h" 
//#include "..\VisualLeakDetector\include\vld.h"  
#include <gdiplus.h>
#include <afxwinappex.h> 
#include <afxdialogex.h>

#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "Basic/Shore.h"
#include "UI/Common/SYShowMessage.h"

#include "UI/Common/AboutDlg.h"
#include "WeatherBasedSimulationUI.h"

#include "DailyEditor.h" 
#include "DailyEditorDoc.h"
#include "MainFrm.h"
#include "OutputView.h"


using namespace Gdiplus;
using namespace WBSF;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

 
// CDailyEditorApp

BEGIN_MESSAGE_MAP(CDailyEditorApp, CWinAppEx) 
	ON_COMMAND(ID_APP_ABOUT, &CDailyEditorApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// construction CDailyEditorApp

CDailyEditorApp::CDailyEditorApp() :	m_nGdiplusToken(0)
{
	SetDllDirectory(CString((GetApplicationPath() + "External").c_str()));
	//m_bHiColorIcons = TRUE;
	SetAppID(_T("NRCan.DailyEditor.4"));
}

// Seul et unique objet CDailyEditorApp

CDailyEditorApp theApp;


// initialisation de CDailyEditorApp

BOOL CDailyEditorApp::InitInstance()
{

	WBSF::CRegistry registre;

	int lang = registre.GetLanguage();
	HINSTANCE hInst = NULL;

	if (lang == WBSF::CRegistry::FRENCH)
	{
		hInst = LoadLibraryW(L"DailyEditorFrc.dll");
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

	GdiplusStartupInput gdiplusStartupInput;
	if (Gdiplus::GdiplusStartup(&m_nGdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok)
	{
		MessageBox(NULL, TEXT("GDI+ failed to start up!"), TEXT("Error!"), MB_ICONERROR);
		return FALSE;
	}

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
		RUNTIME_CLASS(CDailyEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // fenêtre frame SDI principale
		RUNTIME_CLASS(COutputView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	if (CShore::GetShore().get() == NULL)
	{
		ERMsg msg;
		msg += CShore::SetShore(GetApplicationPath() + "Layers/shore.ann");

		if (!msg)
			UtilWin::SYShowMessage(msg, NULL);
	}

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
	m_pMainWnd->DragAcceptFiles();
	return TRUE;
}


// Commande App pour exécuter la boîte de dialogue
void CDailyEditorApp::OnAppAbout()
{
	UtilWin::CAboutDlg aboutDlg(AFX_IDS_APP_TITLE); 
	aboutDlg.m_lastBuild = CStringA(WBSF::GetCompilationDateString(__DATE__).c_str());
	aboutDlg.DoModal();
}


void CDailyEditorApp::PreLoadState()
{
	GetContextMenuManager()->AddMenu(_T("Edit1"), IDR_MENU_EDIT);
}

void CDailyEditorApp::LoadCustomState()
{
}

void CDailyEditorApp::SaveCustomState()
{
}


int CDailyEditorApp::ExitInstance()
{
	CMFCToolBar::CleanUpImages();//to avoid crash at exit???
	Gdiplus::GdiplusShutdown(m_nGdiplusToken);

	return CWinApp::ExitInstance();
}

