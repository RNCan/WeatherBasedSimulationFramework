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
#include "stdafx.h"
#include "UI/Common/AboutDlg.h"
#include "UI/Common/UtilWin.h"


#include "WeatherBasedSimulationString.h"


#ifdef _DEBUG
#define new DEBUG_NEW 
#endif

namespace UtilWin
{
 
	CAboutDlg::CAboutDlg(UINT headerID) : 
		CDialog(CAboutDlg::IDD),
		m_headerID(headerID)
	{
	}

	void CAboutDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
//		DDX_Control(pDX, IDC_CMN_BITMAP, m_bitmap);
		DDX_Control(pDX, IDC_CMN_VERSION, m_versionCtrl);
		DDX_Control(pDX, IDC_CMN_EXTRA, m_extratCtrl);
		DDX_Control(pDX, IDC_CMN_MAILTO_REMI, m_ctrlEmailRemi);
		

		if (!pDX->m_bSaveAndValidate)
		{
			m_ctrlEmailRemi.SetURL(_T("mailto:Remi.Saint-Amant@Canada.ca"));

			CString version = UtilWin::GetVersionString(__wargv[0]) + _T(" (") + m_lastBuild + _T(")");
			m_versionCtrl.SetWindowText(GetCString(IDS_STR_VERSION) + _T(" ") + version);

			CString header = GetCString(m_headerID);
			if (!header.IsEmpty())
			{
				CString tmp;
				GetWindowText(tmp);
				SetWindowText(tmp + header);
			}


			m_extratCtrl.SetWindowText(m_extra);
		}
	}

	BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	END_MESSAGE_MAP()
}
