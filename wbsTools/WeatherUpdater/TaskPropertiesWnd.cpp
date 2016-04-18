//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//****************************************************************************
#include "stdafx.h"
#include "boost/dynamic_bitset.hpp"
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/FileNameProperty.h"
#include "UI/Common/AppOption.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/SelectionCtrl.h"
#include "TaskPropertiesWnd.h"
#include "WeatherUpdaterDoc.h"

#include "WeatherBasedSimulationString.h"
#include "resource.h"


using namespace UtilWin;
using namespace std;
using namespace WBSF;



IMPLEMENT_SERIAL(CPropertiesToolBar, CMFCToolBar, 1)

//**********************************************************************************************
IMPLEMENT_DYNAMIC(CTaskPropertyGridCtrl, CMFCPropertyGridCtrl)
BEGIN_MESSAGE_MAP(CTaskPropertyGridCtrl, CMFCPropertyGridCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CTaskPropertyGridCtrl::CTaskPropertyGridCtrl()
{
	m_curAttibute = NOT_INIT;
}

void CTaskPropertyGridCtrl::Init()
{
	CMFCPropertyGridCtrl::Init();

	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
	EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
	SetBoolLabels(UtilWin::GetCString(IDS_STR_TRUE), UtilWin::GetCString(IDS_STR_FALSE));
	EnableDescriptionArea(false);
	SetVSDotNetLook(true);
	MarkModifiedProperties(true);
	SetAlphabeticMode(false);
	SetShowDragContext(true);
	EnableWindow(true);
	AlwaysShowUserToolTip();
}

int CTaskPropertyGridCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCPropertyGridCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void CTaskPropertyGridCtrl::OnSize(UINT nType, int cx, int cy)
{
	//bypass  CMFCPropertyGridCtrl::OnSize(nType, cx, cy);
	CWnd::OnSize(nType, cx, cy);

	EndEditItem();
	AdjustLayout();
}


void CTaskPropertyGridCtrl::Update()
{
	RemoveAll();
	m_curAttibute = NOT_INIT;

	if (m_pTask.get() != NULL)
	{
		CWeatherUpdaterDoc* pDoc = CTaskPropertyWnd::GetDocument();

		CTaskAttributes attributes;
		m_pTask->GetAttributes(attributes);
		
		string description;
		StringVector tmp(GetString(m_pTask->GetDescriptionStringID()), "\r\n");
		if (!tmp.empty())
			description = tmp[0];
		

		AddProperty(new CStdReadOnlyProperty(GetString(IDS_TASK_DESCRIPTION), description));
		for (size_t i = 0; i<attributes.size(); i++)
		{
			CMFCPropertyGridProperty* pItem = NULL;
			string str = m_pTask->Get(i);
			
			
			if (attributes[i].m_type == T_PATH && str.empty())
				str = GetPath(pDoc->GetFilePath());

			switch (attributes[i].m_type)
			{
			case T_STRING:			pItem = new CStdGridProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
			case T_STRING_BROWSE:	pItem = new CStdBrowseProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			case T_BOOL:			pItem = new CBoolGridProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
			case T_COMBO_POSITION:	pItem = new CStdComboPosProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			case T_COMBO_STRING:	pItem = new CStdComboStringProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			case T_PATH:			pItem = new CStdGriFolderProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			case T_FILEPATH:		pItem = new CStdGriFilepathProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			case T_GEOPOINT:		pItem = new CStdGridProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
			case T_GEORECT:			pItem = new CStdGeoRectProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
			case T_PASSWORD:		pItem = new CStdPasswordProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
			case T_DATE:			pItem = new CStdTRefProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			case T_UPDATER:			pItem = new CStdComboStringProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;//always relead options
			case T_URL:			    pItem = new CStdGriFolderProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
			default: ASSERT(false);
			}

			AddProperty(pItem);
		}
	}

	Invalidate();
	EnableProperties(m_pTask.get() != NULL);
}

void CTaskPropertyGridCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
{
	ASSERT(m_pTask.get() != NULL);
	CTaskPropertyGridCtrl& me = const_cast<CTaskPropertyGridCtrl&>(*this);


	CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);
	size_t i = (size_t)pPropIn->GetData();
	if (i > 1000)
	{
		i /= 1000;
		pProp = (CStdGridProperty* )FindItemByData(i);
	}
	
	std::string str = pProp->get_string();

	

	m_pTask->Set(i, str);
}



void CTaskPropertyGridCtrl::EnableProperties(BOOL bEnable)
{
	for (int i = 0; i < GetPropertyCount(); i++)
	{
		CMFCPropertyGridProperty* pProp = GetProperty(i);
		EnableProperties(pProp, bEnable);
	}
}

void CTaskPropertyGridCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
{
	pProp->Enable(bEnable);

	for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
		EnableProperties(pProp->GetSubItem(ii), bEnable);
}

void CTaskPropertyGridCtrl::OnChangeSelection(CMFCPropertyGridProperty* pNewSel, CMFCPropertyGridProperty* pOldSel)
{
	m_curAttibute = NOT_INIT;
	if (pNewSel)
		m_curAttibute = (size_t)pNewSel->GetData();
}

BOOL CTaskPropertyGridCtrl::PreTranslateMessage(MSG* pMsg)
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
				size_t newID = (ID + 1) % GetPropertyCount();
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
	}

	return CMFCPropertyGridCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing;
}

void CTaskPropertyGridCtrl::OnDestroy()
{
	CMFCPropertyGridCtrl::OnDestroy();
}




//**************************************************************************************************************

static const UINT PROPERTY_GRID_ID = 1000;
IMPLEMENT_DYNCREATE(CTaskPropertyWnd, CDockablePane)

BEGIN_MESSAGE_MAP(CTaskPropertyWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_OPEN_PROPERTY, OnUpdateToolBar)
	ON_COMMAND(ID_OPEN_PROPERTY, OnOpenProperty)
END_MESSAGE_MAP()



CWeatherUpdaterDoc* CTaskPropertyWnd::GetDocument()
{
	CWeatherUpdaterDoc* pDoc = NULL;
	CWinApp* pApp = AfxGetApp();
	if (pApp)
	{
		POSITION  pos = pApp->GetFirstDocTemplatePosition();
		CDocTemplate* docT = pApp->GetNextDocTemplate(pos);
		if (docT)
		{
			pos = docT->GetFirstDocPosition();
			pDoc = (CWeatherUpdaterDoc*)docT->GetNextDoc(pos);
		}
	}

	return pDoc;
}

CTaskPropertyWnd::CTaskPropertyWnd() 
{}


CTaskPropertyWnd::~CTaskPropertyWnd()
{}

int CTaskPropertyWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	//add all attribute
	CAppOption options;
	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));

	UINT style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	VERIFY(m_propertiesCtrl.Create(style, CRect(), this, PROPERTY_GRID_ID));

	VERIFY(m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE | CBRS_SIZE_DYNAMIC, IDR_PROPERTIES_TOOLBAR));
	VERIFY(m_wndToolBar.LoadToolBar(IDR_PROPERTIES_TOOLBAR, 0, 0, TRUE /* Is locked */));
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);
	

	return 0;
}
void CTaskPropertyWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	AdjustLayout();
}

void CTaskPropertyWnd::AdjustLayout()
{
	static const int MARGE = 10;
	if (GetSafeHwnd() == NULL || m_propertiesCtrl.GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rect;
	GetClientRect(rect);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, 0, 0, rect.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	m_propertiesCtrl.SetWindowPos(NULL, rect.left, rect.top + cyTlb, rect.Width(), rect.Height() - cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
}
//
//void CTaskPropertyWnd::OnInitialUpdate()
//{
//	CWeatherUpdaterDoc* pDoc = static_cast<CWeatherUpdaterDoc*>(GetDocument());
//	ASSERT(pDoc);
//	pDoc->OnInitialUpdate();
//}

void CTaskPropertyWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);

	if (lHint == CWeatherUpdaterDoc::INIT)
	{
		CRect rect;
		GetClientRect(rect);

		//add all attribute
		CAppOption options;
		int width = options.GetProfileInt(_T("ParametersPropertiesSplitterPos"), rect.Width() / 2);
		m_propertiesCtrl.SetPropertyColumnWidth(width);
	}

	if (lHint == CWeatherUpdaterDoc::INIT || lHint == CWeatherUpdaterDoc::SELECTION_CHANGE || 
		lHint == CWeatherUpdaterDoc::ADD_TASK || lHint == CWeatherUpdaterDoc::REMOVE_TASK ||
		lHint == CWeatherUpdaterDoc::LANGUAGE_CHANGE)
	{
		size_t t = pDoc->GetCurT();
		ASSERT(t != NOT_INIT);

		size_t p = pDoc->GetCurP(t);
		
		m_propertiesCtrl.m_pTask = pDoc->GetTask(t, p);
		m_propertiesCtrl.Update();
	}
	
}

void CTaskPropertyWnd::OnUpdateToolBar(CCmdUI *pCmdUI)
{
	size_t i = m_propertiesCtrl.GetCurAtt();
	if (i > 1000 && i<NOT_INIT)
		i /= 1000;

	bool bEnable = false;

	if (i!=NOT_INIT)
	{
		CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
		ASSERT(pDoc);
		
		const CTaskBase* pTask = m_propertiesCtrl.m_pTask.get();
		ASSERT(pTask);

		size_t type = pTask->Type(i);
		bEnable = type == T_FILEPATH || type == T_PATH;
	}

	
	pCmdUI->Enable(bEnable);

}

void CTaskPropertyWnd::OnOpenProperty()
{
	CStdGridProperty* pProp = static_cast<CStdGridProperty*>(m_propertiesCtrl.GetCurSel());
	ENSURE(pProp);
	
	std::string str = pProp->get_string();
	ShellExecuteW(m_hWnd, L"open", CString(str.c_str()), NULL, NULL, SW_SHOW);
}

void CTaskPropertyWnd::OnDestroy()
{
	CRect rectClient;
	GetWindowRect(rectClient);

	CAppOption option;
	option.WriteProfileRect(_T("DispersalDlgRect"), rectClient);
	option.WriteProfileInt(_T("ParametersPropertiesSplitterPos"), m_propertiesCtrl.GetLeftColumnWidth());


	CDockablePane::OnDestroy();
}


BOOL CTaskPropertyWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the view to route command
	//CWnd* pFocus = GetFocus();
	//if (pFocus)
	//{
		//CWnd* pParent = pFocus->GetParent();

		//if (pFocus == &m_propertiesCtrl || pParent == &m_propertiesCtrl)
		//{
			if (m_propertiesCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		//}
	//}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
