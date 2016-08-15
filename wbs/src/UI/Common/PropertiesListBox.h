//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


class CVSListBoxMS : public CVSListBox
{
public:

	BOOL SetStandardButtons(UINT uiBtns = AFX_VSLISTBOX_BTN_NEW | AFX_VSLISTBOX_BTN_DELETE | AFX_VSLISTBOX_BTN_UP | AFX_VSLISTBOX_BTN_DOWN);


	virtual CWnd* OnCreateList();
	virtual void OnClickButton(int iButton);


	
	void DeleteAllItems(){ m_pWndList->DeleteAllItems(); }

	virtual void CreateNewItem();
	virtual int GetSelectedCount()const;
	virtual int GetSelItem() const;
	virtual void GetSelection(CArray<int>& selection);
	virtual void SetSelection(const CArray<int>& selection);
	virtual DWORD GetStyle()const;
	virtual DWORD GetStyleEx()const;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual bool CanEditLabel(int i)const;
	virtual void OnAfterRemoveItem();
	virtual void UpdateTitle();

	void SelectAll();
	void UnselectAll();

	BOOL GetCheck(int i){ return m_pWndList->GetCheck(i); }
	void SetCheck(int i, BOOL bCheck){ m_pWndList->SetCheck(i, bCheck); }

	CString GetWindowText()const;
	bool SelectString(const CString& txt);
	
protected:

	void DeleteSelection();
	void MoveSelection(bool bIsUp);
	
	
	bool CanMove(const CArray<int>& selection, bool bIsUp)const;
	void MoveItem( int iSelItem, int iNewSelItem );
	void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void OnBeginLabelEdit(NMHDR *pNMHDR, LRESULT *pResult);
	void OnUpdateTitle(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	static const int nListId = 1;
};


//***********************************************************************
//CPropertiesListBoxBase


class CPropertiesListBoxBase : public CStatic
{
public:

	
	virtual void OnSelectionChanged(){}
	virtual BOOL OnBeforeRemoveItem(int /*iItem*/) { return TRUE; }
	virtual void OnAfterAddItem(int /*iItem*/) {}
	virtual void OnAfterRenameItem(int /*iItem*/) {}
	virtual void OnAfterMoveItemUp(int /*iItem*/) {}
	virtual void OnAfterMoveItemDown(int /*iItem*/) {}
	virtual void OnBrowse() {}

	virtual void OnGetDataFromProperties(int iItem) {}
	virtual void OnSetDataToProperties(int iItem) {}
	virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const{}

	DECLARE_MESSAGE_MAP()

};


class CPListBox : public CVSListBoxMS
{
public:

	CPListBox(CPropertiesListBoxBase* pParent)
	{
		m_pParentList=pParent;
	}

	CPropertiesListBoxBase* m_pParentList;

	virtual void OnSelectionChanged() 
	{
		ENSURE(m_pParentList != NULL);
		if( !m_bNewItem )
			m_pParentList->OnSelectionChanged();
	}
	virtual BOOL OnBeforeRemoveItem(int iItem) 
	{
		ENSURE(m_pParentList != NULL);
		return m_pParentList->OnBeforeRemoveItem(iItem);
	}
	

	virtual void OnAfterAddItem(int iItem) 
	{
		ENSURE(m_pParentList != NULL);
		m_pParentList->OnAfterAddItem(iItem);
	}
	
	
	virtual void OnAfterRenameItem(int iItem) 
	{
		ENSURE(m_pParentList != NULL);
		m_pParentList->OnAfterRenameItem(iItem);
	}
	
	virtual void OnAfterMoveItemUp(int iItem) 
	{
		ENSURE(m_pParentList != NULL);
		m_pParentList->OnAfterMoveItemUp(iItem);

	}
	virtual void OnAfterMoveItemDown(int iItem) 
	{
		ENSURE(m_pParentList != NULL);
		m_pParentList->OnAfterMoveItemDown(iItem);
	}
	virtual void OnBrowse()
	{
		ENSURE(m_pParentList != NULL);
		HWND hwndParentList = m_pParentList->GetSafeHwnd();
		m_pParentList->OnBrowse();

		if (::IsWindow(hwndParentList))
		{
			::SetFocus(hwndParentList);
		}
	}

	DECLARE_MESSAGE_MAP()
};

class CCFLPropertyGridProperty : public CMFCPropertyGridProperty
{
public:
	
	CCFLPropertyGridProperty(const CString& strGroupName, DWORD_PTR dwData = 0, BOOL bIsValueList = FALSE):CMFCPropertyGridProperty(strGroupName, dwData, bIsValueList){}
	CCFLPropertyGridProperty(const CString& strName, const COleVariant& varValue, LPCTSTR lpszDescr = NULL, DWORD_PTR dwData = 0,LPCTSTR lpszEditMask = NULL, LPCTSTR lpszEditTemplate = NULL, LPCTSTR lpszValidChars = NULL):CMFCPropertyGridProperty(strName, varValue, lpszDescr, dwData, lpszEditMask, lpszEditTemplate, lpszValidChars){}


	int GetCurSel()const
	{
		CString text = GetValue();
		int index =-1;
		for(int i=0; i<GetOptionCount(); i++)
		{
			if( text == GetOption(i) )
			{
				index=i;
				break;
			}
		}
		return index;
	}
	
	void SetCurSel(int index)
	{
		ASSERT( index>=0 || index<GetOptionCount() );
		if( index<0 || index>GetOptionCount() )
			return;

		SetValue( GetOption(index) );
	}
};

class CCFLPropertyGridCtrl : public CMFCPropertyGridCtrl
{
public:

	CCFLPropertyGridCtrl(CPropertiesListBoxBase* pParent)
	{
		m_pParentList = pParent;
	}

	CCFLPropertyGridProperty* GetProperty(int nIndex) const
	{
		return dynamic_cast<CCFLPropertyGridProperty*>(CMFCPropertyGridCtrl::GetProperty(nIndex));
	}

	void SetLeftColumnWidth(int cx)
	{
		m_nLeftColumnWidth = cx;
		AdjustLayout();
	}
	
	
	virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;

protected:

	DECLARE_MESSAGE_MAP()

	CPropertiesListBoxBase* m_pParentList;
};

class CPropertiesListBox : public CPropertiesListBoxBase
{
public:

	CPListBox* m_pWndList;
	CCFLPropertyGridCtrl* m_pWndProperties;

	CPropertiesListBox(int listSize=-1 );
	~CPropertiesListBox();

	//size of the left pane
	int m_listSize;
protected:

	virtual void PreSubclassWindow();
	virtual int OnCreate(LPCREATESTRUCT lpCreateStruct);

	virtual void Init();
	//virtual BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
	virtual CWnd* OnCreateList();
	virtual CWnd* OnCreateProperties();


	virtual void OnSelectionChanged();
	virtual BOOL OnBeforeRemoveItem(int iItem);
	virtual void OnAfterAddItem(int iItem);
	virtual void OnAfterRenameItem(int iItem);
	virtual void OnAfterMoveItemUp(int iItem);
	virtual void OnAfterMoveItemDown(int iItem);
	//virtual void OnBrowse();
	virtual void OnGetDataFromProperties(int /*iItem*/){}
	virtual void OnSetDataToProperties(int iItem){}
	//virtual void OnPropertiesChange(int iItem, UINT ctrlID, CMFCPropertyGridProperty* pProp){}
	virtual void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const;


	virtual BOOL PreTranslateMessage(MSG* pMsg);
	

	int m_lastSel;
	
	afx_msg LRESULT OnPropertyChanged(WPARAM,LPARAM);
	DECLARE_MESSAGE_MAP()
};


