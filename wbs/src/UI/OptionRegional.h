//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include "Basic/UtilTime.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/UGEditCtrl.h"

#include "WeatherBasedSimulationUI.h"

namespace WBSF
{

	class CTimeFormatGridCtrl : public CUGEditCtrl
	{
	public:

		virtual void OnSetup();


		void GetFormat(WBSF::CTRefFormat& format);
		void SetFormat(const WBSF::CTRefFormat& format);


	};

	/////////////////////////////////////////////////////////////////////////////
	// COptionRegional dialog

	class COptionRegional : public CMFCPropertyPage
	{


		// Construction
	public:
		COptionRegional();
		~COptionRegional();

		bool OnFinish();
		// Dialog Data
		enum { IDD = IDD_CMN_OPTION_TIME_FORMAT };

		CString m_listDelimiter;
		CString m_decimalDelimiter;
		CTimeFormatGridCtrl m_formatCtrl;

	protected:

		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		virtual BOOL OnInitDialog();
		virtual void OnOK();


		// Generated message map functions
		DECLARE_MESSAGE_MAP()
		afx_msg bool OnBrowse(CEdit& editBox, CString& fileName);

		CFont m_font;
		CEdit m_listDelimiterCtrl;
		CEdit m_decimalDelimiterCtrl;

	};

}