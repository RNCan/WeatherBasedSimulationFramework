#include "stdafx.h"

#include "UI/Common/AppOption.h"

#include "InputWnd.h"
#include "MatchStationDoc.h"
#include "Resource.h"




#include "UI/Common/UtilWin.h"

#include "MatchStationApp.h"
#include "PropertiesWnd.h"
#include "MainFrm.h"
#include "MatchStationDoc.h"
#include "WeatherBasedSimulationString.h"


using namespace std;
using namespace WBSF;



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif



using namespace WBSF;



//*****************************************************************************************************

const int CInputPropertyCtrl::INPUT_TYPE[NB_INPUTS] = { IT_FILEPATH, IT_OBS_TYPE, IT_FILEPATH, IT_FILEPATH, IT_FILEPATH, IT_VAR, IT_STRING, IT_STRING, IT_STRING, IT_BOOL };


BEGIN_MESSAGE_MAP(CInputPropertyCtrl, CMFCPropertyGridCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

CInputPropertyCtrl::CInputPropertyCtrl()
{
	m_nLeftColumnWidth = 150;
	m_lastCol = -1;
	m_lastRow = -1;
	m_lastIndex = UNKNOWN_POS;
}


int CInputPropertyCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCPropertyGridCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;


	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
	EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
	EnableDescriptionArea(TRUE);
	SetDescriptionRows(2);
	SetVSDotNetLook(TRUE);
	SetGroupNameFullWidth(TRUE);
	MarkModifiedProperties();

	return 0;
}

void CInputPropertyCtrl::OnSize(UINT nType, int cx, int cy)
{
	//bypass  CMFCPropertyGridCtrl::OnSize(nType, cx, cy);
	CWnd::OnSize(nType, cx, cy);

	EndEditItem();
	AdjustLayout();
}


void CInputPropertyCtrl::SetPropListFont()
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
	SetFont(&m_fntPropList);
}


void CInputPropertyCtrl::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	if (lHint == CMatchStationDoc::LANGUAGE_CHANGE)
		Invalidate();


	CMatchStationDoc* pDoc = CInputWnd::GetDocument();

	if (lHint == CMatchStationDoc::INIT )
	{
		RemoveAll();

		
		typedef CStdIndexProperty < IDS_OBSERVATION_TYPE> CObservationTypeProperty;
		typedef CStdIndexProperty < IDS_STR_WEATHER_VARIABLES_TITLE> CWeatherVariableProperty;

		StringVector title(IDS_INPUT_TITLE, ";|");
		StringVector description(IDS_INPUT_DESCRIPTION, ";|");

		ASSERT(title.size() == NB_INPUTS);

		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			string value;
			std::string filter;

			switch (i)
			{
			case LOC_FILEPATH:		if (pDoc->GetLocations())value = pDoc->GetLocations()->GetFilePath(); filter = GetString(IDS_STR_FILTER_CSV); break;
			case OBSERVATION_TYPE:	value = ToString(pDoc->GetObservationType()); break;
			case HOURLY_FILEPATH:	if (pDoc->GetHourlyDatabase())value = pDoc->GetHourlyDatabase()->GetFilePath(); filter = GetString(IDS_STR_FILTER_HOURLY); break;
			case DAILY_FILEPATH:	if (pDoc->GetDailyDatabase())value = pDoc->GetDailyDatabase()->GetFilePath(); filter = GetString(IDS_STR_FILTER_DAILY); break;
			case NORMALS_FILEPATH:	if (pDoc->GetNormalsDatabase())value = pDoc->GetNormalsDatabase()->GetFilePath(); filter = GetString(IDS_STR_FILTER_NORMALS); break;
			case VARIABLES:			value = ToString(pDoc->GetVariable()); break; 
			case YEAR:				value = ToString(pDoc->GetYear()); break;
			case NB_STATIONS:		value = ToString(pDoc->GetNbStation()); break; 
			case SEARCH_RADIUS:		value = ToString(pDoc->GetSearchRadius()); break;
			case SKIP_VERIFY:		value = ToString(pDoc->GetSkipVerify()); break;
			default: ASSERT(false);
			}
			
			CMFCPropertyGridProperty* pItem = NULL;
			switch (INPUT_TYPE[i])
			{
			case IT_FILEPATH:	pItem = new CStdGriFilepathProperty(title[i], value, description[i], filter, i); break;
			case IT_OBS_TYPE:	pItem = new CObservationTypeProperty(title[i], value, description[i], i); break;
			case IT_VAR:		pItem = new CWeatherVariableProperty(title[i], value, description[i], i); break;
			case IT_STRING:		pItem = new CStdGridProperty(title[i], value, description[i], i); break;
			case IT_BOOL:		pItem = new CStdBoolGridProperty(title[i], as<bool>(value), description[i], i); break;
			default: ASSERT(false);
			}
			
			
			AddProperty(pItem);
		}

		EnableWindow(true);
		//EnableProperties(true);
	}
}


//void CInputPropertyCtrl::EnableProperties(BOOL bEnable)
//{
//	for (int i = 0; i < GetPropertyCount(); i++)
//	{
//		CMFCPropertyGridProperty* pProp = GetProperty(i);
//		EnableProperties(pProp, bEnable);
//	}
//}
//
//void CInputPropertyCtrl::EnableProperties(CMFCPropertyGridProperty* pProp, BOOL bEnable)
//{
//	pProp->Enable(bEnable);
//
//	for (int ii = 0; ii < pProp->GetSubItemsCount(); ii++)
//		EnableProperties(pProp->GetSubItem(ii), bEnable);
//}
//

void CInputPropertyCtrl::OnPropertyChanged(CMFCPropertyGridProperty* pPropIn) const
{
	CStdGridProperty* pProp = static_cast<CStdGridProperty*>(pPropIn);

	CMatchStationDoc* pDoc = CInputWnd::GetDocument();

	int i = pProp->GetData();
	string val = pProp->get_string();

	switch (i)
	{
	case LOC_FILEPATH:		pDoc->SetLocationFilePath(val); break;
	case OBSERVATION_TYPE:	pDoc->SetObservationType(ToSizeT(val)); break;
	case HOURLY_FILEPATH:	pDoc->SetHourlyFilePath(val); break;
	case DAILY_FILEPATH:	pDoc->SetDailyFilePath(val); break;
	case NORMALS_FILEPATH:	pDoc->SetNormalsFilePath(val); break;
	case VARIABLES:			pDoc->SetVariable((HOURLY_DATA::TVarH)ToSizeT(val)); break;
	case YEAR:				pDoc->SetYear(ToInt(val)); break;
	case NB_STATIONS:		pDoc->SetNbStation(ToSizeT(val)); break;
	case SEARCH_RADIUS:		pDoc->SetSearchRadius(ToDouble(val)); break;
	case SKIP_VERIFY:		pDoc->SetSkipVerify(ToBool(val)); break;
	default: ASSERT(false);
	}


}


BOOL CInputPropertyCtrl::PreTranslateMessage(MSG* pMsg)
{

	//if (pMsg->message == WM_KEYDOWN)
	//{
	//	
	//	if (m_pSel != NULL && m_pSel->IsInPlaceEditing() != NULL && m_pSel->IsEnabled())
	//	{
	//		BOOL bAlt = GetKeyState(VK_CONTROL) & 0x8000;
	//		if (pMsg->wParam == VK_RETURN)
	//		{

	//			if (m_pSel->GetOptionCount() > 0)
	//				OnSelectCombo();

	//			EndEditItem();

	//			//select next item
	//			size_t ID = m_pSel->GetData();
	//			size_t newID = (ID + 1) % GetPropertyCount();
	//			CMFCPropertyGridProperty* pNext = FindItemByData(newID);
	//			if (pNext)
	//			{
	//				SetCurSel(pNext);
	//				EditItem(pNext);
	//			}

	//			return TRUE; // this doesn't need processing anymore
	//		}
	//		else if (pMsg->wParam == VK_DOWN && bAlt)
	//		{
	//			m_pSel->OnClickButton(CPoint(-1, -1));
	//			return TRUE; // this doesn't need processing anymore
	//		}
	//	}
	//}

	return CMFCPropertyGridCtrl::PreTranslateMessage(pMsg); // all other cases still need default processing;
}


//**********************************************************************************************************
/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CMatchStationDoc* CInputWnd::GetDocument()
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


CInputWnd::CInputWnd()
{
}

CInputWnd::~CInputWnd()
{
}

BEGIN_MESSAGE_MAP(CInputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETTINGCHANGE()
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_SETTEXT, OnSetText)
	//ON_EN_KILLFOCUS(IDC_VIAL, OnVialKillFocus)
END_MESSAGE_MAP()


//
//void CInputWnd::OnKillFocus()
//{
//	CWeatherDatabasePtr& pDB = GetDatabasePtr(); ASSERT(pDB);
//	if (pDB)
//	{
//		CString vialNoStr;
//		m_vialCtrl.GetWindowText(vialNoStr);
//
//		if (!vialNoStr.IsEmpty())
//		{
//			int VialNo = UtilWin::ToInt(vialNoStr);
//
//			CDPTProjectProperties& properties = pDB->m_properties;
//			const CStudiesDefinitions& stydies = pDB->m_studiesDefinitions;
//			ASSERT(stydies.find(properties.m_studyName) != stydies.end());
//			const CStudyDefinition& study = stydies.at(properties.m_studyName);
//
//			int curRow = VialNo - ToInt(study.m_firstVial);
//			if (curRow >= 0 && curRow < study.GetNbVials())
//			{
//				if (curRow != properties.m_curRow)
//				{
//					properties.m_curRow = curRow;
//					GetDocument()->UpdateAllViews(NULL, CMatchStationDoc::PROPERTIES_CHANGE, NULL);
//				}
//			}
//		}
//	}
//}


/////////////////////////////////////////////////////////////////////////////
// Gestionnaires de messages de CResourceViewBar

int CInputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	
	VERIFY(m_inputCtrl.Create(WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CRect(), this, 1001));

	//add all attribute
//	CAppOption options;
	//m_inputCtrl.SetLeftColumnWidth(options.GetProfileInt(_T("InputsPropertiesSplitterPos"), 150));

	return 0;
}

void CInputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CInputWnd::AdjustLayout()
{
	CRect rect;
	GetClientRect(rect);

	if (GetSafeHwnd() != NULL )
		m_inputCtrl.SetWindowPos(NULL, 0, 0, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER| SWP_NOMOVE);

}


void CInputWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_inputCtrl.SetFocus();
}

void CInputWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	m_inputCtrl.SetPropListFont();
}


void CInputWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	//if (lHint == CMatchStationDoc::INIT)
	//{
	//	//add all attribute
	//	CAppOption options;
	//	m_inputCtrl.SetLeftColumnWidth(options.GetProfileInt(_T("InputsPropertiesSplitterPos"), 150));
	//}

	m_inputCtrl.OnUpdate(pSender, lHint, pHint);
}

LRESULT CInputWnd::OnSetText(WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = Default(); //let it do the default thing if you want

	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));
	m_inputCtrl.EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);

	return Result;
}

void CInputWnd::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CAppOption options;

	CRect rectClient;
	GetWindowRect(rectClient);
	
	if (bShow)
		m_inputCtrl.SetLeftColumnWidth(options.GetProfileInt(_T("InputsPropertiesSplitterPos"), 150));
	else 
		options.WriteProfileInt(_T("InputsPropertiesSplitterPos"), m_inputCtrl.GetLeftColumnWidth());
		

}

void CInputWnd::OnDestroy()
{
	CRect rectClient;
	GetWindowRect(rectClient);

	CAppOption option;
	option.WriteProfileInt(_T("InputsPropertiesSplitterPos"), m_inputCtrl.GetLeftColumnWidth());

	CDockablePane::OnDestroy();
}

BOOL CInputWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the trl to route command
	CWnd* pFocus = GetFocus();
	if (pFocus)
	{
		CWnd* pParent = pFocus->GetParent();

		if (pFocus == &m_inputCtrl || pParent == &m_inputCtrl)
		{
			if (m_inputCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
				return TRUE;
		}
	}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}