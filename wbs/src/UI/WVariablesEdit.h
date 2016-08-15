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



#include "Basic/UtilStd.h"
#include "Basic/WeatherDefine.h"
#include "UI/Common/SelectionCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{


	class CModel;


	class CWVariablesDlg : public CDialog
	{
		// Construction
	public:


		bool m_bShowFromModel;
		WBSF::StringVector m_models;

		CWVariablesDlg(CWnd* pParent);   // standard constructor
		virtual ~CWVariablesDlg();


		// Dialog Data
		enum { IDD = IDD_VARIABLES };
		CWVariables m_variables;

		// Overrides
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Implementation
	protected:

		// Generated message map functions
		virtual BOOL OnInitDialog();
		virtual void OnOK();

		DECLARE_MESSAGE_MAP()
		void OnSelectFromModel();
		void SetSelection();

		CSelectionCtrl m_variablesCtrl;
		CMenu m_menu;
	public:

		CMFCMenuButton m_fromModelCtrl;



	};



	class CWVariablesEdit : public CMFCEditBrowseCtrl
	{
		// Construction
	public:

		bool m_bShowFromModel;
		WBSF::StringVector m_models;


		CWVariablesEdit()
		{
			m_bShowFromModel = false;
		}

		virtual ~CWVariablesEdit() {};

		BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

		// --- In  :	filter			-	filter to be associated with the control
		// --- Out :	
		// --- Returns:	
		// --- Effect : Associates the control with the filter
		void SetVariables(CWVariables filter);

		// --- In  :	
		// --- Out :	
		// --- Returns:	The filter associated with the control
		// --- Effect : Retreieves the filter associated with the control
		CWVariables GetVariables()const;



	protected:

		// called every time new filter is set
		virtual void OnSetFilter(CWVariables filter);
		virtual void OnBrowse();


		DECLARE_MESSAGE_MAP()

	};


	class CMFCToolBarWVariablesEditCtrl : public CMFCToolBarEditCtrl
	{
		// Construction
	public:

		bool m_bShowFromModel;
		WBSF::StringVector m_models;

		CMFCToolBarWVariablesEditCtrl(CMFCToolBarEditBoxButton& edit);
		virtual ~CMFCToolBarWVariablesEditCtrl();


		BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

		// --- In  :	filter			-	filter to be associated with the control
		// --- Out :	
		// --- Returns:	
		// --- Effect : Associates the control with the filter
		void SetVariables(CWVariables filter);

		// --- In  :	
		// --- Out :	
		// --- Returns:	The filter associated with the control
		// --- Effect : Retreieves the filter associated with the control
		CWVariables GetVariables()const;


		// Implementation

		virtual void OnSetFilter(CWVariables filter);
		virtual void OnBrowse();

		DECLARE_MESSAGE_MAP()
	};

	// The following function was introduced in order to be specifically used in 
	// DoDataExchange function of any Dialog or FormView based application for 
	// Browse Color Edit controls. 
	void AFXAPI DDX_WVariables(CDataExchange *pDX, int nIDC, CWVariables& filter);
	//override DDX_Control to provide initialization
	void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, CWVariablesEdit& rControl);



	class CMFCToolBarWVariablesButton : public CMFCToolBarEditBoxButton
	{
	public:

		DECLARE_SERIAL(CMFCToolBarWVariablesButton)

		CMFCToolBarWVariablesButton()
		{}

		CMFCToolBarWVariablesButton(UINT uiID, UINT uimageID, int iWidth = 0) : CMFCToolBarEditBoxButton(uiID, uimageID, ES_AUTOHSCROLL | ES_WANTRETURN | WS_TABSTOP, iWidth)
		{
		}

		virtual CEdit* CreateEdit(CWnd* pWndParent, const CRect& rect)
		{
			CMFCToolBarWVariablesEditCtrl* pEdit = new CMFCToolBarWVariablesEditCtrl(*this);
			pEdit->Create(m_dwStyle, rect, pWndParent, m_nID);

			return pEdit;
		}

		void SetVariables(CWVariables vars)
		{
			if (GetEditBox())
			{
				GetEditBox()->GetWindowText(m_strContents);

				CString newStr(vars.to_string().c_str());
				if (newStr != m_strContents)
				{
					m_strContents = newStr;
					GetEditBox()->SetWindowText(m_strContents);
				}
			}
		}


		CWVariables GetVariables()
		{
			CString tmp;
			if (GetEditBox())
				GetEditBox()->GetWindowText(tmp);

			std::string str = CStringA(tmp);

			return CWVariables(str);
		}

	};

}