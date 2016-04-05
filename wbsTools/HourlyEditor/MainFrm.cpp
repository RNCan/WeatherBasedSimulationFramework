
// MainFrm.cpp : implémentation de la classe CMainFrame
//

#include "stdafx.h"

#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "Basic/Statistic.h"
#include "UI/Common/UtilWin.h"

#include "HourlyEditorOptionsDlg.h"
#include "HourlyEditor.h"
#include "MainFrm.h"

using namespace UtilWin;


static const int ID_LAGUAGE_CHANGE = 5;//from document


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx) 

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
	ON_WM_SETTINGCHANGE()
	ON_COMMAND_RANGE(ID_LANGUAGE_FRENCH, ID_LANGUAGE_ENGLISH, &CMainFrame::OnLanguageChange)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LANGUAGE_FRENCH, ID_LANGUAGE_ENGLISH, &CMainFrame::OnLanguageUI)
	ON_COMMAND(ID_OPTIONS, &CMainFrame::OnEditOptions)
	
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // indicateur de la ligne d'état
};

// construction ou destruction de CMainFrame

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
	m_wndMenuBar.SetRestoredFromRegistry(false);
	m_wndMenuBar.SetMenuSizes(CSize(22, 22), CSize(16, 16));
	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndMenuBar.SetRecentlyUsedMenus(FALSE);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	CMFCPopupMenu::SetForceMenuFocus(FALSE);
	DockPane(&m_wndMenuBar);

	
	//Create Toolbar
	VERIFY(m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_MAINFRAME_TOOLBAR, 0, 0, 1));
	m_wndMenuBar.SetRestoredFromRegistry(false);
	m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);
	DockPane(&m_wndToolBar);
	

	//Create Status 
	VERIFY(m_wndStatusBar.Create(this));
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// Create docking
	VERIFY(CreateDockingWindows());

	CDockablePane* pPaneFrame = NULL;
	DockPane(&m_spreadsheetWnd, AFX_IDW_DOCKBAR_RIGHT);
	m_chartWnd.AttachToTabWnd(&m_spreadsheetWnd, DM_STANDARD, 0, &pPaneFrame);
	m_wndOutput.DockToWindow(pPaneFrame, CBRS_ALIGN_BOTTOM);

	DockPane(&m_wndProperties);



	OnApplicationLook(theApp.m_nAppLook); 
	EnablePaneMenu(TRUE, ID_VIEW_STATUS_BAR, GetCString(IDS_TOOLBAR_STATUS), ID_VIEW_TOOLBAR);
	LoadBasicCommand();

	return 0;
}


BOOL CMainFrame::CreateDockingWindows()
{
	if (!m_spreadsheetWnd.Create(GetCString(IDS_SPREASHEET_WND), this, CRect(0,0,800,400), TRUE, ID_SPREADSHEET_VIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Impossible de créer la fenêtre Affichage des fichiers\n");
		return FALSE; // échec de la création
	}
	m_spreadsheetWnd.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_chartWnd.Create(GetCString(IDS_CHART_WND), this, CRect(0,0,800,400), TRUE, ID_CHARTS_VIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Impossible de créer la fenêtre Affichage des fichiers\n");
		return FALSE; // échec de la création
	}
	m_chartWnd.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndProperties.Create(GetCString(IDS_PROPERTIES_WND), this, CRect(0,0,600,400), TRUE, ID_VIEW_PROPERTIESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}
	m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);

	if (!m_wndOutput.Create(GetCString(IDS_OUTPUT_WND), this, CRect(0,0,800,400), TRUE, ID_VIEW_OUTPUTWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		TRACE0("Impossible de créer la fenêtre Sortie\n");
		return FALSE; // échec de la création
	}
	m_wndOutput.EnableDocking(CBRS_ALIGN_ANY);

	SetDockingWindowIcons(theApp.m_bHiColorIcons);


	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{

	HICON hSpreadsheetIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_SPREADSHEET_WND), IMAGE_ICON, 24, 24, 0);
	m_spreadsheetWnd.SetIcon(hSpreadsheetIcon, TRUE);

	HICON hChartIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CHART_WND), IMAGE_ICON, 24, 24, 0);
	m_chartWnd.SetIcon(hChartIcon, TRUE);


	HICON hOutputIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_OUTPUT_WND), IMAGE_ICON, 24, 24, 0);
	m_wndOutput.SetIcon(hOutputIcon, TRUE);

	HICON hPropertiesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES_WND), IMAGE_ICON, 24, 24, 0);
	m_wndProperties.SetIcon(hPropertiesBarIcon, TRUE);
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

	m_wndOutput.UpdateFonts();
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
	m_wndOutput.UpdateFonts();
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
	OnUpdate(NULL, 0, NULL);
	CFrameWndEx::ActivateFrame(nCmdShow);
}

void CMainFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{ 
	m_wndOutput.OnUpdate(pSender, lHint, pHint);
	m_wndProperties.OnUpdate(pSender, lHint, pHint); 
	m_spreadsheetWnd.OnUpdate(pSender, lHint, pHint);
	m_chartWnd.OnUpdate(pSender, lHint, pHint);

}

int GetLanguage(UINT id)
{
	return id - ID_LANGUAGE_FRENCH;
}

void CMainFrame::OnLanguageUI(CCmdUI* pCmdUI)
{
	WBSF::CRegistry registry;
	pCmdUI->SetRadio(registry.GetLanguage() == GetLanguage(pCmdUI->m_nID));

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
			hInst = LoadLibraryW(L"HourlyEditorFrc.dll");
		}
		else
		{
			hInst = LoadLibraryW(L"HourlyEditor.exe");
		}

		if (hInst != NULL)
			AfxSetResourceHandle(hInst);

		//set resources for non MFC get string
		CDynamicResources::set(AfxGetResourceHandle());
		
		WBSF::CStatistic::ReloadString();
		WBSF::CTM::ReloadString();
		

		m_wndToolBar.RestoreOriginalState();
		m_wndMenuBar.RestoreOriginalState();
		
		//CMFCToolBar::ResetAllImages();
		CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);

		m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
		m_spreadsheetWnd.SetWindowText(GetCString(IDS_SPREASHEET_WND));
		m_chartWnd.SetWindowText(GetCString(IDS_CHART_WND));
		m_wndOutput.SetWindowText(GetCString(IDS_OUTPUT_WND));
		m_wndProperties.SetWindowText(GetCString(IDS_PROPERTIES_WND));
		
		GetActiveDocument()->UpdateAllViews(NULL, ID_LAGUAGE_CHANGE);
		RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
//		Invalidate();
	}
	

}

void CMainFrame::LoadBasicCommand()
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
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);
	lstBasicCommands.AddTail(ID_SENDTO_SHOWMAP);
	lstBasicCommands.AddTail(ID_SORTPROPERTIES);
	lstBasicCommands.AddTail(ID_STATION_LIST_YEAR);
	lstBasicCommands.AddTail(ID_STATION_LIST_FILTER);
	lstBasicCommands.AddTail(ID_TABLE_MODE_VISUALISATION);
	lstBasicCommands.AddTail(ID_TABLE_MODE_EDITION);
	lstBasicCommands.AddTail(ID_TABLE_STAT);
	lstBasicCommands.AddTail(ID_TABLE_TM_TYPE);
	lstBasicCommands.AddTail(ID_GRAPH_COPY);
	lstBasicCommands.AddTail(ID_GRAPH_SAVE);
	lstBasicCommands.AddTail(ID_GRAPH_OPTIONS);
	lstBasicCommands.AddTail(ID_GRAPH_ZOOM);
	lstBasicCommands.AddTail(ID_GRAPH_PERIOD_ENABLED);
	lstBasicCommands.AddTail(ID_GRAPH_PERIOD_BEGIN);
	lstBasicCommands.AddTail(ID_GRAPH_PERIOD_END);
	lstBasicCommands.AddTail(ID_GRAPH_TM_TYPE);

	lstBasicCommands.AddTail(ID_LANGUAGE_FRENCH);
	lstBasicCommands.AddTail(ID_LANGUAGE_ENGLISH);

	
	CMFCToolBar::SetBasicCommands(lstBasicCommands);
}



void CMainFrame::OnEditOptions()
{
	CHourlyEditorOptionsDlg dlg;
	dlg.DoModal();

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

