// MainFrm.cpp : implementation of the CMainFrame class
//
#include "stdafx.h"
#include "Basic/Callback.h"
#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "UI/FileManagerPropSheet.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/ProgressStepDlg.h"
#include "UI/Common/SYShowMessage.h"


#include "BioSIMOptionDlg.h"
#include "BioSIM.h"
#include "BioSIMDoc.h"
#include "OutputView.h"
#include "MainFrm.h"


#include "WeatherBasedSimulationUI.h"

#include "UI\LOCGeneratorDlg.h"

using namespace std;
using namespace UtilWin; 
using namespace WBSF;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static const UINT ID_PROJECT_WND        = 500;
static const UINT ID_SPREADSHEET_WND	= 501;
//static const UINT ID_GRAPH_WND			= 502;
static const UINT ID_PROPERTIES_WND		= 503;
static const UINT ID_EXPORT_WND			= 504;

//static const UINT ID_PROGRESS_WND		= 106;

//static const UINT ID_FILE_MANAGER_WND = 156;

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx) 

static UINT WM_CMN_TOOLS_OPTIONS = ::RegisterWindowMessage(_T("WM_CMN_TOOLS_OPTIONS"));

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	//ON_UPDATE_COMMAND_UI_RANGE(ID_SPREADSHEET_WND, ID_OUTPUT_WND, &CFrameWndEx::OnUpdatePaneMenu)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_TOOLS_FILE_MANAGER, &CMainFrame::OnToolsDbEditor)
	ON_COMMAND_EX(ID_TOOLS_TDATE, &CMainFrame::OnToolsRunApp)
	ON_COMMAND_EX(ID_TOOLS_MERGEFILES, &CMainFrame::OnToolsRunApp)
	ON_COMMAND(ID_TOOLS_CLEANUP, &CMainFrame::CleanUp)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_CLEANUP, &CMainFrame::OnUpdateExecute)	

	ON_COMMAND(ID_TOOLS_OPTIONS, &CMainFrame::OnToolsOptions)
	ON_REGISTERED_MESSAGE(WM_CMN_TOOLS_OPTIONS, &CMainFrame::OnToolsOptions2)

	ON_COMMAND(ID_EXECUTE, &CMainFrame::OnExecute)
	ON_UPDATE_COMMAND_UI(ID_EXECUTE, &CMainFrame::OnUpdateExecute)	
	
	ON_COMMAND_RANGE(ID_LANGUAGE_FRENCH, ID_LANGUAGE_ENGLISH, &CMainFrame::OnLanguageChange)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LANGUAGE_FRENCH, ID_LANGUAGE_ENGLISH, &CMainFrame::OnLanguageUI)

	ON_COMMAND_EX(ID_DOWNLOAD_DATA, &CMainFrame::OnHelp)
	ON_COMMAND_EX(ID_HELP_MANUAL, &CMainFrame::OnHelp)
	ON_COMMAND_EX(ID_HELP_TUTORIAL, &CMainFrame::OnHelp)
	ON_UPDATE_COMMAND_UI_RANGE(ID_HOURLY_EDITOR, ID_MATCH_STATION, &CMainFrame::OnUpdateToolbar)
	ON_COMMAND_RANGE(ID_HOURLY_EDITOR, ID_MATCH_STATION, &CMainFrame::OnRunApp)
	ON_REGISTERED_MESSAGE(m_uTaskbarBtnCreatedMsg, OnTaskbarBtnCreated)
END_MESSAGE_MAP()




static UINT indicators[] =
{
	ID_SEPARATOR// status line indicator
};


const UINT CMainFrame::m_uTaskbarBtnCreatedMsg = RegisterWindowMessage ( _T("TaskbarButtonCreated") );
LRESULT CMainFrame::OnTaskbarBtnCreated ( WPARAM wParam, LPARAM lParam )
{
    // On pre-Win 7, anyone can register a message called "TaskbarButtonCreated"
    // and broadcast it, so make sure the OS is Win 7 or later before acting on
    // the message. (This isn't a problem for this app, which won't run on pre-7,
    // but you should definitely do this check if your app will run on pre-7.)
	DWORD dwMajor = LOBYTE(LOWORD(GetVersion()));
	DWORD dwMinor = HIBYTE(LOWORD(GetVersion()));

    // Check that the Windows version is at least 6.1 (yes, Win 7 is version 6.1).
    if ( dwMajor > 6 || ( dwMajor == 6 && dwMinor > 0 ) )
    {
        m_pTaskbarList.Release();
        m_pTaskbarList.CoCreateInstance ( CLSID_TaskbarList );
    }

    return 0;
}


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2005);
	CTabbedPane::m_pTabWndRTC = RUNTIME_CLASS(CMFCTabCtrl24);
}


CMainFrame::~CMainFrame()
{

}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	CDockingManager::SetDockingMode(DT_SMART);
	EnableAutoHidePanes(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);

	//Create menu
	VERIFY(m_wndMenuBar.Create(this));
	m_wndMenuBar.SetMenuSizes(CSize(22, 22), CSize(16, 16));
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndMenuBar.SetRecentlyUsedMenus(FALSE);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	CMFCPopupMenu::SetForceMenuFocus(FALSE);


	//Create Toolbar
	VERIFY(m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_MAINFRAME, 0, 0, TRUE));
	m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);
	DockPane(&m_wndToolBar);


	//Create Status 
	VERIFY(m_wndStatusBar.Create(this));
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	// Create docking pane
	VERIFY(CreateDockingWindows());


	DockPane(&m_projectWnd, AFX_IDW_DOCKBAR_LEFT);
	DockPane(&m_exportWnd, AFX_IDW_DOCKBAR_RIGHT);
	CDockablePane* pPaneFrame = NULL;
	DockPane(&m_spreadsheetWnd, AFX_IDW_DOCKBAR_TOP);
//	m_chartWnd.AttachToTabWnd(&m_spreadsheetWnd, DM_STANDARD, 0, &pPaneFrame);
	m_propertiesWnd.DockToWindow(&m_projectWnd, CBRS_ALIGN_BOTTOM);


	OnApplicationLook(theApp.m_nAppLook);
	EnablePaneMenu(TRUE, ID_VIEW_STATUS_BAR, GetCString(IDS_TOOLBAR_STATUS), ID_VIEW_TOOLBAR);
	LoadBasicCommand();

	return 0;
}


BOOL CMainFrame::CreateDockingWindows()
{
	// Create dockable pane 
	VERIFY(m_projectWnd.Create(GetCString(IDS_PROJECT_WND), this, CRect(0, 0, 250, 400), TRUE, ID_PROJECT_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI));
	m_projectWnd.EnableDocking(CBRS_ALIGN_ANY);


	VERIFY(m_propertiesWnd.Create(GetCString(IDS_PROPERTIES_WND), this, CRect(0, 0, 250, 400), TRUE, ID_PROPERTIES_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI));
	m_propertiesWnd.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(m_spreadsheetWnd.Create(GetCString(IDS_SPREADSHEET_WND), this, CRect(0, 0, 600, 400), TRUE, ID_SPREADSHEET_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI));
	m_spreadsheetWnd.EnableDocking(CBRS_ALIGN_ANY);

	//VERIFY(m_chartWnd.Create(GetCString(IDS_CHARTS_WND), this, CRect(0, 0, 600, 400), TRUE, ID_GRAPH_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI));
	//m_chartWnd.EnableDocking(CBRS_ALIGN_ANY);

	VERIFY(m_exportWnd.Create(GetCString(IDS_EXPORT_WND), this, CRect(0, 0, 250, 400), TRUE, ID_EXPORT_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI));
	m_exportWnd.EnableDocking(CBRS_ALIGN_ANY);

	SetDockingWindowIcons();
	return TRUE;
}


void CMainFrame::SetDockingWindowIcons()
{
	HICON hProjectWnd = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROJECT_WND), IMAGE_ICON, 24, 24, 0);
	m_projectWnd.SetIcon(hProjectWnd, TRUE);

	HICON hPropertiesIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES_WND), IMAGE_ICON, 24, 24, 0);
	m_propertiesWnd.SetIcon(hPropertiesIcon, TRUE);

	HICON hExportIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EXPORT_WND), IMAGE_ICON, 24, 24, 0);
	m_exportWnd.SetIcon(hExportIcon, TRUE);

	HICON hSpreadsheetIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_SPREADSHEET_WND), IMAGE_ICON, 24, 24, 0);
	m_spreadsheetWnd.SetIcon(hSpreadsheetIcon, TRUE);

	//HICON hChartIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CHARTS_WND), IMAGE_ICON, 24, 24, 0);
	//m_chartWnd.SetIcon(hChartIcon, TRUE);

	//HICON hProgressIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROGRESS_WND), IMAGE_ICON, 24, 24, 0);
	//m_progressWnd.SetIcon(hProgressIcon, TRUE);


	//HICON hFMIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CHARTS_WND), IMAGE_ICON, 24, 24, 0);
	//m_fileManagerWnd.SetIcon(hFMIcon, TRUE);

	
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

void CMainFrame::LoadBasicCommand()
{
	// enable menu personalization (most-recently used commands)
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_DUPLICATE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);
	lstBasicCommands.AddTail(ID_PROPERTIES);
	lstBasicCommands.AddTail(ID_ADD_GROUP);
	lstBasicCommands.AddTail(ID_ADD_WEATHER_UPDATE);
	lstBasicCommands.AddTail(ID_ADD_WEATHER_GENERATION);
	lstBasicCommands.AddTail(ID_ADD_MODEL_EXECUTION);
	lstBasicCommands.AddTail(ID_ADD_ANALYSIS);
	lstBasicCommands.AddTail(ID_ADD_FUNCTION_ANALYSIS);
	lstBasicCommands.AddTail(ID_ADD_MERGE);
	lstBasicCommands.AddTail(ID_ADD_MAP);
	lstBasicCommands.AddTail(ID_ADD_IMPORT_SIMULATION);
	lstBasicCommands.AddTail(ID_ADD_INPUT_ANALYSIS);
	lstBasicCommands.AddTail(ID_ADD_DISPERSAL);
	lstBasicCommands.AddTail(ID_ADD_SCRIPT_R);
	lstBasicCommands.AddTail(ID_ADD_COPY_EXPORT);
	lstBasicCommands.AddTail(ID_ADD_MODEL_PARAMETERIZATION);
	lstBasicCommands.AddTail(ID_EDIT);
	lstBasicCommands.AddTail(ID_MATCH_STATION);
	lstBasicCommands.AddTail(ID_REMOVE);
	lstBasicCommands.AddTail(ID_EXPORT_SPREADSHEET1);

	lstBasicCommands.AddTail(ID_EXPORT_SPREADSHEET2);
	lstBasicCommands.AddTail(ID_EXECUTE);
	lstBasicCommands.AddTail(ID_EXECUTE_MENU);
	lstBasicCommands.AddTail(ID_TOOLS_FILE_MANAGER);
	lstBasicCommands.AddTail(ID_SHOWLOC);
	lstBasicCommands.AddTail(ID_TOOLS_OPTIONS);
	lstBasicCommands.AddTail(ID_ADD_EXPORT);
	lstBasicCommands.AddTail(ID_ADD_GRAPH);
	lstBasicCommands.AddTail(ID_EXPORT_NOW);
	lstBasicCommands.AddTail(ID_EXPORT_OPEN_DIRECTORY);
	lstBasicCommands.AddTail(ID_ITEM_EXPAND_ALL);
	lstBasicCommands.AddTail(ID_ITEM_COLLAPSE_ALL);
	lstBasicCommands.AddTail(ID_SHOWMAPS);
	lstBasicCommands.AddTail(ID_OPEN_WORKING_DIR);
	lstBasicCommands.AddTail(ID_OPEN_OUTPUT_MAP_DIR);
	lstBasicCommands.AddTail(ID_TOOLS_TDATE);
	lstBasicCommands.AddTail(ID_TOOLS_MERGEFILES);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);
}



void CMainFrame::OnToolsDbEditor()
{
	CAppOption option(_T("DBEditor"));
	
	CDBManagerDlg dlg(this, option.GetProfileInt(_T("CurPage"), 0));
	dlg.DoModal();

	option.WriteProfileInt(_T("CurPage"),dlg.GetActiveIndex());
}

void CMainFrame::OnToolsOptions()
{
	CBioSIMOptionDlg dlg(this);
	dlg.DoModal();
}

LRESULT CMainFrame::OnToolsOptions2(WPARAM wParam, LPARAM lParam)
{
	OnToolsOptions();
	return 0;
}

void CMainFrame::CleanUp()
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetActiveDocument();
	if( pDoc->IsInit() )
	{
		if( MessageBox( GetCString(IDS_CLEANUP), 0, MB_OKCANCEL ) == IDOK )
		{
			pDoc->SetIsExecute(true);
			
			string path = GetFM().GetProjectPath();
			if( !path.empty() )
			{
				path += "Tmp\\";
				static const char* EXT[4] = {"*.DBdata", "*.DBindex", "*.DBmeta", "*.log"};
				for(size_t i=0; i<4; i++)
				{
					string filter = path + EXT[i];
					StringVector list = WBSF::GetFilesList(filter,WBSF::FILE_PATH,true);

					for(size_t j=0; j<list.size(); j++)
						WBSF::RemoveFile(list[j]);
				}
			}

			pDoc->SetIsExecute(false);
		}
	}
}


void CMainFrame::OnExecute()
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetActiveDocument();
	
	ERMsg msg = pDoc->Execute(m_pTaskbarList);

	
	if( !msg)
		SYShowMessage( msg, this );
}


void CMainFrame::OnUpdateExecute(CCmdUI *pCmdUI)
{
	CBioSIMDoc* pDoc = (CBioSIMDoc* )GetActiveDocument();
	if( pDoc )
		pCmdUI->Enable(pDoc->IsInit()&&!pDoc->IsExecute());

}

int GetLanguage(UINT id)
{
	return id-ID_LANGUAGE_FRENCH;
}


void CMainFrame::OnLanguageChange(UINT id)
{
	CRegistry registry;
	if (registry.GetLanguage() != GetLanguage(id))
	{
		registry.SetLanguage(GetLanguage(id));
		//VERIFY(theApp.GetContextMenuManager()->ResetState());


		HINSTANCE hInst = NULL;
		if (GetLanguage(id) == CRegistry::FRENCH)
		{
			hInst = LoadLibraryW(L"BioSIM11Frc.dll");
		}
		else
		{
			hInst = LoadLibraryW(L"BioSIM11.exe");
		}

		if (hInst != NULL)
			AfxSetResourceHandle(hInst);

		//set resources for non MFC get string
		CDynamicResources::set(AfxGetResourceHandle());

		CStatistic::ReloadString();
		CTM::ReloadString();


		


		m_wndToolBar.RestoreOriginalState();
		m_wndMenuBar.RestoreOriginalState();

		
		((CContextMenuManagerEx*)theApp.GetContextMenuManager())->RestoreOriginalState();
		

	//	CMFCPopupMenuBar menuBar;
//		theApp.GetContextMenuManager()->m_Menus;// CopyOriginalMenuItemsToMenu(IDR_POPUP, menuBar);
	//	menuBar.RestoreOriginalState();
		//theApp.GetContextMenuManager()->CopyOriginalMenuItemsFromMenu(IDR_POPUP, menuBar);

		//VERIFY(theApp.GetContextMenuManager()->LoadState(""));

		//CMenu* pPopop = CMenu::FromHandle(theApp.GetContextMenuManager()->GetMenuById(IDR_POPUP));
		//VERIFY(pPopop->DestroyMenu());
		//pPopop->CreateMenu();
		//VERIFY(pPopop->LoadMenuW(IDR_POPUP));

		
		

		//CMFCToolBar::ResetAllImages();
		CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);

		m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
		m_projectWnd.SetWindowText(GetCString(IDS_PROJECT_WND));
		m_propertiesWnd.SetWindowText(GetCString(IDS_PROPERTIES_WND));
		m_spreadsheetWnd.SetWindowText(GetCString(IDS_SPREADSHEET_WND));
		//m_chartWnd.SetWindowText(GetCString(IDS_CHARTS_WND));
		m_exportWnd.SetWindowText(GetCString(IDS_EXPORT_WND));
		//m_progressWnd.SetWindowText(GetCString(IDS_PROGRESS_WND));
		
		GetActiveDocument()->UpdateAllViews(NULL, CBioSIMDoc::ID_LAGUAGE_CHANGE);
		Invalidate();
		RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
	}


}


void CMainFrame::OnLanguageUI(CCmdUI* pCmdUI)
{
	WBSF::CRegistry registry;
	pCmdUI->SetRadio(registry.GetLanguage() == GetLanguage(pCmdUI->m_nID) );
}

void CMainFrame::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
}
void CMainFrame::OnRunApp(UINT id)
{
	OnToolsRunApp(id);
}

BOOL CMainFrame::OnToolsRunApp(UINT id)
{
	CRegistry registry;
	string filePath;

	
	switch( id )
	{
	case ID_TOOLS_TDATE: filePath = registry.GetAppFilePath(CRegistry::TDATE); break;
	case ID_TOOLS_MERGEFILES: filePath = registry.GetAppFilePath(CRegistry::MERGEFILE); break;
	case ID_HOURLY_EDITOR:filePath = registry.GetAppFilePath(CRegistry::HOURLY_EDITOR); break;
	case ID_DAILY_EDITOR:filePath = registry.GetAppFilePath(CRegistry::DAILY_EDITOR); break;
	case ID_NORMALS_EDITOR:filePath = registry.GetAppFilePath(CRegistry::NORMAL_EDITOR); break;
	case ID_MATCH_STATION:filePath = registry.GetAppFilePath(CRegistry::MATCH_STATION); break;
	case ID_WEATHER_UPDATER:filePath = registry.GetAppFilePath(CRegistry::WEATHER_UPDATER); break;
	default: ASSERT(false);
	}
	
	//
 //   CWnd* pWnd = CWnd::FindWindowW(NULL, CStringW(fileName.c_str()));
 //   if( pWnd )
	//{
	//	pWnd->ShowWindow(SW_SHOW);
	//}
	//else
 //   {
		
    WinExec( filePath.c_str(), SW_SHOW);
    

	return TRUE;
}


BOOL CMainFrame::OnHelp(UINT id)
{
	string name;
	
	CRegistry registry;
	string lang = (registry.GetLanguage() == CRegistry::FRENCH) ? "Fr" : "En";
	switch(id)
	{
	case ID_DOWNLOAD_DATA:	name="https://drive.google.com/drive/u/0/folders/1KZBlYp54URTP3eoMHZcvGI_BO6LrxW1Q";break;
	case ID_HELP_MANUAL:	
	case ID_HELP_TUTORIAL:	name = "https://drive.google.com/drive/u/0/folders/15I1IQtIGnbUuEigSumc_Q5JwFdjKEdsu";break;
	default: ASSERT(false);
	}
	
	ShellExecuteW(NULL, _T("open"), Convert(name), NULL,NULL, SW_SHOW);

	return TRUE;
}



void CMainFrame::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if( GetActiveDocument()!=NULL)
	{
		m_projectWnd.OnUpdate(pSender, lHint, pHint);
		m_propertiesWnd.OnUpdate(pSender, lHint, pHint);
		m_spreadsheetWnd.OnUpdate(pSender, lHint, pHint);
		//m_chartWnd.OnUpdate(pSender, lHint, pHint);
		m_exportWnd.OnUpdate(pSender, lHint, pHint);
		//m_progressWnd.OnUpdate(pSender, lHint, pHint);
		//m_fileManagerWnd.OnUpdate(pSender, lHint, pHint);
	}
}

BOOL CMainFrame::OnCmdMsg(UINT id, int code, void *pExtra, AFX_CMDHANDLERINFO* pHandler)
{

	//try the focus window first 
	//let the trl to route command
	/*CWnd* pFocus = GetFocus();
	if (pFocus && )
	{
		if (pFocus->OnCmdMsg(id, code, pExtra, pHandler))
			return TRUE;
	}*/


	//route cmd first to registered dockable pane
	if (m_projectWnd.GetSafeHwnd() && m_projectWnd.IsVisible() && m_projectWnd.OnCmdMsg(id, code, pExtra, pHandler))
		return TRUE;
	if (m_propertiesWnd.GetSafeHwnd() && m_propertiesWnd.IsVisible() && m_propertiesWnd.OnCmdMsg(id, code, pExtra, pHandler))
		return TRUE;
	if (m_spreadsheetWnd.GetSafeHwnd() && m_spreadsheetWnd.IsVisible() && m_spreadsheetWnd.OnCmdMsg(id, code, pExtra, pHandler))
		return TRUE;
	//if (m_chartWnd.GetSafeHwnd() && m_chartWnd.IsVisible() && m_chartWnd.OnCmdMsg(id, code, pExtra, pHandler))
		//return TRUE;
	if (m_exportWnd.GetSafeHwnd() && m_exportWnd.IsVisible() && m_exportWnd.OnCmdMsg(id, code, pExtra, pHandler))
		return TRUE;
		

	return CFrameWndEx::OnCmdMsg(id, code, pExtra, pHandler);
}


#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


//st
//*************************************************************************************
