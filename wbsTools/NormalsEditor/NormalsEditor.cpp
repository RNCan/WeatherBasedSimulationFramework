// 4.4.6	20/12/2021	R�mi Saint-Amant	Compile with GDAL 3.0.3
// 4.4.5	02/08/2021	R�mi Saint-Amant	Compile with VC 2019 and MFC dll version
// 4.4.4	16/06/2021	R�mi Saint-Amant	Correction in grid cell height
// 4.4.3	02/02/2020	R�mi Saint-Amant	Update
// 4.4.2	24/05/2091	R�mi Saint-Amant	Update
// 4.4.0	13/09/2018	R�mi Saint-Amant	Compile with WBSF 1.1.0 (BioSIM 11.5)
// 4.3.2	12/06/2018	R�mi Saint-Amant	Some fix
// 4.3.1	19/03/2018	R�mi Saint-Amant	Compile with VS 2017 
// 4.3.0	09/02/2018	R�mi Saint-Amant	Add shore distance
// 4.2.2	09/01/2018	R�mi Saint-Amant	Remove LANGUAGE 9, 1. 
// 4.2.1    10/10/2017  R�mi Saint-Amant	Recompilation from backup after hard drive crash
// 4.2.0 	23/11/2016	R�mi Saint-Amant	New database     
// 4.1.1 	01/11/2016	R�mi Saint-Amant	New database with .csv file 
// 4.1.0	20/09/2016	R�mi Saint-Amant	Change Tair and Trng by Tmin and Tmax   
// 4.0.1				R�mi Saint-Amant	Compile with WBSF   
// 4.0.0				R�mi Saint-Amant	Initial version     from Normals editor 

#include "stdafx.h"
//#include "VisualLeakDetector\include\vld.h" 
#include "afxwinappex.h" 
#include "afxdialogex.h"
#include "NormalsEditor.h" 
#include "MainFrm.h"

#include "Basic/Shore.h"
#include "NormalsEditorDoc.h"
#include "OutputView.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/AboutDlg.h"
#include "basic/Registry.h"
#include "basic/DynamicRessource.h"
#include "WeatherBasedSimulationUI.h"
#include <gdiplus.h>

using namespace Gdiplus;
using namespace WBSF;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

 
// CNormalsEditorApp

BEGIN_MESSAGE_MAP(CNormalsEditorApp, CWinAppEx) 
	ON_COMMAND(ID_APP_ABOUT, &CNormalsEditorApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// construction CNormalsEditorApp

CNormalsEditorApp::CNormalsEditorApp() :m_nGdiplusToken(0)
{
	SetDllDirectory(CString((GetApplicationPath() + "External").c_str()));
	m_bHiColorIcons = TRUE;
	SetAppID(_T("NRCan.NormalsEditor.4"));
}

// Seul et unique objet CNormalsEditorApp

CNormalsEditorApp theApp;


// initialisation de CNormalsEditorApp

BOOL CNormalsEditorApp::InitInstance()
{
	WBSF::CRegistry registre;

	int lang = registre.GetLanguage();
	HINSTANCE hInst = NULL;

	if (lang == WBSF::CRegistry::FRENCH)
	{
		hInst = LoadLibraryW(L"NormalsEditorFrc.dll");
		if (hInst != NULL)
			AfxSetResourceHandle(hInst);
	}
	

	CDynamicResources::set(AfxGetResourceHandle());

	// InitCommonControlsEx() est requis sur Windows�XP si le manifeste de l'application
	// sp�cifie l'utilisation de ComCtl32.dll version�6 ou ult�rieure pour activer les
	// styles visuels.  Dans le cas contraire, la cr�ation de fen�tres �chouera.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	if (!CWinAppEx::InitInstance())
		return FALSE;

	

	EnableTaskbarInteraction(FALSE);
	SetRegistryKey(_T("NRCan"));
	LoadStdProfileSettings(8);  // Charge les options de fichier INI standard (y compris les derniers fichiers utilis�s)


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

	// Inscrire les mod�les de document de l'application.  Ces mod�les
	//  lient les documents, fen�tres frame et vues entre eux
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CNormalsEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // fen�tre frame SDI principale
		RUNTIME_CLASS(COutputView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Activer les ouvertures d'ex�cution DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	ERMsg msg;
	msg += CShore::SetShore(GetApplicationPath() + "Layers/shore.ann");

	if (!msg)
		UtilWin::SYShowMessage(msg, AfxGetMainWnd());


	// Commandes de dispatch sp�cifi�es sur la ligne de commande.  Retournent FALSE si
	// l'application a �t� lanc�e avec /RegServer, /Register, /Unregserver ou /Unregister.
	// Analyser la ligne de commande pour les commandes shell standard, DDE, ouverture de fichiers
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// La seule fen�tre a �t� initialis�e et peut donc �tre affich�e et mise � jour
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();



	return TRUE;
}


// Commande App pour ex�cuter la bo�te de dialogue
void CNormalsEditorApp::OnAppAbout()
{
	UtilWin::CAboutDlg aboutDlg(AFX_IDS_APP_TITLE); 
	aboutDlg.m_lastBuild = CStringA(WBSF::GetCompilationDateString(__DATE__).c_str());
	aboutDlg.DoModal();
}


void CNormalsEditorApp::PreLoadState()
{
	GetContextMenuManager()->AddMenu(_T("Edit1"), IDR_MENU_EDIT);
}

void CNormalsEditorApp::LoadCustomState()
{
}

void CNormalsEditorApp::SaveCustomState()
{
}


int CNormalsEditorApp::ExitInstance()
{
	CMFCToolBar::CleanUpImages();
	Gdiplus::GdiplusShutdown(m_nGdiplusToken);

	return CWinApp::ExitInstance();
}

