// 4.0.3	R�mi Saint-Amant	Compile with WBSF
// 4.0.3	R�mi Saint-Amant	Put spreadsheet and charts into CDockablePane
// 4.0.2	R�mi Saint-Amant	unordored SSI and bug correction
// 4.0.1	R�mi Saint-Amant	Change in CLocation.  
// 4.0.0	R�mi Saint-Amant	Initial version     from hourly editor 
 
#include "stdafx.h"
//#include "..\VisualLeakDetector\include\vld.h" 
#include <gdiplus.h>
#include <afxwinappex.h> 
#include <afxdialogex.h>

#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"

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
	// Commandes de fichier standard
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Commande standard de configuration de l'impression
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
END_MESSAGE_MAP()


// construction CDailyEditorApp

CDailyEditorApp::CDailyEditorApp() :
	m_nGdiplusToken(0)
{
	SetDllDirectory(CString((GetApplicationPath() + "External").c_str()));
	m_bHiColorIcons = TRUE;
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

	// InitCommonControlsEx() est requis sur Windows�XP si le manifeste de l'application
	// sp�cifie l'utilisation de ComCtl32.dll version�6 ou ult�rieure pour activer les
	// styles visuels.  Dans le cas contraire, la cr�ation de fen�tres �chouera.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// � d�finir pour inclure toutes les classes de contr�les communs � utiliser
	// dans votre application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	EnableTaskbarInteraction(FALSE);
	
	SetRegistryKey(_T("NRCan"));
	LoadStdProfileSettings(8);  // Charge les options de fichier INI standard (y compris les derniers fichiers utilis�s)

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

	// Inscrire les mod�les de document de l'application.  Ces mod�les
	//  lient les documents, fen�tres frame et vues entre eux
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CDailyEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // fen�tre frame SDI principale
		RUNTIME_CLASS(COutputView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);


	// Analyser la ligne de commande pour les commandes shell standard, DDE, ouverture de fichiers
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Activer les ouvertures d'ex�cution DDE
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);


	// Commandes de dispatch sp�cifi�es sur la ligne de commande.  Retournent FALSE si
	// l'application a �t� lanc�e avec /RegServer, /Register, /Unregserver ou /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// La seule fen�tre a �t� initialis�e et peut donc �tre affich�e et mise � jour
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	m_pMainWnd->DragAcceptFiles();
	return TRUE;
}


// Commande App pour ex�cuter la bo�te de dialogue
void CDailyEditorApp::OnAppAbout()
{
	UtilWin::CAboutDlg aboutDlg(AFX_IDS_APP_TITLE); 
	aboutDlg.m_lastBuild = CStringA(WBSF::GetCompilationDateString(__DATE__).c_str());
	aboutDlg.DoModal();
}

// CDailyEditorApp, m�thodes de chargement/d'enregistrement de la personnalisation

void CDailyEditorApp::PreLoadState()
{
	//BOOL bNameValid;
	//CString strName;
	//ameValid = strName.LoadString(IDS_EDIT_MENU);
	//ASSERT(bNameValid);
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
	GdiplusShutdown(m_nGdiplusToken);
	return CWinAppEx::ExitInstance();
}
