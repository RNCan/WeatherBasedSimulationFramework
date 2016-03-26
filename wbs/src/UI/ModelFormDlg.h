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

#include "WeatherBasedSimulationUI.h"


namespace WBSF
{
	class CModelInputParameterCtrl;
	class CModelInputParameterDef;
	class CModelFormItem;


	class CModelFormDialog : public CDialog
	{
	public:

		enum TMessage { MY_LCLICK = WM_USER+2 };			// left click of the mouse

		CModelFormDialog();
		~CModelFormDialog();

		void SetRect(const CRect& rect);
		const CRect& GetRect();
		void AddItem(CModelInputParameterDef& refInVar);
		void DeleteItem(int nID);
		void DeleteAllItem();

		int GetItemId(CString& string)const;
		void UpdateItem(int nID);
		void SetCurSel(int nID);

		BOOL Create(const RECT& rect, UINT nIDTemplate, CModelInputParameterCtrl* pParentWnd = NULL);


		void OnItemActivation(const CString& name)const;
		void OnUpdateParent(const CString& name);


		// Dialog Data
		//{{AFX_DATA(CModelFormDialog)
		enum { IDD = IDD_MODELS_FORM };
		//}}AFX_DATA


		// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CModelFormDialog)

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL


		// Implementation
	protected:

		// Generated message map functions
		//{{AFX_MSG(CModelFormDialog)
		virtual BOOL OnInitDialog();
		virtual void OnCancel();
		virtual void OnOK();


		afx_msg void OnDestroy();
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()

	private:

		CRect m_rect;
		CModelInputParameterCtrl* m_pParent;
		CArray<CModelFormItem*> m_ItemsList;//CObArray 
	};

}