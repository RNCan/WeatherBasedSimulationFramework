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
#include "TransfoDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace UtilWin;

namespace WBSF
{

	/////////////////////////////////////////////////////////////////////////////
	// CTransfoDlg dialog


	CTransfoDlg::CTransfoDlg(CWnd* pParent /*=NULL*/)
		: CDialog(CTransfoDlg::IDD, pParent)
	{

		m_transfoType = -1;
		m_logType = -1;
		m_n = 0;
		m_bUseXPrime = FALSE;
		m_bDataInPercent=FALSE;
	}


	void CTransfoDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);

		DDX_Control(pDX, IDC_TRANSFO_N, m_nCtrl);
		DDX_Control(pDX, IDC_TRANSFO_X_PRIME, m_useXPrimeCtrl);
		DDX_Control(pDX, IDC_TRANSFO_LOGX, m_logXCtrl);
		DDX_Control(pDX, IDC_TRANSFO_LOGX1, m_logX1Ctrl);
		DDX_Control(pDX, IDC_TRANSFO_PERCENT, m_percentCtrl);
		

		DDX_Radio(pDX, IDC_TRANFO_NOTRANSFO, m_transfoType);
		DDX_Radio(pDX, IDC_TRANSFO_LOGX, m_logType);
		DDX_Text(pDX, IDC_TRANSFO_N, m_n);
		DDX_Check(pDX, IDC_TRANSFO_X_PRIME, m_bUseXPrime);
		DDX_Check(pDX, IDC_TRANSFO_PERCENT, m_bDataInPercent);
	

		if (pDX->m_bSaveAndValidate)
		{
	
		}
		else
		{

		}

	}


	BEGIN_MESSAGE_MAP(CTransfoDlg, CDialog)
		//{{AFX_MSG_MAP(CTransfoDlg)
		ON_BN_CLICKED(IDC_TRANFO_NOTRANSFO, UpdateCtrl)
		ON_BN_CLICKED(IDC_TRANSFO_LOG, UpdateCtrl)
		ON_BN_CLICKED(IDC_TRANSFO_LOGIT, UpdateCtrl)
		ON_BN_CLICKED(IDC_TRANSFO_X_PRIME, UpdateCtrl)
		//}}AFX_MSG_MAP
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// CTransfoDlg message handlers


	BOOL CTransfoDlg::OnInitDialog()
	{
		CDialog::OnInitDialog();

		
		UpdateCtrl();

		return TRUE;  // return TRUE unless you set the focus to a control
		// EXCEPTION: OCX Property Pages should return FALSE
	}

	void CTransfoDlg::UpdateCtrl()
	{
		int transfoType = GetCheckedRadioButton(IDC_TRANFO_NOTRANSFO, IDC_TRANSFO_LOGIT);
		bool bEnableLog = transfoType == IDC_TRANSFO_LOG;
		bool bEnableLogit = transfoType == IDC_TRANSFO_LOGIT;

		m_logXCtrl.EnableWindow(bEnableLog);
		m_logX1Ctrl.EnableWindow(bEnableLog);
		for (int i = 1; i <= 7; i++)
			GetStaticID(i).EnableWindow(bEnableLog);

		 for(int i=8; i<=17; i++)
			 GetStaticID(i).EnableWindow(bEnableLogit);

		m_useXPrimeCtrl.EnableWindow(bEnableLogit);
		m_percentCtrl.EnableWindow(bEnableLogit);
		m_nCtrl.EnableWindow(bEnableLogit&&m_useXPrimeCtrl.GetCheck());
	}


}