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

/////////////////////////////////////////////////////////////////////////
// CModelInputDlg window

#include "ModelBase/Model.h"
#include "ModelBase/ModelInput.h"

namespace WBSF
{

	class CModelInputDlg : public CDialog
	{
		// Construction
	public:
		CModelInputDlg();

		// Attributes
	public:

		// Operations
	public:


		void ResetCtrl();
		BOOL Create(const CModelInputParameterDefVector& varArray, const CRect& rect, const CString& modelName = _T(""), CWnd* pParentWnd = NULL, bool bForTest = false);
		BOOL Create(const CModel& model, CWnd* pParentWnd = NULL, bool bForTest = false);

		//void TakeDefaultModelInput();
		void SetModelInput(CModelInput& modelInput, bool bDefault = true);
		void GetModelInput(CModelInput& modelInput);

		virtual void OnOK();
		virtual void OnCancel();

		std::string m_appPath;
		std::string m_projectPath;

		// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CModelInputDlg)
	public:

		//}}AFX_VIRTUAL


		// Implementation
	public:
		virtual ~CModelInputDlg();

		// Generated message map functions
	protected:
		//{{AFX_MSG(CModelInputDlg)
		//afx_msg void OnPaint();
		//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnClose();
		afx_msg void OnDestroy();
		//}}AFX_MSG


		//afx_msg void OnButtonClicked( UINT nID );

		DECLARE_MESSAGE_MAP()
	private:


		UINT GetID(size_t varNo)const{ return int(varNo) + 200; }
		//UINT GetIndex(int nID)const{ return nID - 200; }



		CObArray m_titleCtrlArray;
		CObArray m_varCtrlArray;

		bool m_bDefault;

		CModelInputParameterDefVector m_variables;
		CModelInput m_modelInput;


	};
}