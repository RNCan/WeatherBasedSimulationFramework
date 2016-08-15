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
#pragma once

#include "StdAfx.h"

#include "ModelBase/Model.h"
#include "FileManager/FileManager.h"
#include "UI/Common/UtilWin.h"
#include "UI/WVariablesEdit.h"


using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	static const UINT BASE_ID = 1000;

	//*********************************************************************************************
	// CWVariablesDlg dialog

	BEGIN_MESSAGE_MAP(CWVariablesDlg, CDialog)
		ON_COMMAND(IDC_FROM_MODEL, OnSelectFromModel)
	END_MESSAGE_MAP()


	CWVariablesDlg::CWVariablesDlg(CWnd* pParent) :
		CDialog(CWVariablesDlg::IDD, pParent)
	{

		//init model comboBox
		m_menu.CreatePopupMenu();

	}

	CWVariablesDlg::~CWVariablesDlg()
	{
		m_menu.DestroyMenu();
	}


	void CWVariablesDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_FROM_MODEL, m_fromModelCtrl);
		DDX_Control(pDX, IDC_VARIABLES, m_variablesCtrl);


		if (pDX->m_bSaveAndValidate)
		{
			m_variables.SetSelection(m_variablesCtrl.GetSelection());
		}
		else
		{

			std::string possibleValues;
			for (size_t v = 0; v < NB_VAR_H; v++)
			{
				if (!possibleValues.empty())
					possibleValues += "|";

				possibleValues += GetVariableTitle(v);
			}

			m_variablesCtrl.SetPossibleValues(possibleValues);
			m_variablesCtrl.SetSelection(m_variables.GetSelection());

			//SetSelection();

			if (m_bShowFromModel)
			{
				for (size_t i = 0; i < m_models.size(); i++)
				{
					CString name(WBSF::GetFileTitle(m_models[i]).c_str());
					m_menu.AppendMenu(MF_STRING | MF_BYCOMMAND, BASE_ID + i, name);
				}

				m_fromModelCtrl.m_hMenu = m_menu.GetSafeHmenu();
				m_fromModelCtrl.SizeToContent();
			}
			else
			{
				CRect rect1;
				m_fromModelCtrl.GetWindowRect(rect1); ScreenToClient(rect1);

				CRect rect2;
				m_variablesCtrl.GetWindowRect(rect2); ScreenToClient(rect2);


				m_fromModelCtrl.ShowWindow(SW_HIDE);
				m_variablesCtrl.SetWindowPos(NULL, rect2.left, rect1.top, rect2.Width(), rect2.bottom - rect1.top, SWP_NOACTIVATE | SWP_NOZORDER);
			}
			//FillModel();
		}


	}




	BOOL CWVariablesDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		return TRUE;  // return TRUE unless you set the focus to a control
	}

	void CWVariablesDlg::OnOK()
	{
		if (!UpdateData())
			return;

		CDialog::OnOK();
	}

	void CWVariablesDlg::SetSelection()
	{
		/*std::string str;
		for (size_t v = 0; v < HOURLY_DATA::NB_VAR_H; v++)
		{
		if (m_variables[v])
		{
		if (!str.empty())
		str += "|";

		str += std::to_string(v);
		}
		}
		*/
		m_variablesCtrl.SetSelection(m_variables.GetSelection());
	}

	void CWVariablesDlg::OnSelectFromModel()
	{
		assert(m_fromModelCtrl.m_nMenuResult - BASE_ID >= 0);

		size_t no = m_fromModelCtrl.m_nMenuResult - BASE_ID;
		ASSERT(no < m_models.size());
		std::string filePath = WBSF::GetFM().Model().GetFilePath(m_models[no]);

		CModel model;
		if (model.Load(filePath))
		{
			m_variables = model.m_variables;
			SetSelection();
		}


	}

	//*********************************************************************************************
	BEGIN_MESSAGE_MAP(CWVariablesEdit, CMFCEditBrowseCtrl)
	END_MESSAGE_MAP()


	BOOL CWVariablesEdit::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		BOOL bRep = CMFCEditBrowseCtrl::Create(dwStyle, rect, pParentWnd, nID);
		EnableBrowseButton();

		return bRep;
	}

	CWVariables CWVariablesEdit::GetVariables() const
	{
		CString text;
		GetWindowText(text);
		std::string str = UtilWin::ToUTF8(text);

		return CWVariables(str);
	}

	void CWVariablesEdit::SetVariables(CWVariables filter)
	{
		OnSetFilter(filter);
	}



	void CWVariablesEdit::OnSetFilter(CWVariables filter)
	{
		if (::IsWindow(GetSafeHwnd()))
		{
			if (filter != GetVariables())
			{
				SetWindowText(CString(filter.to_string().c_str()));
				SendMessage(WM_KEYDOWN, VK_RETURN, 0);
			}
		}
	}



	void CWVariablesEdit::OnBrowse()
	{

		CWVariablesDlg dlg(this);
		dlg.m_bShowFromModel = m_bShowFromModel;
		dlg.m_models = m_models;
		dlg.m_variables = GetVariables();

		if (dlg.DoModal() == IDOK)
		{
			SetVariables(dlg.m_variables);
		}

		SetFocus();
	}


	//*********************************************************************************************
	BEGIN_MESSAGE_MAP(CMFCToolBarWVariablesEditCtrl, CMFCToolBarEditCtrl)
	END_MESSAGE_MAP()


	CMFCToolBarWVariablesEditCtrl::CMFCToolBarWVariablesEditCtrl(CMFCToolBarEditBoxButton& edit) :
		CMFCToolBarEditCtrl(edit)
	{
		m_bShowFromModel = false;
	}

	CMFCToolBarWVariablesEditCtrl::~CMFCToolBarWVariablesEditCtrl()
	{}

	BOOL CMFCToolBarWVariablesEditCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		BOOL bRep = CMFCToolBarEditCtrl::Create(dwStyle, rect, pParentWnd, nID);
		EnableBrowseButton();

		return bRep;
	}

	CWVariables CMFCToolBarWVariablesEditCtrl::GetVariables() const
	{
		CString text;
		GetWindowText(text);
		std::string str = UtilWin::ToUTF8(text);

		return CWVariables(str);
	}

	void CMFCToolBarWVariablesEditCtrl::SetVariables(CWVariables filter)
	{
		OnSetFilter(filter);
	}



	void CMFCToolBarWVariablesEditCtrl::OnSetFilter(CWVariables filter)
	{
		if (::IsWindow(GetSafeHwnd()))
		{
			if (filter != GetVariables())
			{
				SetWindowText(CString(filter.to_string().c_str()));
				PostMessage(WM_KEYDOWN, VK_RETURN, MAKELPARAM(1, NULL));
				PostMessage(WM_KEYUP, VK_RETURN, MAKELPARAM(1, KF_UP)); //actually not necessary, just good practice
			}
		}
	}



	void CMFCToolBarWVariablesEditCtrl::OnBrowse()
	{
		CWVariablesDlg dlg(this);
		dlg.m_bShowFromModel = m_bShowFromModel;
		dlg.m_models = m_models;
		dlg.m_variables = GetVariables();

		if (dlg.DoModal() == IDOK)
		{
			SetVariables(dlg.m_variables);
		}

		SetFocus();
	}

	//*********************************************************************************************

	void AFXAPI DDX_Control(CDataExchange* pDX, int nIDC, CWVariablesEdit& rControl)
	{
		::DDX_Control(pDX, nIDC, (CWnd&)rControl);
		rControl.EnableBrowseButton();
	}

	void AFXAPI DDX_WVariables(CDataExchange *pDX, int nIDC, CWVariables& variables)
	{
		CString tmp(variables.to_string().c_str());
		DDX_Text(pDX, nIDC, tmp);
		variables = CStringA(tmp);
	}

	//*********************************************************************************************
	IMPLEMENT_SERIAL(CMFCToolBarWVariablesButton, CMFCToolBarEditBoxButton, 1)

}