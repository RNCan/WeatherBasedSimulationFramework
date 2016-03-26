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

#include "ModelPages.h"

namespace WBSF
{

	class CModel;

	/////////////////////////////////////////////////////////////////////////////
	// CModelDlg

	class CModelDlg : public CMFCPropertySheet
	{
	public:

		enum TPages{ GENERAL, TGINPUT, SSI, INPUT, OUTPUT, CREDIT_DOCUMENT, NB_PAGES };

		CModelDlg(CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
		virtual ~CModelDlg();
		virtual BOOL OnInitDialog();

		CModel m_model;



	protected:

		CModelGeneralPage	m_generalPage;
		CModelWGInputPage	m_WGInputPage;
		CModelSSIPage		m_SSIPage;
		CModelInputPage		m_inputPage;
		CModelOutputPage	m_outputPage;
		CModelCreditPage	m_creditPage;

		HICON	m_hIcon;

		DECLARE_MESSAGE_MAP()

	};

	/////////////////////////////////////////////////////////////////////////////
}