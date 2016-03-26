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

#include "FileManager/FileManager.h"
#include "UI/Common/CommonCtrl.h"
#include "UI/Common/SYShowMessage.h"
#include "UI/Common/CustomDDX.h"
#include "UI/Common/UtilWin.h"

#include "WeatherUpdateDlg.h"

using namespace std;



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace WBSF
{


	//*************************************************************************************************
	// CWeatherUpdateDlg dialog

	BEGIN_MESSAGE_MAP(CWeatherUpdateDlg, CDialog)
	END_MESSAGE_MAP()


	CWeatherUpdateDlg::CWeatherUpdateDlg(const CExecutablePtr& pParent, CWnd* pParentWnd) :
		CDialog(CWeatherUpdateDlg::IDD, pParentWnd),
		m_pParent(pParent)
	{
	}



	void CWeatherUpdateDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		if (!pDX->m_bSaveAndValidate)
		{
			WBSF::StringVector list = WBSF::GetFM().WeatherUpdate().GetFilesList();
			CCFLComboBox* pList = (CCFLComboBox*)GetDlgItem(IDC_KIND);
			pList->FillList(list);
			pList->SelectStringExact(0, m_weatherUpdate.m_fileTitle);
		}

		DDX_Text(pDX, IDC_NAME, m_weatherUpdate.m_name);
		DDX_Text(pDX, IDC_DESCRIPTION, m_weatherUpdate.m_description);
		DDX_Text(pDX, IDC_INTERNAL_NAME, m_weatherUpdate.m_internalName);
		DDX_Text(pDX, IDC_KIND, m_weatherUpdate.m_fileTitle);
	}



}