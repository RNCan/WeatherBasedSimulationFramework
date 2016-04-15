//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <boost/dynamic_bitset.hpp>
#include <string>

#include "Basic/UtilTime.h"
#include "UI/Common/UtilWin.h"
#include "UI/Common/SelectionCtrl.h"
#include "UI/Common/BioSIMListBox.h"
#include "UI/Common/OpenDirEditCtrl.h"
#include "UI/Common/MFCEditBrowseCtrlEx.h"
#include "UI/Common/MFCPropertyGridDateTimeProperty.h"   
#include "UI/Common/HtmlTree/XHtmlTree.h"



class CCFLComboBox : public CComboBox
{
public:

	using CComboBox::GetWindowText;
	CString GetWindowText()const;
	std::string GetString()const{ return UtilWin::ToUTF8(GetWindowText()); }

	using CComboBox::SetWindowText;
	void SetWindowText(const std::string& str){ CComboBox::SetWindowText(CString(str.c_str())); }

	using CComboBox::SelectString;
	int SelectString(int nStartAfter, const std::string& str){ return SelectString(nStartAfter, CString(str.c_str())); }

	using CComboBox::FindString;
	int FindString(int nStartAfter, const std::string& str) const{ return FindString(nStartAfter, str.c_str()); }

	int SelectStringExact(int nStartAfter, LPCTSTR lpszString, int defaultSel = 0);
	int SelectStringExact(int nStartAfter, const std::string& str, int defaultSel = 0){ return SelectStringExact(nStartAfter, UtilWin::Convert(str), defaultSel); }
	int GetItemDataIndex(int itemData)const;
	int GetCurItemData()const;
	int SelectFromItemData(int itemData, int defaultSel = 0);
	void FillList(const WBSF::StringVector& list, std::string selection = "");

	using CComboBox::AddString;
	int AddString(const std::string& str){ return CComboBox::AddString(CString(str.c_str())); }

};

class CDefaultComboBox : public CCFLComboBox
{
public:

	CDefaultComboBox(CString str = CString(WBSF::STRDEFAULT));
	
	CString GetWindowText()const
	{
		CString tmp;
		if (GetCurSel() > 0)
			tmp = CCFLComboBox::GetWindowText();

		return tmp;

	}
	std::string GetString()const{ return UtilWin::ToUTF8(GetWindowText()); }

	int SelectStringExact(int nStartAfter, LPCTSTR lpszString, int defaultSel = 0)
	{
		int pos = 0;
		if (lpszString == NULL || CString(lpszString).IsEmpty() || CString(lpszString) == WBSF::STRDEFAULT)
			SetCurSel(0);
		else
			pos = CCFLComboBox::SelectStringExact(nStartAfter, lpszString, defaultSel);

		return pos;
	}

	int SelectStringExact(int nStartAfter, const std::string& str, int defaultSel = 0)
	{
		return SelectStringExact(nStartAfter, UtilWin::Convert(str), defaultSel);
	}

	void FillList(const WBSF::StringVector& list, std::string selection = "");

protected:

	CString m_defaultStr;

};

class CCFLListBox : public CListBox
{
public:

	CString GetText(int i)const;
	int GetItemDataIndex(int itemData)const;

	std::string GetWindowText()
	{
		ASSERT((CListBox::GetStyle()&LBS_EXTENDEDSEL) == 0);
		CString tmp;
		if (GetCurSel() != LB_ERR)
			CListBox::GetText(GetCurSel(), tmp);
		return std::string((LPCSTR)CStringA(tmp));
	}

	using CListBox::SetWindowText;
	void SetWindowText(const std::string& str){ CListBox::SetWindowText(CString(str.c_str())); }

	void DeleteCurrentString()
	{
		int curSel = GetCurSel();
		int nCount = DeleteString(curSel);

		if (nCount > 0)
			if (curSel == nCount)SetCurSel(curSel - 1);
			else SetCurSel(curSel);
	}

	using CListBox::AddString;
	int AddString(const std::string& str){ return CListBox::AddString(CString(str.c_str())); }

};



class CCFLEdit : public CEdit
{
public:

	using CEdit::GetWindowText;
	CString GetWindowText()const
	{
		CString tmp;
		CEdit::GetWindowText(tmp);
		return tmp;
	}

	std::string GetString()const{ return UtilWin::ToUTF8(GetWindowText()); }

	using CEdit::SetWindowText;
	void SetWindowText(const std::string& tmp)
	{
		CEdit::SetWindowText(CString(tmp.c_str()));
	}

	void SetString(const std::string& tmp){ SetWindowText(tmp); }
};


class CIntEdit : public CCFLEdit
{
	// Construction
public:

	CIntEdit(short base = 10);
	virtual ~CIntEdit();

	void SetInt(__int64 val);
	__int64 GetInt()const;


	// Generated message map functions
protected:

	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()

	short m_base;
};


class CFloatEdit : public CCFLEdit
{
	// Construction
public:

	void SetFloat(double val, int pres = 4);
	double GetFloat()const;

	// Generated message map functions
protected:

	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()

};

//*******************************************************************************************
class CReadOnlyEdit : public CEdit
{
	// Construction
public:
	CReadOnlyEdit();
	virtual ~CReadOnlyEdit();

	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnUpdateToolBar(CCmdUI *pCmdUI);
	afx_msg BOOL OnToolBarCommand(UINT);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};
//*******************************************************************************************
class CStatisticComboBox : public CComboBox
{
public:

	void ReloadContent();
	static CString GetStatisticTitle(int i);

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void PreSubclassWindow();

	void Init();
	CFont m_font;

	//static CStringArrayEx STATISTIC_TITLE; 
};

//*******************************************************************************************
class CTMTypeComboBox : public CComboBox
{
public:

	CTMTypeComboBox();

	size_t m_nbTypeAvailable;

	void ReloadContent();
	static CString GetTitle(short i);

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void PreSubclassWindow();

	void Init();
	CFont m_font;
};


class CTMModeComboBox : public CComboBox
{
public:


	void ReloadContent();
	static CString GetTitle(short i);

protected:

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void PreSubclassWindow();

	void Init();
	CFont m_font;
};
//*******************************************************************************************
class CTransparentCheckBox : public CButton
{

public:

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


class CTransparentStatic : public CStatic
{
public:

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


class CTransparentEdit : public CCFLEdit
{
public:

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PreSubclassWindow();

protected:
	DECLARE_MESSAGE_MAP()

public:

	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	CBrush a_brush;
};

class CAutoEnableStatic : public CStatic
{

public:

	enum TButton{ CHECKBOX, RADIO_BUTTON };

	static void AFXAPI DDX_Check(CDataExchange* pDX, int nIDC, bool& value);

	CAutoEnableStatic(bool bShowButton = true, int buttonType = CHECKBOX);
	virtual ~CAutoEnableStatic();
	void Init();  // Though shalt always call this before using my class.
	int AddItem(int ID);       // Adds a control to the custom list
	void ClearItems();         // Empties the custom list
	void SetCheck(BOOL check); // Sets the check state and toggles dialog items
	//BOOL GetCheck()const{return m_checkbox.GetCheck();}
	BOOL GetCheck();
	BOOL EnableWindow(BOOL bEnable = TRUE);

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	void ShowCheckbox(bool bShow)
	{
		ShowButton(bShow, CHECKBOX);
	}
	void ShowRadio(bool bShow)
	{
		ShowButton(bShow, RADIO_BUTTON);
	}

	void ShowButton(bool bShow = true, int type = CHECKBOX)
	{

		m_bShowButton = bShow;
		m_buttonType = type;
		if (GetSafeHwnd() != NULL)
			SetWindowText(m_bShowButton ? _T("") : m_caption);
		if (m_checkbox.GetSafeHwnd() != NULL)
			m_checkbox.ShowWindow(m_bShowButton&&m_buttonType == CHECKBOX ? SW_SHOW : SW_HIDE);
		//		if( m_radio.GetSafeHwnd() != NULL)
		//			m_radio.ShowWindow(m_bShowButton&&m_buttonType==RADIO_BUTTON?SW_SHOW:SW_HIDE);
	}

protected:

	virtual void PreSubclassWindow();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam);
	static BOOL CALLBACK EnableChild(HWND hwndChild, LPARAM lParam);
	static BOOL CALLBACK EnableParentChild(HWND hwndChild, LPARAM lParam);
	void UpdateChild(BOOL check);


	bool m_bShowButton;
	int m_buttonType;


	CString m_caption;
	CButton m_checkbox;
	//CButton m_radio;
	CRect    m_rcStatic;
	CArray <int, int> m_ItemID;
	CArray <int, int> m_IDList;

	DECLARE_MESSAGE_MAP()

	CBrush m_br;


};

//*****************************************************************************************************
//

interface CStdGriInterface
{
public:

	virtual std::string get_string() = 0;
	virtual void set_string(std::string str) = 0;
};

class CStdGridProperty : public CStdGriInterface, public CMFCPropertyGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:
	
	CStdGridProperty(const std::string& groupName, size_t no=-1, bool bIsValueList=false) :
		CMFCPropertyGridProperty(CString(groupName.c_str()), (DWORD)no, bIsValueList)
	{
	}

	CStdGridProperty(const std::string& name, COleVariant value, const std::string& description, size_t no) :
		CMFCPropertyGridProperty(CString(name.c_str()), value, CString(description.c_str()), (DWORD_PTR)no)
	{
	}

	CStdGridProperty(const std::string& name, const std::string& value, const std::string& description, size_t no) :
		CMFCPropertyGridProperty(CString(name.c_str()), CString(value.c_str()), CString(description.c_str()), (DWORD_PTR)no)
	{
	}

	template <typename T>
	CStdGridProperty(const std::string& name, const T& value, const std::string& description, size_t no) :
		CMFCPropertyGridProperty(CString(name.c_str()), CString(WBSF::ToString(value).c_str()), CString(description.c_str()), (DWORD_PTR)no)
	{
	}


	template <typename T>
	T to_object()const
	{
		string str = get_string();
		return to_object(str);
	}

	virtual std::string get_string(){return std::string((LPCSTR)CStringA(CString(FormatProperty())));}
	virtual void set_string(std::string str){SetValue(CString(str.c_str()));}


};


class CBoolGridProperty : public CStdGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:

	CBoolGridProperty(const std::string& name, const std::string& value, const std::string& description, size_t no) :
		CStdGridProperty(name, COleVariant(WBSF::ToShort(value), VT_BOOL), description, no)
	{}

	CBoolGridProperty(const std::string& name, bool bValue, const std::string& description, size_t no) :
		CStdGridProperty(name, COleVariant((short)bValue, VT_BOOL), description, no)
	{}

	virtual std::string get_string()
	{
		return m_varValue.boolVal == VARIANT_TRUE ? "1" : "0";
	}

	virtual void set_string(std::string str)
	{
		bool bValue = str != "0";
		SetValue(COleVariant(bValue ? VARIANT_TRUE : VARIANT_FALSE, VT_BOOL));
	}
};



template<UINT RES_STRING_ID, int BASE_INDEX = 0, bool ADD_EMPTY = false>
class CIndexProperty : public CStdGridProperty
{
public:

	CIndexProperty(const std::string& name, const std::string& strIndex, const std::string& description, size_t dwData) :
		CStdGridProperty(name, "", description, dwData)
	{
		CStringArrayEx OPTIONS_VALUES(UtilWin::GetCString(RES_STRING_ID));
		
		if (ADD_EMPTY)
			AddOption(_T(""));
			

		for (int i = 0; i < OPTIONS_VALUES.GetSize(); i++)
			AddOption(OPTIONS_VALUES[i]);
		
		AllowEdit(FALSE);

		int index = ToInt(strIndex);
		CString strValue = GetOptionText(int(index) - BASE_INDEX);
		SetValue(strValue);
		SetOriginalValue(strValue);
	}

	CIndexProperty(const std::string& strName, size_t index, const std::string& description, size_t dwData) :
		CStdGridProperty(strName, "", description, dwData)
	{
		CStringArrayEx OPTIONS_VALUES(UtilWin::GetCString(RES_STRING_ID));

		if (ADD_EMPTY)
			AddOption(_T(""));
			

		for (int i = 0; i < OPTIONS_VALUES.GetSize(); i++)
			AddOption(OPTIONS_VALUES[i]);

		AllowEdit(FALSE);

		CString strValue = GetOptionText(int(index) - BASE_INDEX);
		SetValue(strValue);
		SetOriginalValue(strValue);
	}

	CString GetOptionText(int index)
	{
		ASSERT(index >= 0 && index < m_lstOptions.GetSize());
		POSITION pos = m_lstOptions.FindIndex(index);
		return m_lstOptions.GetAt(pos);
	}

	int GetIndex()const
	{
		int index = m_pWndCombo->GetCurSel();
		return index + BASE_INDEX;
	}

	void SetIndex(int index)
	{
		CMFCPropertyGridProperty::SetValue(GetOptionText(index - BASE_INDEX));
	}

	virtual std::string get_string()
	{
		return WBSF::ToString(GetIndex());
	}

	virtual void set_string(std::string str)
	{
		int index = WBSF::ToInt(str);
		if (index >= 0 && index < m_lstOptions.GetSize())
			SetIndex(index);
	}
};


class CStdBrowseProperty : public CStdGridProperty
{
public:

	CStdBrowseProperty(const std::string& strName, const std::string& value, const std::string& description, const std::string& options, size_t dwData) :
		CStdGridProperty(strName, value, description, dwData)
	{
		CStringArrayEx OPTIONS_VALUES(CString(options.c_str()));

		for (INT_PTR i = 0; i < OPTIONS_VALUES.GetSize(); i++)
			AddOption(OPTIONS_VALUES[i]);

		SetOriginalValue(CString(value.c_str()));
	}

	virtual BOOL HasButton() const{ return TRUE; }
	virtual void OnClickButton(CPoint point)
	{
		AfxMessageBox(_T("a faire"));
	}
};

class CStdComboStringProperty : public CStdGridProperty
{
public:

	CStdComboStringProperty(const std::string& strName, const std::string& value, const std::string& description, const std::string& options, size_t dwData) :
		CStdGridProperty(strName, value, description, dwData)
	{
		CStringArrayEx OPTIONS_VALUES(CString(options.c_str()));

		m_bAddEmpty = options.empty() ? true : options[0]=='|';
		if (m_bAddEmpty)
			AddOption(_T(""));

		for (INT_PTR i = 0; i < OPTIONS_VALUES.GetSize(); i++)
			AddOption(OPTIONS_VALUES[i]);

		AllowEdit(FALSE);

		SetOriginalValue(CString(value.c_str()));
	}

	bool m_bAddEmpty;
};

class CStdComboPosProperty : public CStdComboStringProperty
{

public:

	CStdComboPosProperty(const std::string& strName, const std::string& value, const std::string& description, const std::string& options, size_t dwData) :
		CStdComboStringProperty(strName, "", description, options, dwData)
	{
		int index = WBSF::ToInt(value);
		CString strValue = GetOptionText(index);
		SetValue(strValue);
		SetOriginalValue(strValue);
	}


	CStdComboPosProperty(const std::string& strName, size_t index, const std::string& description, const std::string& options, size_t dwData) :
		CStdComboStringProperty(strName, "", description, options, dwData)
	{
		SetValue(GetOptionText(int(index)));
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



class CStdGriFilepathProperty : public CStdGriInterface, public CMFCPropertyGridFileProperty
{
	friend class CMFCPropertyGridCtrl;

public:


	CStdGriFilepathProperty(const std::string& name, const std::string& fileName, const std::string& description, const std::string& filter, size_t no,
		bool bOpen = TRUE, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT) :
		CMFCPropertyGridFileProperty(CString(name.c_str()), bOpen, CString(fileName.c_str()), NULL, dwFlags, CString(filter.c_str()), CString(description.c_str()), (DWORD_PTR)no)
	{}

	virtual std::string get_string(){ return std::string(CStringA(GetValue())); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

};


class CStdGriFolderProperty : public CStdGriInterface, public CMFCPropertyGridFileProperty
{
	friend class CMFCPropertyGridCtrl;

public:

	CStdGriFolderProperty(const std::string& name, const std::string& folderName, const std::string& description, const std::string& filter, size_t no) :
		CMFCPropertyGridFileProperty(CString(name.c_str()), CString(folderName.c_str()), (DWORD_PTR)no, CString(description.c_str()))
	{}

	virtual std::string get_string(){ return std::string(CStringA(GetValue())); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }
	
};




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

	virtual void OnClickButton(CPoint point);


	std::string m_filter;
};



class CStdTRefProperty : public CStdGriInterface, public CMFCPropertyGridDateTimeProperty
{
	DECLARE_DYNAMIC(CStdTRefProperty)


public:

	static COleDateTime GetOleDatTime(std::string str)
	{
		COleDateTime v;

		WBSF::CTRef TRef;
		TRef.FromFormatedString(str, "%Y-%m-%d");
		if (TRef.IsInit())
		{
			TRef.Transform(WBSF::CTM(WBSF::CTM::HOURLY));
			v = COleDateTime(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0);
		}

		return v;
	}

	
	CStdTRefProperty(const std::string& name, const std::string& value, const std::string& description, const std::string& options, size_t no) :
		CMFCPropertyGridDateTimeProperty(CString(name.c_str()), GetOleDatTime(value), CString(description.c_str()), (DWORD)no)
	{
	}

	virtual std::string get_string()
	{
		COleDateTime v = GetValue();
		WBSF::CTRef TRef(v.GetYear(), v.GetMonth() - 1, v.GetDay() - 1);

		return TRef.GetFormatedString("%Y-%m-%d");
	}

	virtual void set_string(std::string str)
	{
		WBSF::CTRef TRef;
		TRef.FromFormatedString(str, "%Y-%m-%d");
		if (TRef.IsInit())
		{
			TRef.Transform(WBSF::CTM(WBSF::CTM::HOURLY));
			COleDateTime v(TRef.GetYear(), int(TRef.GetMonth() + 1), int(TRef.GetDay() + 1), int(TRef.GetHour()), 0, 0);
			SetValue(v);
			SetOriginalValue(v);
		}

	}
};




class CStdGeoRectProperty : public CStdGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:

	CStdGeoRectProperty(const std::string& name, const std::string& value, const std::string& description, size_t no);

};


class CStdPasswordProperty : public CStdGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:

	CStdPasswordProperty(const std::string& name, const std::string& value, const std::string& description, size_t no) :
		CStdGridProperty(name, value, description, no)
	{
	}

	virtual CWnd* CreateInPlaceEdit(CRect rectEdit, BOOL& bDefaultFormat)
	{
		CEdit* pWndEdit = new CEdit;

		DWORD dwStyle = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_PASSWORD;

		if (!m_bEnabled || !m_bAllowEdit)
			dwStyle |= ES_READONLY;

		pWndEdit->Create(dwStyle, rectEdit, m_pWndList, AFX_PROPLIST_ID_INPLACE);
		pWndEdit->SetPasswordChar(_T('*'));
		bDefaultFormat = TRUE;

		return pWndEdit;
	}

	virtual std::string get_string(){ return std::string((LPCSTR)CStringA(CString(GetValue()))); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

	virtual CString FormatProperty()
	{
		CString value = GetValue();
		for (int i = 0; i < value.GetLength(); i++)
			value.SetAt(i, '*');

		return value;
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

class CStdReadOnlyProperty : public CStdGridProperty
{
	friend class CMFCPropertyGridCtrl;

public:
	
	CStdReadOnlyProperty(const std::string& name, const std::string& value) :
		CStdGridProperty(name, value, "", -1)
	{
		

		m_bAllowEdit = false;
	}
};

class CStdPossibleValues : public boost::dynamic_bitset<size_t>
{
public:

	CStdPossibleValues(const std::string& possibleValues, const char* sep = "|;") :
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
			WBSF::StringVector selection(str, sep);
			for (WBSF::StringVector::const_iterator it = selection.begin(); it != selection.end(); it++)
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

		for (size_type i = std::min(size(), str.length()) - 1; i != NOT_INIT; i--)
			set(i, str[str.length() - i - 1] != '0');
	}

protected:

	WBSF::StringVector m_possibleValues;
};


class CStdGriFilepathProperty2 : public CStdGriInterface, public CMFCPropertyGridFileProperty
{
	friend class CMFCPropertyGridCtrl;

public:


	CStdGriFilepathProperty2(const std::string& name, const std::string& fileName, const std::string& description, const std::string& filter, size_t no,
		bool bOpen = TRUE, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING)
		: CMFCPropertyGridFileProperty(CString(name.c_str()), bOpen, CString(fileName.c_str()), NULL, dwFlags, CString(filter.c_str()), CString(description.c_str()), (DWORD_PTR)no)
	{}

	virtual std::string get_string(){ return std::string(CStringA(GetValue())); }
	virtual void set_string(std::string str){ SetValue(CString(str.c_str())); }

};

class CStdBrowseProperty2 : public CStdGridProperty
{
public:

	CStdBrowseProperty2(const std::string& strName, const std::string& value, const std::string& description, const std::string& options, size_t dwData) :
		CStdGridProperty(strName, value, description, dwData)
	{

		if (options.find('=') != NOT_INIT)
		{
			WBSF::StringVector tmp(options, "|=");
			ASSERT(tmp.size() % 2 == 0);
			for (size_t i = 0; i < tmp.size(); i += 2)
			{
				m_possibleValues += i != 0 ? "|" : "";
				m_convertValues += i != 0 ? "|" : "";

				m_possibleValues += tmp[i];
				m_convertValues += tmp[i + 1];
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
		CStdPossibleValues tmp(m_possibleValues);
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

//**************************************************************************************************************************************
//Tab control with icon 24x24
class CMFCTabCtrl24 : public CMFCTabCtrl
{
public:

	DECLARE_DYNCREATE(CMFCTabCtrl24)

	BOOL CMFCTabCtrl24::SetTabHicon(int iTab, HICON /*hIcon*/)
	{
		CWnd* pWnd = GetTabWnd(iTab);
		if (pWnd->GetSafeHwnd() == NULL)
		{
			return FALSE;
		}

		m_sizeImage = CSize(24, 24);
		return CMFCTabCtrl::SetTabHicon(iTab, pWnd->GetIcon(TRUE));
	}
};


//**************************************************************************************************************************************
//CPaneSplitter

class CPaneSplitter : public CSplitterWndEx
{
public:

	BOOL AddWindow(int row, int col, CWnd* pWnd, CString clsName, DWORD dwStyle, DWORD dwStyleEx, SIZE sizeInit)
	{
		m_pColInfo[col].nIdealSize = sizeInit.cx;
		m_pRowInfo[row].nIdealSize = sizeInit.cy;
		CRect rect(CPoint(0, 0), sizeInit);
		if (!pWnd->CreateEx(dwStyleEx, clsName, NULL, dwStyle, rect, this, IdFromRowCol(row, col)))
			return FALSE;
		return TRUE;
	}

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnCheckbox(WPARAM wParam, LPARAM lParam);
};

//**************************************************************************************************************************************
//CPaneSplitter

class CSplittedToolBar : public CMFCToolBar
{
public:

	DECLARE_SERIAL(CSplittedToolBar)
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), bDisableIfNoHndler);
	}
	virtual BOOL LoadState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL SaveState(LPCTSTR lpszProfileName = NULL, int nIndex = -1, UINT uiID = (UINT)-1){ return TRUE; }
	virtual BOOL AllowShowOnList() const { return FALSE; }
	virtual void AdjustLocations();
};

