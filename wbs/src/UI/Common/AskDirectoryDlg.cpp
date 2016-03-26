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
#include "UI/Common/AskDirectoryDlg.h"


// CAskDirectoryDlg dialog

IMPLEMENT_DYNAMIC(CAskDirectoryDlg, CDialog)
CAskDirectoryDlg::CAskDirectoryDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAskDirectoryDlg::IDD, pParent)
{
}

CAskDirectoryDlg::~CAskDirectoryDlg()
{
}

void CAskDirectoryDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAskDirectoryDlg, CDialog)
	ON_LBN_DBLCLK(IDC_CMN_ADD_DIRECTORY_LIST, OnSelectDirectory)
END_MESSAGE_MAP()


// CAskDirectoryDlg message handlers


BOOL CAskDirectoryDlg::OnInitDialog(void)
{
	if( !m_titleDlg.empty() )
		SetWindowText( CString(m_titleDlg.c_str()) );

	for(size_t i=0; i<m_directoryArray.size(); i++)
	{
		GetDirListCtrl().AddString(CString(m_directoryArray[i].c_str()));
	}

	if( !m_directoryArray.empty())
	{
		if( m_selectedDir>= 0 && m_selectedDir < m_directoryArray.size())
			GetDirListCtrl().SetCurSel(m_selectedDir);
		else 
			GetDirListCtrl().SetCurSel(0);
	}
	else
	{
		GetDlgItem(IDOK)->EnableWindow(false);
	}

	
	return TRUE;
}

void CAskDirectoryDlg::OnOK()
{
	m_selectedDir = GetDirListCtrl().GetCurSel();

	ASSERT( m_selectedDir >= 0 && m_selectedDir <m_directoryArray.size());

	CDialog::OnOK();
}

void CAskDirectoryDlg::OnSelectDirectory()
{
	OnOK();
}
