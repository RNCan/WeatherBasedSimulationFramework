//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once


#include "Basic/UtilStd.h"
#include "WeatherBasedSimulationUI.h"

// CAskDirectoryDlg dialog

class CAskDirectoryDlg : public CDialog
{
	DECLARE_DYNAMIC(CAskDirectoryDlg)

public:

	CAskDirectoryDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAskDirectoryDlg();

	WBSF::StringVector m_directoryArray;
	int m_selectedDir;

	std::string m_titleDlg;
// Dialog Data
	enum { IDD = IDD_CMN_ASK_DIRECTORY };

protected:
	
	virtual BOOL OnInitDialog(void);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSelectDirectory();

	


	CListBox& GetDirListCtrl()
	{
		ASSERT(GetDlgItem(IDC_CMN_ADD_DIRECTORY_LIST));
		return (CListBox&)(*GetDlgItem(IDC_CMN_ADD_DIRECTORY_LIST));
	}




	
};
