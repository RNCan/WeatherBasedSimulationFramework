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
#include "ParametersWnd.h"
#include "WeatherUpdaterDoc.h"
#include "MFCPropertyGridDateTimeProperty.h"   


#include "WeatherBasedSimulationString.h"
#include "resource.h"


using namespace UtilWin;
using namespace std;
using namespace WBSF;

IMPLEMENT_DYNAMIC(CTaskPropertyWnd, CDockablePane)
IMPLEMENT_DYNAMIC(CTaskPropertyGridCtrl, CMFCPropertyGridCtrl)


class CStdGriFolderProperty2 : public CStdGriInterface, public CMFCPropertyGridFileProperty
{
	friend class CMFCPropertyGridCtrl;

public:

	CStdGriFolderProperty2(const std::string& name, const std::string& folderName, const std::string& description, const std::string& filter, size_t no) :
		CMFCPropertyGridFileProperty(CString(name.c_str()), CString(folderName.c_str()), (DWORD_PTR)no, CString(description.c_str()))
	{
		m_filter = filter;
	}

	virtual std::string get_string(){ return std::string(CStringA(GetValue())); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

	virtual void OnClickButton(CPoint point)
	{
		CString strFolder = GetValue();
		CString strResult;
		
		//m_ulBrowseFolderFlags = BIF_NEWDIALOGSTYLE;
		afxShellManager->BrowseForFolder(strResult, m_pWndList, strFolder, 0, BIF_NEWDIALOGSTYLE);
		if (strResult != strFolder)
		{
			SetValue(strResult);
		}

	}

	std::string m_filter;
	//virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
	//{
	//	CWnd* pWnd = CMFCPropertyGridFileProperty::CreateInPlaceEdit(rectEdit, bDefaultFormat);
	//	
	//	/*CEdit* pWndEdit = new CEdit;

	//	DWORD dwStyle = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD;

	//	if (!m_bEnabled || !m_bAllowEdit)
	//		dwStyle |= ES_READONLY;

	//	pWndEdit->Create(dwStyle, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
	//	pWndEdit->SetPasswordChar(_T('*'));
	//	bDefaultFormat = TRUE;*/

	//	return pWnd;
	//}
};


//class CMFCPropertyGridDateTimeProperty : public CMFCPropertyGridProperty 
//{ 
//	DECLARE_DYNAMIC(CMFCPropertyGridDateTimeProperty) 
//	BOOL m_style; 
//	BOOL m_updown; 
//	CString m_setformat; 
//	CString m_format; 
//public: 
//	CMFCPropertyGridDateTimeProperty(const CString& strName, COleDateTime &nValue, LPCTSTR lpszDescr = NULL, DWORD dwData = 0, BOOL style = TRUE, BOOL updown = FALSE, LPCTSTR setFormat = NULL, LPCTSTR format = NULL); 
//	virtual BOOL OnUpdateValue(); 
//	virtual CString FormatProperty(); 
//	virtual CString FormatOriginalProperty(); 
//
//protected: 
//	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat); virtual BOOL OnSetCursor() const { return FALSE; /* Use default */ } BOOL IsValueChanged() const; }; -See more at : http ://www.codexpert.ro/articole.php?id=20#sthash.1Pq5mmxc.dpuf
//#define WM_PG_DATESELCHANGED WM_USER+489

// CPropertyGridDateTimeCtrl

//class CPropertyGridDateTimeCtrl : public CDateTimeCtrl
//{
//	DECLARE_DYNAMIC(CPropertyGridDateTimeCtrl)
//
//public:
//	CPropertyGridDateTimeCtrl();
//	virtual ~CPropertyGridDateTimeCtrl();
//
//protected:
//	DECLARE_MESSAGE_MAP()
//public:
//	afx_msg void OnKillFocus(CWnd* pNewWnd);
//	afx_msg void OnMcnSelect(NMHDR *pNMHDR, LRESULT *pResult);
//	afx_msg UINT OnGetDlgCode();
//	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
//};
//
//
//
//
//IMPLEMENT_DYNAMIC(CPropertyGridDateTimeCtrl, CDateTimeCtrl)
//CPropertyGridDateTimeCtrl::CPropertyGridDateTimeCtrl()
//{
//}
//
//CPropertyGridDateTimeCtrl::~CPropertyGridDateTimeCtrl()
//{
//}
//
//
//BEGIN_MESSAGE_MAP(CPropertyGridDateTimeCtrl, CDateTimeCtrl)
//	ON_WM_KILLFOCUS()
//	ON_NOTIFY_REFLECT(MCN_SELECT, OnMcnSelect)
//	ON_WM_GETDLGCODE()
//	ON_WM_KEYDOWN()
//END_MESSAGE_MAP()
//
//
//
//// CPropertyGridDateTimeCtrl message handlers
//
//
//void CPropertyGridDateTimeCtrl::OnKillFocus(CWnd* pNewWnd)
//{
//	CDateTimeCtrl::OnKillFocus(pNewWnd);
//	CWnd* pParent = pNewWnd ? pNewWnd->GetParent() : NULL;
//	if (pParent != this)
//		DestroyWindow();
//}
//
//void CPropertyGridDateTimeCtrl::OnMcnSelect(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	LPNMSELCHANGE pSelChange = reinterpret_cast<LPNMSELCHANGE>(pNMHDR);
//	GetOwner()->SendMessage(WM_PG_DATESELCHANGED);
//	*pResult = 0;
//}
//
//UINT CPropertyGridDateTimeCtrl::OnGetDlgCode()
//{
//	return DLGC_WANTALLKEYS;
//}
//
//void CPropertyGridDateTimeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
//{
//	if (nChar == VK_ESCAPE)
//	{
//		DestroyWindow();
//		return;
//	}
//	else if (nChar == VK_RETURN || nChar == VK_EXECUTE)
//	{
//		GetOwner()->SendMessage(WM_PG_DATESELCHANGED);
//		return;
//	}
//
//	CDateTimeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
//}

static CWeatherUpdaterDoc* GetDocument()
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



//template<int BASE_INDEX = 0, bool ADD_EMPTY = false>
class CGridComboPosProperty2 : public CGridComboStringProperty
{

public:

	CGridComboPosProperty2(const std::string& strName, const std::string& value, const std::string& description, const std::string& options, size_t dwData) :
		CGridComboStringProperty(strName, "", description, options, dwData)
	{
		int index = WBSF::ToInt(value);
		CString strValue = GetOptionText(index);
		SetValue(strValue);
		SetOriginalValue(strValue);
	}


	CGridComboPosProperty2(const std::string& strName, size_t index, const std::string& description, const std::string& options, size_t dwData) :
		CGridComboStringProperty(strName, "", description, options, dwData)
	{
		SetOriginalValue(GetOptionText(int(index)));
	}

	CString GetOptionText(int index)
	{
		ASSERT(index >= 0 && index < m_lstOptions.GetSize());
		POSITION pos = m_lstOptions.FindIndex(index);
		return m_lstOptions.GetAt(pos);
	}

	int GetIndex()const
	{
		int index = -1;
		if (m_pWndCombo)
		{
			index = m_pWndCombo->GetCurSel();
		}
		else
		{
			CString value = GetValue();
			for (int i = 0; i < GetOptionCount() && index == -1; i++)
				if (value == GetOption(i))
					index = i;
		}

		return index;
	}

	void SetIndex(int index)
	{
		SetValue(GetOptionText(index));
	}

	virtual std::string get_string(){ return WBSF::ToString(GetIndex()); }

	virtual void set_string(std::string str)
	{
		int index = WBSF::ToInt(str);
		if (index >= 0 && index < m_lstOptions.GetSize())
			SetIndex(index);
	}
};

//class CTRefProperty : public CStdGridProperty
class CTRefProperty : public CStdGriInterface, public CMFCPropertyGridDateTimeProperty
{
public:

	static COleDateTime GetOleDatTime(string str)
	{
		COleDateTime v;

		CTRef TRef;
		TRef.FromFormatedString(str, "%Y-%m-%d" );
		if (TRef.IsInit())
		{
			TRef.Transform(CTM(CTM::HOURLY));
			v = COleDateTime(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0);
		}

		return v;
	}

	DECLARE_DYNAMIC(CTRefProperty)
	CTRefProperty(const std::string& name, const std::string& value, const std::string& description, const std::string& options, size_t no) :
		CMFCPropertyGridDateTimeProperty(CString(name.c_str()), GetOleDatTime(value), CString(description.c_str()), (DWORD)no)
		//(const CString& strName, COleDateTime &nValue, LPCTSTR lpszDescr = NULL, DWORD dwData = 0, BOOL style = TRUE, BOOL updown = FALSE, LPCTSTR setFormat = NULL, LPCTSTR format = NULL);
	//{}
	//CTRefProperty(const std::string& name, const std::string& value, const std::string& description, const std::string& options, size_t no) :
	//	CMFCPropertyGridProperty(CString(name.c_str()), COleDateTime(), CString(description.c_str()), (DWORD_PTR)no)
	{
		//set_string(value);
	}

	////CTRefProperty(const std::string& strName, CTRef value, const std::string& description, const std::string& options, size_t dwData) :
	//	//CStdGridProperty(strName, value, description, dwData)
	////{}

	//virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
	//{
	//	CWnd* pWndTest = CMFCPropertyGridProperty::CreateInPlaceEdit(rectEdit, bDefaultFormat);
	//	
	//	//COleVariant value = GetValue();
	//	COleDateTime value = GetValue();
	//	// create it
	//	CDateTimeCtrl* pWnd = new CDateTimeCtrl;
	//	
	//	
	//	DWORD dwStyle = WS_VISIBLE | WS_CHILD | DTS_SHORTDATECENTURYFORMAT;
	//	pWnd->Create(dwStyle, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
	//	//pWnd->CreateEx(0, MONTHCAL_CLASS, NULL, dwStyle, CRect(), m_pWndList, AFX_PROPLIST_ID_INPLACE);
	//	pWnd->SetTime(value);
	//	// now position it
	//	//CRect rc2;
	//	//pWnd->GetWindowRect(&rc2);
	//	//rc2.OffsetRect(rectEdit.right - rc2.right, 0);
	//	//pWnd->SetWindowPos(NULL, rectEdit.left, rectEdit.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
	//	
	//	return pWnd;
	//}

	//virtual BOOL OnKillFocus(CWnd* pNewWnd) 
	//{ 
	//	return CMFCPropertyGridProperty::OnKillFocus(pNewWnd);
	//}

	//virtual BOOL OnEditKillFocus() 
	//{ 
	//	return CMFCPropertyGridProperty::OnEditKillFocus();
	//}

	

	//virtual CString FormatProperty()
	//{
	//	COleDateTime value = GetValue();
	//	CString str = value.Format();

	//	return str;
	//} 

	//virtual BOOL OnUpdateValue()
	//{
	//	return CMFCPropertyGridProperty::OnUpdateValue();
	//}


	//virtual BOOL OnEndEdit()
	//{
	//	return CMFCPropertyGridProperty::OnEndEdit();
	//}

//	virtual BOOL HasButton() const{ return TRUE; }
	/*virtual void OnClickButton(CPoint point)
	{
		CMFCToolBarDateTimeCtrl
		AfxMessageBox(_T("a faire"));
	}*/

	virtual std::string get_string()
	{ 
		COleDateTime v = GetValue();
		CTRef TRef(v.GetYear(), v.GetMonth()-1, v.GetDay()-1);
		
		return TRef.GetFormatedString("%Y-%m-%d");
	}

	virtual void set_string(std::string str)
	{
		CTRef TRef;
		TRef.FromFormatedString(str, "%Y-%m-%d");
		if (TRef.IsInit())
		{
			TRef.Transform(CTM(CTM::HOURLY));
			COleDateTime v(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0);
			SetValue(v);
			SetOriginalValue(v);
		}
		
	}
};


IMPLEMENT_DYNAMIC(CTRefProperty, CMFCPropertyGridProperty)

class CGeoRectProperty : public CStdGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:

	CGeoRectProperty(const std::string& name, const std::string& value, const std::string& description, size_t no) :
		CStdGridProperty(name, no, true)
	{
		
		CGeoRect rect(-180,-90,180,90,PRJ_WGS_84);
		if (!value.empty())
		{
			std::stringstream tmp(value);
			rect << tmp;
		}

		CStdGridProperty* pProp = NULL;
		
		pProp = new CStdGridProperty("Xmin", rect.m_xMin, "Specifies the window's height", m_dwData * 1000 + 1);
		AddSubItem(pProp);

		pProp = new CStdGridProperty("Xmax", rect.m_xMax, "Specifies the window's width", m_dwData * 1000 + 2);
		AddSubItem(pProp);

		pProp = new CStdGridProperty("Ymin", rect.m_yMin, "Specifies the window's height", m_dwData * 1000 + 3);
		AddSubItem(pProp);

		pProp = new CStdGridProperty("Ymax", rect.m_yMax, "Specifies the window's width", m_dwData * 1000 + 4);
		AddSubItem(pProp);
	}

	
};


class CPasswordProperty : public CStdGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:
	
	CPasswordProperty(const std::string& name, const std::string& value, const std::string& description, size_t no) :
		CStdGridProperty(name, value, description, no)
	{
	}

	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
	{
	//	CWnd* pWnd = CStdGridProperty::CreateInPlaceEdit(rectEdit, bDefaultFormat);
		CEdit* pWndEdit = new CEdit;

		DWORD dwStyle = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL| ES_PASSWORD;

		if (!m_bEnabled || !m_bAllowEdit)
			dwStyle |= ES_READONLY;

		pWndEdit->Create(dwStyle, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
		pWndEdit->SetPasswordChar(_T('*'));
		bDefaultFormat = TRUE;

		return pWndEdit;
	}

	
//	virtual std::string get_string(){ return std::string((LPCSTR)CStringA(CString(FormatProperty()))); }
	//virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

	virtual std::string get_string(){ return std::string((LPCSTR)CStringA(CString(GetValue()))); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

	virtual CString FormatProperty()
	{
		CString value = GetValue();
		for (int i = 0; i < value.GetLength(); i++)
			value.SetAt(i, '*');
		
		return value;// CStdGridProperty::FormatProperty();
	}

	virtual BOOL OnUpdateValue()
	{
		return CStdGridProperty::OnUpdateValue();
	}
	

	virtual BOOL OnEndEdit()
	{
		return CStdGridProperty::OnEndEdit();
	}



};
class CPossibleValues : public boost::dynamic_bitset<size_t>
{
public:

	CPossibleValues(const std::string& possibleValues, const char* sep = "|;") :
		m_possibleValues(possibleValues, sep)
	{
		resize(m_possibleValues.size(), false);
	}

	std::string GetSelection(const char sep = '|')const
	{
		std::string str;
		if (!none() && !all())
		{
			for (size_type i = 0; i != size(); i++)
			{
				if (test(i))
				{
					if (!str.empty())
						str += sep;

					str += m_possibleValues[i];
				}
			}
		}

		return str;
	}

	void SetSelection(const std::string& str, const char* sep = "|;")
	{
		reset();

		if (str.empty())
		{
			set();
		}
		else
		{
			StringVector selection(str, sep);
			for (StringVector::const_iterator it = selection.begin(); it != selection.end(); it++)
			{
				size_t pos = m_possibleValues.Find(*it, false);
				if (pos < size())
				{
					set(pos);
				}
			}
		}
	}

	

	std::string GetBinary()const
	{
		std::string str;

		for (size_type i = size() - 1; i != NOT_INIT; i--)
			str += test(i) ? '1' : '0';

		return str;
	}

	void SetBinary(const std::string& str)
	{
		reset();

		for (size_type i = min(size(), str.length()) - 1; i != NOT_INIT; i--)
			set(i, str[str.length() - i - 1] != '0');
	}

protected:

	StringVector m_possibleValues;
};

class CGridBrowseProperty2 : public CStdGridProperty
{
public:

	CGridBrowseProperty2(const std::string& strName, const std::string& value, const std::string& description, const std::string& options, size_t dwData) :
		CStdGridProperty(strName, value, description, dwData)
	{
		
		if (options.find('=') != NOT_INIT)
		{
			StringVector tmp(options, "|=");
			ASSERT(tmp.size() % 2 == 0);
			for (size_t i = 0; i < tmp.size(); i+=2)
			{
				m_possibleValues += i !=0 ? "|":"";
				m_convertValues += i != 0 ? "|":"";

				m_possibleValues += tmp[i];
				m_convertValues += tmp[i+1];
			}
		}
		else
		{
			m_possibleValues = options;
			m_convertValues = options;
		}
		
		

		SetValue(CString(value.c_str()));
		SetOriginalValue(CString(value.c_str()));
	}

	virtual BOOL HasButton() const{ return TRUE; }
	virtual void OnClickButton(CPoint point)
	{
		CSelectionDlg dlg(m_pWndList);
		CPossibleValues tmp(m_possibleValues);
		tmp.SetSelection(get_string());

		dlg.m_possibleValues = m_convertValues;
		dlg.m_selection = tmp.GetBinary();

		if (dlg.DoModal() == IDOK)
		{
			tmp.SetBinary(dlg.m_selection);
			set_string(tmp.GetSelection());
		}
			
	}

	std::string SelectionToBinary(std::string str)
	{
		std::string m_possibleValues;
	}

	virtual std::string get_string(){ return std::string((LPCSTR)CStringA(CString(GetValue()))); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

	std::string m_possibleValues;
	std::string m_convertValues;
};


IMPLEMENT_SERIAL(CPropertiesToolBar, CMFCToolBar, 1)

BEGIN_MESSAGE_MAP(CTaskPropertyGridCtrl, CMFCPropertyGridCtrl)
END_MESSAGE_MAP()

CTaskPropertyGridCtrl::CTaskPropertyGridCtrl()
{
	m_curAttibute = NOT_INIT;
}

std::string GetUpdaterList()
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);

	return pDoc->GetUpdaterList();
}

void CTaskPropertyGridCtrl::Update()
{
	RemoveAll();
	m_curAttibute = NOT_INIT;

	if (m_pTask.get() != NULL)
	{
			CTaskAttributes attributes;
			m_pTask->GetAttributes(attributes);
			

			//string GetString(IDS_PROPERTY_ROOT);
			//CMFCPropertyGridProperty* pGeneral = new CMFCPropertyGridProperty(_T("ALL"), -1);
			for (size_t i = 0; i<attributes.size(); i++)
			{
				CMFCPropertyGridProperty* pItem = NULL;
				string str = m_pTask->Get(i);
				switch (attributes[i].m_type)
				{
				case T_STRING:			pItem = new CStdGridProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
				case T_STRING_BROWSE:	pItem = new CGridBrowseProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
				case T_BOOL:			pItem = new CBoolGridProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
				case T_COMBO_POSITION:	pItem = new CGridComboPosProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
				case T_COMBO_STRING:	pItem = new CGridComboStringProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
				case T_PATH:			pItem = new CStdGriFolderProperty2(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
				case T_FILEPATH:		pItem = new CStdGriFilepathProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
				case T_GEOPOINT:		pItem = new CStdGridProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
				case T_GEORECT:			pItem = new CGeoRectProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
				case T_PASSWORD:		pItem = new CPasswordProperty(attributes[i].m_title, str, attributes[i].m_description, i); break;
				case T_DATE:			pItem = new CTRefProperty(attributes[i].m_title, str, attributes[i].m_description, attributes[i].m_option, i); break;
				case T_UPDATER:			pItem = new CGridComboStringProperty(attributes[i].m_title, str, attributes[i].m_description, m_pTask->Option(i), i); break;//always relead options
				default: ASSERT(false);
				}

				//pGeneral->AddSubItem(pItem);
				AddProperty(pItem);
			}

			//AddProperty(pGeneral);
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
	
	//pDoc->SetCurP(t, p, i)

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


//**************************************************************************************************************

//**********************************************************************************************


//
//void CTaskPropertyWnd::DoDataExchange(CDataExchange* pDX)
//{
//	CDockablePane::DoDataExchange(pDX);
//
////		DDX_Control(pDX, IDC_SIM_PROPERTIES, m_propertiesCtrl);
//
//
//
//	//if (pDX->m_bSaveAndValidate)
//	//{
//	//	m_dispersal.m_name = m_nameCtrl.GetString();
//	//	m_dispersal.m_description = m_descriptionCtrl.GetString();
//	//	m_dispersal.m_parameters = m_propertiesCtrl.Get();
//	//}
//	//else
//	//{
//	//	m_nameCtrl.SetString(m_dispersal.m_name);
//	//	m_descriptionCtrl.SetString(m_dispersal.m_description);
//	//	m_propertiesCtrl.Set(m_dispersal.m_parameters);
//	//	m_internalNameCtrl.SetString(m_dispersal.m_internalName);
//
//	//	//resize window
//	//	CRect rectClient;
//	//	GetWindowRect(rectClient);
//
//	//	CAppOption option;
//	//	rectClient = option.GetProfileRect(_T("DispersalDlgRect"), rectClient);
//	//	UtilWin::EnsureRectangleOnDisplay(rectClient);
//	//	SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), rectClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
//	//}
//
//
//
//}

// CTaskPropertyWnd dialog

//IMPLEMENT_DYNAMIC(CTaskPropertyWnd, CDockablePane)


static const UINT PROPERTY_GRID_ID = 1000;

BEGIN_MESSAGE_MAP(CTaskPropertyWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI(ID_OPEN_PROPERTY, OnUpdateToolBar)
	ON_COMMAND(ID_OPEN_PROPERTY, OnOpenProperty)

	
END_MESSAGE_MAP()



CTaskPropertyWnd::CTaskPropertyWnd() 
{}


CTaskPropertyWnd::~CTaskPropertyWnd()
{}

int CTaskPropertyWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;


	//add all attribute
	CAppOption options;
	CStringArrayEx propertyHeader(UtilWin::GetCString(IDS_STR_PROPERTY_HEADER));

	UINT style = WS_CHILD | WS_VISIBLE;
	VERIFY(m_propertiesCtrl.Create(style, CRect(), this, PROPERTY_GRID_ID));
	m_propertiesCtrl.SetBoolLabels(UtilWin::GetCString(IDS_STR_TRUE), UtilWin::GetCString(IDS_STR_FALSE));
	m_propertiesCtrl.EnableHeaderCtrl(true, propertyHeader[0], propertyHeader[1]);
	m_propertiesCtrl.EnableDescriptionArea(false);
	m_propertiesCtrl.SetVSDotNetLook(true);
	m_propertiesCtrl.MarkModifiedProperties(true);
	m_propertiesCtrl.SetAlphabeticMode(false);
	m_propertiesCtrl.SetShowDragContext(true);
	m_propertiesCtrl.EnableWindow(true);
	m_propertiesCtrl.AlwaysShowUserToolTip();
	 
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

void CTaskPropertyWnd::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
	CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	ASSERT(pDoc);


	if (lHint == CWeatherUpdaterDoc::INIT || lHint == CWeatherUpdaterDoc::SELECTION_CHANGE || 
		lHint == CWeatherUpdaterDoc::ADD_TASK || lHint == CWeatherUpdaterDoc::REMOVE_TASK)
	{
		size_t t = pDoc->GetCurT();
		ASSERT(t != NOT_INIT);

		size_t p = pDoc->GetCurP(t);
		
		m_propertiesCtrl.m_pTask = pDoc->GetTask(t, p);
		m_propertiesCtrl.Update();
	}
	else if (lHint == CWeatherUpdaterDoc::TASK_CHANGE)
	{
		////only execute and name can change
		//size_t p = pDoc->GetCurPos(m_type);
		//ASSERT(p != NOT_INIT);

		//HTREEITEM hItem = m_taskCtrl.FindItem(p);
		//CTaskPtr& pTask = pDoc->GetTask(m_type, p);
		//BOOL bChecked = ToBool(pTask->Get(CTaskBase::EXECUTE));
		//if (m_taskCtrl.GetCheck(hItem) != bChecked)
		//	m_taskCtrl.SetCheck(hItem, bChecked);

		//std::string name = pTask->Get(CTaskBase::NAME);
		//if (m_taskCtrl.GetIte.GetCheck(hItem) != name)
		//m_taskCtrl.SetCheck(hItem, bChecked);

	}

	
}

void CTaskPropertyWnd::OnUpdateToolBar(CCmdUI *pCmdUI)
{
	size_t i = m_propertiesCtrl.GetCurAtt();
	if (i > 1000 && i<NOT_INIT)
		i /= 1000;

	//CStdGridProperty* pProp = static_cast<CStdGridProperty*>(m_propertiesCtrl.GetCurSel());
	bool bEnable = false;

	if (i!=NOT_INIT)
	{
		CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
		ASSERT(pDoc);
		
		const CTaskBase* pTask = m_propertiesCtrl.m_pTask.get();
		ASSERT(pTask);

		size_t type = pTask->Type(i);
		bEnable = type == T_FILEPATH || type == T_FILEPATH;
	}

	
	pCmdUI->Enable(bEnable);

}

void CTaskPropertyWnd::OnOpenProperty()
{
	//CWeatherUpdaterDoc* pDoc = (CWeatherUpdaterDoc*)GetDocument();
	//ASSERT(!pDoc);

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

	CDockablePane::OnDestroy();
}


BOOL CTaskPropertyWnd::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	//let the view to route command
	//CWnd* pFocus = GetFocus();
	//if (pFocus)
	//{
		//CWnd* pParent = pFocus->GetParent();
		//CWnd* pOwner = pFocus->GetParentOwner();

		//if (pFocus == &m_propertiesCtrl || pParent == &m_propertiesCtrl || pOwner == &m_propertiesCtrl)
		//{
		if (m_propertiesCtrl.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
//		}
	//}

	return CDockablePane::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
