
#include "stdafx.h"


#include "UI/Common/UtilWin.h"

#include "MatchStationApp.h"
#include "PropertiesWnd.h"
#include "MainFrm.h"
#include "MatchStationDoc.h"
#include "WeatherBasedSimulationString.h"


using namespace std;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif



using namespace WBSF;


#define AFX_PROP_HAS_LIST 0x0001
#define AFX_PROP_HAS_BUTTON 0x0002
#define AFX_PROP_HAS_SPIN 0x0004





//*****************************************************************************************************

BEGIN_MESSAGE_MAP(CDataPropertyCtrl, CMFCPropertyGridCtrl)
	ON_EN_KILLFOCUS(AFX_PROPLIST_ID_INPLACE, &CDataPropertyCtrl::OnEditKillFocus)
END_MESSAGE_MAP()




//
//void CDataPropertyCtrl::SetProject(CWeatherDatabasePtr& pDB)
//{
//	m_lastStationIndex = UNKNOWN_POS;
//	m_lastRow = -1;
//	m_lastCol = -1;
//
//	RemoveAll();
//}

void CDataPropertyCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pProp) const
{
	CMatchStationDoc* pDoc = CPropertiesWnd::GetDocument();
	//const CLocation &location = pDoc->GetCurLocation();

	//	int col = pProp->GetData();
	////	string val = CStringA(pProp->GetValue());

	//location.SetMember(col, val);
	//pDoc->SetCurStation(location);//update station info

}

void CDataPropertyCtrl::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
	{//
		//CStatistic::ReloadString();
		//CTM::ReloadString();

		//m_wndToolBar.RestoreOriginalState();
		Invalidate();
	}


	CMatchStationDoc* pDoc = CPropertiesWnd::GetDocument();


	//const CWeatherStationPtr& pStation = pDoc->GetCurStation();

	if (lHint == CMatchStationDoc::INIT || lHint == CMatchStationDoc::LOCATION_INDEX_CHANGE)
	{
		size_t index = pDoc->GetCurIndex();
		if (index != m_lastIndex)
		{
			m_lastIndex = index;

			RemoveAll();

			EnableDescriptionArea(FALSE);
			SetVSDotNetLook();
			MarkModifiedProperties();
			CStringArrayEx header(IDS_PROPERTIES_LOCATION_HEADER);
			ASSERT(header.GetSize() == 2);

			CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(header[0], -1);
			CMFCPropertyGridProperty* pSSI = new CMFCPropertyGridProperty(header[1], -1);

			if (pDoc->GetCurIndex() != UNKNOWN_POS)
			{

				const CLocation &location = pDoc->GetLocation(index);

				for (size_t i = 0; i < CLocation::NB_MEMBER; i++)
				{
					if (i < CLocation::SITE_SPECIFIC_INFORMATION)
					{
						CMFCPropertyGridProperty* pItem = new CMFCPropertyGridProperty(CString(CLocation::GetMemberTitle(i)), CString(location.GetMember((int)i).c_str()), CString(""), i);
						pGroup->AddSubItem(pItem);
					}
					else
					{
						const SiteSpeceficInformationMap& SSI = location.m_siteSpeceficInformation;
						for (SiteSpeceficInformationMap::const_iterator it = SSI.begin(); it != SSI.end(); it++, i++)
						{
							CMFCPropertyGridProperty* pItem = new CMFCPropertyGridProperty(CString(it->first.c_str()), CString(it->second.first.c_str()), CString(""), i);
							pSSI->AddSubItem(pItem);
						}
					}
				}
			}

			pGroup->AddSubItem(pSSI);
			AddProperty(pGroup);

			EnableWindow(true);
			EnableProperties(!pDoc->GetLocationVector()->GetFilePath().empty());
		}
	}
	//else if (lHint == CMatchStationDoc::PROPERTIES_CHANGE)
	//{
	//EnableWindow(!pDoc->GetDataInEdition());
	//}
}


void CDataPropertyCtrl::EnableProperties(BOOL bEnable)
{
	for (int i = 0; i < GetPropertyCount(); i++)
	{
		CMFCPropertyGridProperty* pProp = GetProperty(i);
		EnableProperties(pProp, bEnable);
	}
}

void CDataPropertyCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
{
	pProp->Enable(bEnable);

	for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
		EnableProperties(pProp->GetSubItem(ii), bEnable);
}


BOOL CDataPropertyCtrl::PreTranslateMessage(MSG* pMsg)
{

	if (pMsg->message == WM_KEYDOWN)
	{
		BOOL bAlt = GetKeyState(VK_CONTROL) & 0x8000;
		if (pMsg->wParam == VK_RETURN)
		{

			if (m_pSel != NULL && m_pSel->IsInPlaceEditing() != NULL && m_pSel->IsEnabled())
			{
				if (m_pSel->GetOptionCount()>0)
					OnSelectCombo();

				EndEditItem();

				//select next item
				size_t ID = m_pSel->GetData();
				size_t newID = (ID + 1) % GetProperty(0)->GetSubItemsCount();
				CMFCPropertyGridProperty* pNext = FindItemByData(newID);
				if (pNext)
				{
					SetCurSel(pNext);
					EditItem(pNext);
				}

				return TRUE; // this doesn't need processing anymore
			}

		}
		else if (pMsg->wParam == VK_DOWN && bAlt)
		{
			m_pSel->OnClickButton(CPoint(-1, -1));
			return TRUE; // this doesn't need processing anymore
		}

		//	//PreTranslateMessage(pMsg);
		//}
	}

	return CMFCPropertyGridCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing;
}

BOOL CDataPropertyCtrl::ValidateItemData(CMFCPropertyGridProperty* /*pProp*/)
{
	return TRUE;
}

void CDataPropertyCtrl::OnEditKillFocus()
{
	CMFCPropertyGridCtrl::OnEditKillFocus();
}
//**********************************************************************************************************
/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CMatchStationDoc* CPropertiesWnd::GetDocument()
{
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		CFrameWnd * pFrame = (CFrameWnd *)(pApp->m_pMainWnd);
		if (pFrame && pFrame->GetSafeHwnd() != NULL)
			return (CMatchStationDoc*)(pFrame->GetActiveDocument());
	}
	return NULL;

}

CLocationVectorPtr CPropertiesWnd::GetLocationPtr()
{
	CLocationVectorPtr pLocation;
	CMatchStationDoc* pDocument = GetDocument();

	if (pDocument)
		pLocation = pDocument->GetLocationVector();


	return  pLocation;
}

CPropertiesWnd::CPropertiesWnd()
{
	//m_nComboHeight = 0;
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	//ON_EN_KILLFOCUS(IDC_VIAL, OnVialKillFocus)
END_MESSAGE_MAP()


/*
void CPropertiesWnd::OnVialKillFocus()
{
CWeatherDatabasePtr& pDB = GetDatabasePtr(); ASSERT(pDB);
if (pDB)
{
CString vialNoStr;
m_vialCtrl.GetWindowText(vialNoStr);

if (!vialNoStr.IsEmpty())
{
int VialNo = UtilWin::ToInt(vialNoStr);

CDPTProjectProperties& properties = pDB->m_properties;
const CStudiesDefinitions& stydies = pDB->m_studiesDefinitions;
ASSERT(stydies.find(properties.m_studyName) != stydies.end());
const CStudyDefinition& study = stydies.at(properties.m_studyName);

int curRow = VialNo - ToInt(study.m_firstVial);
if (curRow >= 0 && curRow<study.GetNbVials())
{
if (curRow != properties.m_curRow)
{
properties.m_curRow = curRow;
GetDocument()->UpdateAllViews(NULL, CMatchStationDoc::PROPERTIES_CHANGE, NULL);
}
}
}
}
}
*/

/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de CResourceViewBar

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd() == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	//int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;
	//int m_nComboHeight = cyTlb;


	//CString text;
	//m_titleCtrl.GetWindowText(text);
	//CSize size;
	//GetTextExtentPoint(GetDC()->m_hDC, text, text.GetLength(), &size);

	//m_titleCtrl.SetWindowPos(NULL, rectClient.left + 2, rectClient.top, size.cx, m_nComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_vialCtrl.SetWindowPos(NULL, rectClient.left + 2 + size.cx, rectClient.top+3, rectClient.Width() - size.cx - 2, m_nComboHeight-5, SWP_NOACTIVATE | SWP_NOZORDER);
	//m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);


}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Créer une zone de liste déroulante :
	//const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;


	//| WS_GROUP
	//if (!m_titleCtrl.Create(_T("Vial No"), WS_CHILD | WS_VISIBLE  | SS_CENTERIMAGE, rectDummy, this, IDC_STATIC))
	//{
	//	TRACE0("Impossible de créer la zone de liste déroulante des propriétés\n");
	//	return -1;      // échec de la création
	//}


	//const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP;
	//if (!m_vialCtrl.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_LEFT | ES_AUTOHSCROLL, rectDummy, this, IDC_VIAL))
	//{
	//	TRACE0("Impossible de créer la zone de liste déroulante des propriétés\n");
	//	return -1;      // échec de la création
	//}

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP, rectDummy, this, 2))
	{
		TRACE0("Impossible de créer la grille des propriétés\n");
		return -1;      // échec de la création
	}

	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
	m_wndPropList.EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);

	//InitPropList();
	SetPropListFont();

	//m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	//m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* Est verrouillé */);
	//m_wndToolBar.CleanUpLockedImages();
	//m_wndToolBar.LoadBitmap(IDR_PROPERTIES, 0, 0, TRUE /* Verrouillé */);

	//m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	//m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	//m_wndToolBar.SetOwner(this);

	//// Toutes les commandes sont routées via ce contrôle et non via le frame parent :
	//m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties()
{
	m_wndPropList.ExpandAll();
}

void CPropertiesWnd::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CPropertiesWnd::OnSortProperties()
{
	m_wndPropList.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnProperties1()
{
	// TODO: ajoutez ici le code de votre gestionnaire de commande
}

void CPropertiesWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: ajoutez ici le code du gestionnaire d'interface utilisateur de mise à jour des commandes
}

void CPropertiesWnd::OnProperties2()
{
	// TODO: ajoutez ici le code de votre gestionnaire de commande
}

void CPropertiesWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: ajoutez ici le code du gestionnaire d'interface utilisateur de mise à jour des commandes
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	//m_titleCtrl.SetFont(&m_fntPropList);
	m_wndPropList.SetFont(&m_fntPropList);
	//m_vialCtrl.SetFont(&m_fntPropList);
}

//CMatchStationDoc* CPropertiesWnd::GetDocument()
//{
//	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMatchStationDoc)));
//	return (CMatchStationDoc*)m_pDocument;
//}


void CPropertiesWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	m_wndPropList.OnUpdate(pSender, lHint, pHint);
}

BOOL CPropertiesWnd::PreTranslateMessage(MSG* pMsg)
{
	//if (pMsg->message == WM_KEYDOWN &&
	//	pMsg->wParam == VK_RETURN &&
	//	GetFocus() == &m_vialCtrl )
	//{
	//	// handle return pressed in edit control
	//	//OnVialKillFocus();
	//	m_wndPropList.SetFocus();
	//	return TRUE; // this doesn't need processing anymore
	//}

	return CDockablePane::PreTranslateMessage(pMsg); // all other cases still need default processing
}



LRESULT CPropertiesWnd::OnSetText(WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = Default(); //let it do the default thing if you want

	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
	m_wndPropList.EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
	//do your stuff
	return Result;
}
