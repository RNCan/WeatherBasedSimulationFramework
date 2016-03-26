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


#include "Geomatic/PrePostTransfo.h"
#include "UI/Common/CommonCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CTransfoDlg dialog

	class CTransfoDlg : public CDialog
	{
		// Construction
	public:

		CTransfoDlg(CWnd* pParent = NULL);   // standard constructor

		const CPrePostTransfo& GetPrePostTransfo()
		{
			m_prePostTransfo.m_type = m_transfoType;
			m_prePostTransfo.m_logType = m_logType;
			m_prePostTransfo.m_bUseXPrime = m_bUseXPrime != 0;
			m_prePostTransfo.m_bDataInPercent = m_bDataInPercent!=0;
			m_prePostTransfo.m_n = m_n;

			return m_prePostTransfo;
		}

		void SetPrePostTransfo(const CPrePostTransfo& prePostTransfo)
		{
			m_prePostTransfo = prePostTransfo;

			m_transfoType = (int)m_prePostTransfo.m_type;
			m_logType = (int)m_prePostTransfo.m_logType;
			m_bUseXPrime = m_prePostTransfo.m_bUseXPrime;
			m_bDataInPercent = m_prePostTransfo.m_bDataInPercent;
			m_n = (int)m_prePostTransfo.m_n;

		}

		// Overrides
	protected:

		// Dialog Data
		enum { IDD = IDD_MAP_PREPOST };
		CEdit	m_nCtrl;
		CButton	m_useXPrimeCtrl;
		CButton	m_percentCtrl;
		
		CButton	m_logXCtrl;
		CButton	m_logX1Ctrl;


		int		m_transfoType;
		int		m_logType;
		int		m_n;
		BOOL	m_bUseXPrime;
		BOOL	m_bDataInPercent;


		
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();


		// Implementation
	protected:

		// Generated message map functions



		DECLARE_MESSAGE_MAP()
		afx_msg void UpdateCtrl();


		CWnd& CTransfoDlg::GetStaticID(int i)
		{
			CWnd* pWnd = GetDlgItem(IDC_CMN_STATIC1+i-1);
			ASSERT(pWnd && pWnd->GetSafeHwnd());
			return *pWnd;
		}
	

		CPrePostTransfo m_prePostTransfo;
	};

	
}