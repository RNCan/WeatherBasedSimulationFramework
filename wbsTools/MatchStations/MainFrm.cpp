
// MainFrm.cpp : implémentation de la classe CMainFrame
//

#include "stdafx.h"
#include "Basic/Registry.h"
#include "Basic/DynamicRessource.h"
#include "Basic/Statistic.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/AppOption.h"
#include "UI/WVariablesEdit.h"

#include "MatchStationApp.h"
#include "MainFrm.h"
#include "resource.h"


using namespace UtilWin;
using namespace WBSF;

static const UINT ID_LAGUAGE_CHANGE			= 5;//from document
static const UINT ID_VIEW_NORMALS_WND		= 500 + 1;
static const UINT ID_VIEW_OBSERVATION_WND   = 500 + 2;
static const UINT ID_VIEW_WEIGHT_CHARTS_WND = 500 + 3;
static const UINT ID_VIEW_LOCATIONS_WND     = 500 + 4;
static const UINT ID_VIEW_PROPERTIES_WND    = 500 + 5;
static const UINT ID_VIEW_FILEPATH_WND      = 500 + 6;
static const UINT ID_VIEW_GRADIENT_WND      = 500 + 7;
static const UINT ID_VIEW_CORRECTION_WND    = 500 + 8;
static const UINT ID_VIEW_ESTIMATE_WND		= 500 + 9;
static const UINT ID_VIEW_OBS_ESTIMATE_WND	= 500 + 10;
const UINT CMainFrame::m_uTaskbarBtnCreatedMsg = RegisterWindowMessage(_T("TaskbarButtonCreated"));


#ifdef _DEBUG
#define new DEBUG_NEW
#endif




//***********************************************************************************************************************************
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
	ID_SEPARATOR,           // indicateur de la ligne d'état
};


// construction ou destruction de CMainFrame

CMainFrame::CMainFrame()
{
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);
	CTabbedPane::m_pTabWndRTC = RUNTIME_CLASS(CMFCTabCtrl24);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// activer le comportement de la fenêtre d'ancrage de style Visual Studio 2005
	CDockingManager::SetDockingMode(DT_SMART, AFX_SDT_VS2008);
	EnableDocking(CBRS_ORIENT_ANY);
	EnableAutoHidePanes(CBRS_ORIENT_ANY);

	
	VERIFY(m_wndMenuBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY));
	m_wndMenuBar.SetMenuSizes(CSize(22, 22), CSize(16, 16));
	//m_wndMenuBar.RestoreOriginalState();//reload menu from resource
	m_wndMenuBar.SetRecentlyUsedMenus(FALSE);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar, AFX_IDW_DOCKBAR_TOP);
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	
	VERIFY(m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_MAINFRAME_TOOLBAR, 0, 0, 1));
	//m_wndToolBar.RestoreOriginalState();//reload good resource when language is chnaged from other aspplication
	m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
	m_wndToolBar.EnableDocking(CBRS_ORIENT_HORZ);
	DockPane(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);

	
	VERIFY(m_wndStatusBar.Create(this));
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	// créer des fenêtres d'ancrage
	VERIFY(CreateDockingWindows());


	// définir le gestionnaire visuel et le style visuel en fonction d'une valeur persistante
	OnApplicationLook(theApp.m_nAppLook);

	// Activer le remplacement du menu de la fenêtre d'ancrage et de la barre d'outils
	EnablePaneMenu(TRUE, ID_VIEW_STATUS_BAR, GetCString(IDS_TOOLBAR_STATUS), ID_VIEW_TOOLBAR);
	LoadtBasicCommand();
		 
	return 0;
}

	
BOOL CMainFrame::CreateDockingWindows()
{

	if (!m_wndFilePath.Create(GetCString(IDS_FILEPATH_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_FILEPATH_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP, AFX_CBRS_REGULAR_TABS, AFX_IDW_DOCKBAR_TOP | AFX_CBRS_CLOSE))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}

	// Créer la fenêtre Propriétés
	if (!m_propertiesWnd.Create(GetCString(IDS_PROPERTIES_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_PROPERTIES_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_BOTTOM))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}

	if (!m_locationsWnd.Create(GetCString(IDS_LOCATION_LIST_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_LOCATIONS_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_LEFT))
	{
		TRACE0("Impossible de créer la fenêtre Sortie\n");
		return FALSE; // échec de la création
	}

	if (!m_normalsWnd.Create(GetCString(IDS_NORMALS_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_NORMALS_WND, WS_CHILD | WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}

	if (!m_observationWnd.Create(GetCString(IDS_OBSERVATION_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_OBSERVATION_WND, WS_CHILD | WS_VISIBLE | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}


	if (!m_gradientWnd.Create(GetCString(IDS_GRADIENT_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_GRADIENT_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}

	if (!m_correctionWnd.Create(GetCString(IDS_CORRECTION_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_CORRECTION_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}
		
	if (!m_estimateWnd.Create(GetCString(IDS_ESTIMATE_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_ESTIMATE_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}

	if (!m_obsEstimateWnd.Create(GetCString(IDS_OBS_ESTIMATE_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_OBS_ESTIMATE_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Propriétés\n");
		return FALSE; // échec de la création
	}
		
		

	// Créer la fenêtre Sortie
	if (!m_weightChartsWnd.Create(GetCString(IDS_WEIGHT_CHARTS_WND), this, CRect(100, 100, 400, 400), TRUE, ID_VIEW_WEIGHT_CHARTS_WND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI | CBRS_TOP))
	{
		TRACE0("Impossible de créer la fenêtre Sortie\n");
		return FALSE; // échec de la création
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons);

	m_wndFilePath.EnableDocking(CBRS_ORIENT_HORZ);
	m_normalsWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_observationWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_locationsWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_propertiesWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_gradientWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_correctionWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_estimateWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_obsEstimateWnd.EnableDocking(CBRS_ALIGN_ANY);
	m_weightChartsWnd.EnableDocking(CBRS_ALIGN_ANY);


	DockPane(&m_wndFilePath, AFX_IDW_DOCKBAR_TOP);
	DockPane(&m_locationsWnd, AFX_IDW_DOCKBAR_LEFT);
	m_propertiesWnd.DockToWindow(&m_locationsWnd, CBRS_ALIGN_BOTTOM);
	
	
	CDockablePane* pPaneFrame = NULL;
	DockPane(&m_normalsWnd, AFX_IDW_DOCKBAR_TOP, CRect(0, 0, 800, 800));
	m_gradientWnd.AttachToTabWnd(&m_normalsWnd, DM_STANDARD, 0, &pPaneFrame);
	m_correctionWnd.AttachToTabWnd(&m_normalsWnd, DM_STANDARD, 0);
	m_estimateWnd.AttachToTabWnd(&m_normalsWnd, DM_STANDARD, 0);

	
	m_observationWnd.DockToWindow(pPaneFrame, CBRS_ALIGN_BOTTOM);
	m_obsEstimateWnd.AttachToTabWnd(&m_observationWnd, DM_STANDARD, 0);
	m_weightChartsWnd.AttachToTabWnd(&m_observationWnd, DM_STANDARD, 0);
	
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
	HICON hFilepathIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_FILEPATH_WND), IMAGE_ICON, 24, 24, 0);
	m_wndFilePath.SetIcon(hFilepathIcon, TRUE);
		
	HICON hMatchNormalsIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_NORMAL_WND), IMAGE_ICON, 24, 24, 0);
	m_normalsWnd.SetIcon(hMatchNormalsIcon, TRUE);

	HICON hMatchObservationIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_OBSERVATION_WND), IMAGE_ICON, 24, 24, 0);
	m_observationWnd.SetIcon(hMatchObservationIcon, TRUE);

	HICON hOutputIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 24, 24, 0);
	m_locationsWnd.SetIcon(hOutputIcon, TRUE);

	HICON hPropertiesIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES_WND), IMAGE_ICON, 24, 24, 0);
	m_propertiesWnd.SetIcon(hPropertiesIcon, TRUE);

	HICON hGradientIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_GRADIENT_WND), IMAGE_ICON, 24, 24, 0);
	m_gradientWnd.SetIcon(hGradientIcon, TRUE);

	HICON hCorrectionIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CORRECTION_WND), IMAGE_ICON, 24, 24, 0);
	m_correctionWnd.SetIcon(hCorrectionIcon, TRUE);

	HICON hEstimateIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ESTIMATE_N_WND), IMAGE_ICON, 24, 24, 0);
	m_estimateWnd.SetIcon(hEstimateIcon, TRUE);

	HICON hObsEstimateIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ESTIMATE_O_WND), IMAGE_ICON, 24, 24, 0);
	m_obsEstimateWnd.SetIcon(hObsEstimateIcon, TRUE);
		

	HICON hWeightChartsBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_WEIGHT_CHART_WND), IMAGE_ICON, 24, 24, 0);
	m_weightChartsWnd.SetIcon(hWeightChartsBarIcon, TRUE);
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
}

void CMainFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	m_wndFilePath.OnUpdate(pSender, lHint, pHint);
	m_normalsWnd.OnUpdate(pSender, lHint, pHint);
	m_observationWnd.OnUpdate(pSender, lHint, pHint);
	m_weightChartsWnd.OnUpdate(pSender, lHint, pHint);
	m_gradientWnd.OnUpdate(pSender, lHint, pHint);
	m_correctionWnd.OnUpdate(pSender, lHint, pHint);
	m_estimateWnd.OnUpdate(pSender, lHint, pHint);
	m_obsEstimateWnd.OnUpdate(pSender, lHint, pHint);
	m_propertiesWnd.OnUpdate(pSender, lHint, pHint);
	m_locationsWnd.OnUpdate(pSender, lHint, pHint);
}

int GetLanguage(UINT id)
{
	return id - ID_LANGUAGE_FRENCH;
}

	
	
void CMainFrame::OnLanguageChange(UINT id)
{
	CRegistry registry;
	if (registry.GetLanguage() != GetLanguage(id))
	{
		registry.SetLanguage(GetLanguage(id));

		HINSTANCE hInst = NULL;
		if (GetLanguage(id) == CRegistry::FRENCH)
		{
			hInst = LoadLibraryW(L"MatchStationFrc.dll");
		}
		else
		{
			hInst = LoadLibraryW(L"MatchStation.exe");
		}

		if (hInst != NULL)
			AfxSetResourceHandle(hInst);

		//set resources for non MFC get string
		CDynamicResources::set(AfxGetResourceHandle());

		CStatistic::ReloadString();
		CTM::ReloadString();

		//m_wndMenuBar.RestoreOriginalState();
		//m_wndToolBar.RestoreOriginalState();
		m_wndToolBar.ResetAll();
		
		//m_wndMenuBar.SetDefaultMenuResId(IDR_MAINFRAME);
		m_wndMenuBar.ResetAll();

		
		m_wndMenuBar.Invalidate();
		m_wndToolBar.Invalidate();
		DrawMenuBar();
		//CMFCToolBar::ResetAllImages();
		CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);


		//m_wndMenuBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
		m_wndToolBar.SetWindowText(GetCString(IDS_TOOLBAR_STANDARD));
		m_wndFilePath.SetWindowText(GetCString(IDS_FILEPATH_WND));
		m_normalsWnd.SetWindowText(GetCString(IDS_NORMALS_WND));
		m_observationWnd.SetWindowText(GetCString(IDS_OBSERVATION_WND));
		m_weightChartsWnd.SetWindowText(GetCString(IDS_WEIGHT_CHARTS_WND));
		m_gradientWnd.SetWindowText(GetCString(IDS_GRADIENT_WND));
		m_correctionWnd.SetWindowText(GetCString(IDS_CORRECTION_WND));
		m_estimateWnd.SetWindowText(GetCString(IDS_ESTIMATE_WND));
		m_obsEstimateWnd.SetWindowText(GetCString(IDS_OBS_ESTIMATE_WND));
		m_propertiesWnd.SetWindowText(GetCString(IDS_PROPERTIES_WND));
		m_locationsWnd.SetWindowText(GetCString(IDS_LOCATION_LIST_WND));

		GetActiveDocument()->UpdateAllViews(NULL, ID_LAGUAGE_CHANGE);
		Invalidate();
		RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);
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
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_SENDTO_SHOWMAP);
	lstBasicCommands.AddTail(ID_SORTPROPERTIES);
	lstBasicCommands.AddTail(ID_NB_STATIONS);
	lstBasicCommands.AddTail(ID_STATION_VARIABLES);
	lstBasicCommands.AddTail(ID_STATION_YEAR);
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


void CMainFrame::OnLanguageUI(CCmdUI* pCmdUI)
{
	CRegistry registry;
	pCmdUI->SetRadio(registry.GetLanguage() == GetLanguage(pCmdUI->m_nID));
}



void CMainFrame::OnEditOptions()
{
	//	CMatchStationOptionsDlg dlg;
	//	dlg.DoModal();

}


class CMyEditBox : public CMFCToolBarEditBoxButton
{
public:

	DECLARE_DYNCREATE(CMyEditBox)

	CMyEditBox()
	{}


	CMyEditBox(UINT uiID, int iImage, DWORD dwStyle = ES_AUTOHSCROLL, int iWidth = 0) :
		CMFCToolBarEditBoxButton(uiID, iImage, dwStyle, iWidth)
	{
	}




};

//IMPLEMENT_DYNCREATE(CMyEditBox, CMFCToolBarEditBoxButton)
//	LRESULT CMainFrame::OnToolbarReset(WPARAM wp, LPARAM)
//{
//	UINT uiToolBarId = (UINT)wp;
//	TRACE("CMainFrame::OnToolbarReset : %i\n", uiToolBarId);
//
//	/*switch (uiToolBarId)
//	{
//	case IDR_NORMALS_TOOLBAR:
//	{
//	CMFCToolBarEditBoxButton button1(ID_NB_NORMALS_STATION, GetCmdMgr()->GetCmdImage(ID_NB_NORMALS_STATION, FALSE));
//	m_wndToolBar2.ReplaceButton(ID_NB_NORMALS_STATION, button1);
//	CMFCToolBarEditBoxButton button2(ID_NORMALS_FILEPATH, GetCmdMgr()->GetCmdImage(ID_NORMALS_FILEPATH, FALSE), ES_AUTOHSCROLL, 250);
//	m_wndToolBar2.ReplaceButton(ID_NORMALS_FILEPATH, button2);
//
//	CMFCToolBarEditBoxButton* pCtrl = CMFCToolBarEditBoxButton::GetByCmd(ID_NORMALS_FILEPATH); ASSERT(pCtrl);
//	CMFCToolBarEditCtrl* pCtrl2 = static_cast<CMFCToolBarEditCtrl*>(pCtrl->GetEditBox());
//	pCtrl2->EnableFileBrowseButton(_T(".NormalsStations"), _T("*.NormalsStations|*.NormalsStations||"));
//	}
//	break;
//
//	case IDR_OBSERVATION_TOOLBAR:
//	{
//	CMFCToolBarEditBoxButton button1(ID_NB_OBSERVATION_STATION, GetCmdMgr()->GetCmdImage(ID_NB_OBSERVATION_STATION, FALSE));
//	m_wndToolBar3.ReplaceButton(ID_NB_OBSERVATION_STATION, button1);
//	CMFCToolBarEditBoxButton button2(ID_OBSERVATION_FILEPATH, GetCmdMgr()->GetCmdImage(ID_OBSERVATION_FILEPATH, FALSE), ES_AUTOHSCROLL, 250);
//	m_wndToolBar3.ReplaceButton(ID_OBSERVATION_FILEPATH, button2);
//
//	CMFCToolBarEditBoxButton* pCtrl = CMFCToolBarEditBoxButton::GetByCmd(ID_OBSERVATION_FILEPATH); ASSERT(pCtrl);
//	CMFCToolBarEditCtrl* pCtrl2 = static_cast<CMFCToolBarEditCtrl*>(pCtrl->GetEditBox());
//	pCtrl2->EnableFileBrowseButton(_T(".DailyStations"), _T("*.DailyStations|*.DailyStations||"));
//	}
//
//	break;
//	}*/
//
//	return 0;
//}
	

void CMainFrame::AdjustDockingLayout(HDWP hdwp)
{
	CFrameWndEx::AdjustDockingLayout(hdwp);

	if (m_wndFilePath.GetSafeHwnd())
	{
		CRect rect;//Usable = m_dockManager.Get.GetClientAreaBounds();
		m_wndFilePath.GetWindowRect(rect);
		m_wndFilePath.SetWindowPos(NULL, 0, 0, rect.Width(), 100, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	}
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

