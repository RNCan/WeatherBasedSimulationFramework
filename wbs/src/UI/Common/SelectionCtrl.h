//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "UI/Common/HtmlTree/XHtmlTree.h"


class CSelectionCtrl : public CXHtmlTree
{
public:

	CSelectionCtrl(const std::string& in="");
	virtual ~CSelectionCtrl();

	void SetPossibleValues(const std::string& in);
	const std::string& GetPossibleValues()const { return m_possibleValues; }
	bool IsInit(){ return !m_possibleValues.empty(); }

	void SetSelection(const std::string& selection){ m_selection = selection; UpdateCheck(FALSE);  }
	const std::string& GetSelection() {UpdateCheck(TRUE); return m_selection; }

protected:

	virtual void Init();
	virtual void PreSubclassWindow();
	void UpdateCheck(bool bSaveAndValidate);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	std::string m_possibleValues;
	std::string m_selection;
};

void AFXAPI DDX_Selection(CDataExchange* pDX, int ID, std::string& selection);



class CSelectionDlg : public CDialog
{
	// Construction
public:

	CSelectionDlg(CWnd* pParent);   // standard constructor
	virtual ~CSelectionDlg();


	std::string m_possibleValues;
	std::string m_selection;

	
protected:

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnDestroy();
	

	CSelectionCtrl m_selectionCtrl;
	


};

