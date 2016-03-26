//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


#include "UI/Common/StaticBitmap.h"
#include "UI/Common/HTMLTree/XHyperLink.h"
#include "WeatherBasedSimulationUI.h"

namespace UtilWin
{
	// CAboutDlg dialog used for App About

	class CAboutDlg : public CDialog
	{
	public:


		CString m_lastBuild;
		CString m_extra;


		CAboutDlg(UINT headerID = AFX_IDS_APP_TITLE);

		// Dialog Data
		enum { IDD = IDD_CMN_ABOUTBOX };
		//CStaticBitmap	m_bitmap;
		CStatic m_versionCtrl;
		CStatic m_extratCtrl;
		CXHyperLink m_ctrlEmailRemi;

	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

		// Implementation
	protected:

		UINT m_headerID;
		DECLARE_MESSAGE_MAP()
	};
}