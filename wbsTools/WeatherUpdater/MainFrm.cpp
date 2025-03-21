
// MainFrm.cpp : impl�mentation de la classe CMainFrame
//

#include "stdafx.h"
#include "WeatherUpdater.h"
#include "MainFrm.h"

#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "WeatherUpdaterOptionsDlg.h"
#include "WeatherUpdaterDoc.h"
#include "Tasks/StateSelection.h"
#include "Tasks/ProvinceSelection.h"
#include "WeatherBasedSimulationUI.h"

using namespace WBSF;
using namespace UtilWin;


static const UINT ID_LAGUAGE_CHANGE = 5;//from document

static const UINT ID_PROJECT_WND = 501;
static const UINT ID_PROPERTIES_WND = 502;



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx) 
const UINT CMainFrame::m_uTaskbarBtnCreatedMsg = RegisterWindowMessage(_T("TaskbarButtonCreated"));

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_SETTINGCHANGE()
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &OnUpdateApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LANGUAGE_FRENCH, ID_LANGUAGE_ENGLISH, &OnUpdateToolbar)

	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &OnApplicationLook)
	ON_COMMAND_RANGE(ID_LANGUAGE_FRENCH, ID_LANGUAGE_ENGLISH, &OnLanguageChange)
	ON_COMMAND(ID_OPTIONS, &OnEditOptions)
	ON_REGISTERED_MESSAGE(m_uTaskbarBtnCreatedMsg, OnTaskbarProgress)
END_MESSAGE_MAP()

LRESULT CMainFrame::OnTaskbarProgress(WPARAM wParam, LPARAM lParam)
{
	// On pre-Win 7, anyone can register a message called "TaskbarButtonCreated"
	// and broadcast it, so make sure the OS is Win 7 or later before acting on
	// the message. (This isn't a problem for this app, which won't run on pre-7,
	// but you should definitely do this check if your app will run on pre-7.)
	DWORD dwMajor = LOBYTE(LOWORD(GetVersion()));
	DWORD dwMinor = HIBYTE(LOWORD(GetVersion()));

	// Check that the Windows version is at least 6.1 (yes, Win 7 is version 6.1).
	if (dwMajor > 6 || (dwMajor == 6 && dwMinor > 0))
	{
		m_pTaskbarList.Release();
		m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	}

	return 0;
}

static UINT indicators[] =
{
	ID_SEPARATOR,           // indicateur de la ligne d'�tat
};

// construction ou destruction�de CMainFrame

CMainFrame::CMainFrame()
{
	
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2005);
	CTabbedPane::m_pTabWndRTC = RUNTIME_CLASS(CMFCTabCtrl24);
}

CMainFrame::~CMainFrame()
{}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

//	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	//ChangeWindowMessageFilterEx(m_hWnd, m_uTaskbarBtnCreatedMsg, MSGFLT_ALLOW, &cfs);

	//main frame
	CDockingManager::SetDockingMode(DT_SMART);
	EnableAutoHidePanes(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);


	//menu
	VERIFY(m_wndMenuBar.Create(this));
	m_wndMenuBar.SetMenuSizes(CSize(22, 22), CSize(16, 16));
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndMenuBar.SetRecentlyUsedMenus(FALSE);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	CMFCPopupMenu::SetForceMenuFocus(FALSE);
	DockPane(&m_wndMenuBar);


	//toolbar
	VERIFY(m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_MAINFRAME_TOOLBAR, 0, 0, 1));
	m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);
	DockPane(&m_wndToolBar);

	//status
	VERIFY(m_wndStatusBar.Create(this));
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	//create dockable panes
	VERIFY(CreateDockingWindows());
	
	//dock panes
	DockPane(&m_wndProject, AFX_IDW_DOCKBAR_LEFT);
	DockPane(&m_wndProperties, AFX_IDW_DOCKBAR_TOP);
	//m_progressWnd.AttachToTabWnd(&m_wndProperties, DM_STANDARD, 0);


	OnApplicationLook(theApp.m_nAppLook); 
	EnablePaneMenu(TRUE, ID_VIEW_STATUS_BAR, GetCString(IDS_TOOLBAR_STATUS), ID_VIEW_TOOLBAR);
	LoadtBasicCommand();

	CStateSelection::UpdateString();
	CProvinceSelection::UpdateString();


	return 0;
}

BOOL CMainFrame::CreateDockingWindows()
{
	VERIFY(m_wndProject.Create(GetCString(IDS_PROJECT_WND), this, CRect(0, 0, 600, 400), TRUE, ID_PROJECT_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI));
	m_wndProject.EnableDocking(CBRS_ALIGN_ANY);
	
	VERIFY(m_wndProperties.Create(GetCString(IDS_PROPERTIES_WND), this, CRect(0, 0, 600, 400), TRUE, ID_PROPERTIES_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI));
	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);

	//VERIFY(m_progressWnd.Create(GetCString(IDS_PROGRESS_WND), this, CRect(0, 0, 600, 400), TRUE, ID_PROGRESS_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI));
	//m_progressWnd.EnableDocking(CBRS_ALIGN_ANY);

	SetDockingWindowIcons(theApp.m_bHiColorIcons);

	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
	
	HICON hProjectIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROJECT_WND), IMAGE_ICON, 24, 24, 0);
	m_wndProject.SetIcon(hProjectIcon, TRUE);

	HICON hOutputIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES_WND), IMAGE_ICON, 24, 24, 0);
	m_wndProperties.SetIcon(hOutputIcon, TRUE);

	//HICON hProgressIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROGRESS_WND), IMAGE_ICON, 24, 24, 0);
//	m_progressWnd.SetIcon(hProgressIcon, TRUE);

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

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
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

	//m_wndProperties.UpdateFonts();
	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}


void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);
	//m_wndProperties.UpdateFonts();
}


void CMainFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{ 
	if (GetActiveDocument() != NULL)
	{
		m_wndProject.OnUpdate(pSender, lHint, pHint);
		m_wndProperties.OnUpdate(pSender, lHint, pHint);
	}
}

int GetLanguage(UINT id)
{
	return id - ID_LANGUAGE_FRENCH;
}

void CMainFrame::OnLanguageChange(UINT id)
{
	WBSF::CRegistry registry;
	if (registry.GetLanguage() != GetLanguage(id))
	{
		registry.SetLanguage(GetLanguage(id));

		HINSTANCE hInst = NULL;
		if (GetLanguage(id) == WBSF::CRegistry::FRENCH)
		{
			hInst = LoadLibraryW(L"WeatherUpdaterFrc.dll");
		}
		else
		{
			hInst = LoadLibraryW(L"WeatherUpdater.exe");
		}

		if (hInst != NULL)
			AfxSetResourceHandle(hInst);

		
		//set resources for non MFC get string
		CDynamicResources::set(AfxGetResourceHandle());
		
		WBSF::CStatistic::ReloadString();
		WBSF::CTM::ReloadString();
		CStateSelection::UpdateString();
		CProvinceSelection::UpdateString();
		
		
		m_wndToolBar.RestoreOriginalState();
		m_wndMenuBar.RestoreOriginalState();

		CMFCToolBar::ResetAllImages();
		CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);

		m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
		
		m_wndProject.SetWindowText(GetCString(IDS_PROJECT_WND));
		m_wndProperties.SetWindowText(GetCString(IDS_PROPERTIES_WND));
		//m_progressWnd.SetWindowText(GetCString(IDS_PROGRESS_WND));
		
		CWeatherUpdaterDoc* pDoc = static_cast<CWeatherUpdaterDoc*>(GetActiveDocument());
		ENSURE(pDoc);
		pDoc->SetLanguage(id);
		
		Invalidate();
	}
	

}

void CMainFrame::LoadtBasicCommand()
{
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_PRINT);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_UPDATER_REFERENCE);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_LANGUAGE_FRENCH);
	lstBasicCommands.AddTail(ID_LANGUAGE_ENGLISH);

	
	CMFCToolBar::SetBasicCommands(lstBasicCommands);
}


void CMainFrame::OnUpdateToolbar(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID)
	{
	case ID_LANGUAGE_FRENCH:{WBSF::CRegistry registry; pCmdUI->SetRadio(registry.GetLanguage() == WBSF::CRegistry::FRENCH); break; }
	case ID_LANGUAGE_ENGLISH:{WBSF::CRegistry registry; pCmdUI->SetRadio(registry.GetLanguage() == WBSF::CRegistry::ENGLISH); break; }
	}
}


void CMainFrame::OnEditOptions()
{
	CWeatherUpdaterOptionsDlg dlg;
	dlg.DoModal();

}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	

	if (GetActiveDocument() == NULL)
	{
		CWinApp* pApp = AfxGetApp();
		if (pApp)
		{
			POSITION  pos = pApp->GetFirstDocTemplatePosition();
			CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
			if (docT)
			{
				pos = docT->GetFirstDocPosition();
				CDocument* pDoc = docT->GetNextDoc(pos);
				if (pDoc)
				{
					if (pDoc->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
						return TRUE;
				}
			}
		}
	}
	else
	{
		if (m_wndProject.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
		if (m_wndProperties.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))		
			return TRUE;
	}

	return CFrameWndEx::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


// diagnostics pour CMainFrame

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
