//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once


#include "FileManager/FileManager.h"
#include "Basic/WeatherStation.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CWeaLinkDlg dialog

	class CWeaLinkDlg : public CDialog
	{
		// Construction
	public:
		CWeaLinkDlg(CDailyDatabase& dailyDB, CWnd* pParent = NULL);   // standard constructor

		CStringArray m_packageNameList;
		CStringArray m_unlinkedFile;
		//CDailyStation m_station;
		// Dialog Data
		//{{AFX_DATA(CWeaLinkDlg)
		enum { IDD = IDD_LINKED_WEA };
		// NOTE: the ClassWizard will add data members here
		//}}AFX_DATA


		// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CWeaLinkDlg)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL

		// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CWeaLinkDlg)
		// NOTE: the ClassWizard will add member functions here
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	private:

		inline CListBox& GetLinkedListCtrl();
		inline CListBox& GetUnlinkedListCtrl();
		inline CButton& GetAddCtrl();
		inline CButton& GetRemoveCtrl();

		void MoveString(CListBox& lisboxFrom, CListBox& lisboxTo, int posFrom);
		//	void MoveString( CListBox& lisbox, int oldPos, int newPos );
		void SetSelection(CListBox& list, int* pSel, int nSize, int offset);
		void UpdateCtrl();
		ERMsg ImportWEAFile(const CString filePathIn);

		CDailyDatabase& m_dailyDB;

	public:
		afx_msg void OnAddLink();
		afx_msg void OnRemoveLink();
		afx_msg void OnImport();
	protected:
		virtual void OnOK();
	public:
		virtual BOOL OnInitDialog();


	};


	inline CListBox& CWeaLinkDlg::GetLinkedListCtrl()
	{
		ASSERT(GetDlgItem(IDC_RT_LINKED));
		return (CListBox&)(*GetDlgItem(IDC_RT_LINKED));
	}

	inline CListBox& CWeaLinkDlg::GetUnlinkedListCtrl()
	{
		ASSERT(GetDlgItem(IDC_RT_UNLINKED));
		return (CListBox&)(*GetDlgItem(IDC_RT_UNLINKED));
	}

	inline CButton& CWeaLinkDlg::GetAddCtrl()
	{
		ASSERT(GetDlgItem(IDC_RT_ADD_LINK));
		return (CButton&)(*GetDlgItem(IDC_RT_ADD_LINK));
	}

	inline CButton& CWeaLinkDlg::GetRemoveCtrl()
	{
		ASSERT(GetDlgItem(IDC_RT_REMOVE_LINK));
		return (CButton&)(*GetDlgItem(IDC_RT_REMOVE_LINK));
	}

}