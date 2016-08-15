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
#include "OptionAdvanced.h"
#include "Basic/Registry.h"
#include "Basic/OpenMP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionAdvanced property page 
namespace WBSF
{

	COptionAdvanced::COptionAdvanced() : CMFCPropertyPage(COptionAdvanced::IDD)
	{
		WBSF::CRegistry option("ExecuteCtrl");

		m_maxDistFromLOC = option.GetProfileInt("MaxDistFromLOC", 300);
		m_maxDistFromPoint = option.GetProfileInt("MaxDistFromPoint", 500);
		m_bRunEvenFar = option.GetProfileInt("RunEvenFar", FALSE);
		m_bRunWithMissingYear = option.GetProfileInt("RunWithMissingYear", FALSE);
		m_bKeepTmpFile = option.GetProfileInt("KeepTmpOutputFile", FALSE);
		m_nbMaxThreads = option.GetProfileInt("NbMaxThreads", omp_get_num_procs());
		m_bUseHxGrid = option.GetProfileInt("UseHxGrid", FALSE);

	}

	COptionAdvanced::~COptionAdvanced()
	{
	}

	void COptionAdvanced::DoDataExchange(CDataExchange* pDX)
	{


		CMFCPropertyPage::DoDataExchange(pDX);
		DDX_Text(pDX, IDC_CMN_OPTION_MAX_DISTANCE1, m_maxDistFromLOC);
		DDX_Text(pDX, IDC_CMN_OPTION_MAX_DISTANCE2, m_maxDistFromPoint);
		DDX_Check(pDX, IDC_CMN_OPTION_RUN_EVEN_FAR, m_bRunEvenFar);
		DDX_Check(pDX, IDC_CMN_OPTION_RUN_MISSING_YEAR, m_bRunWithMissingYear);
		DDX_Check(pDX, IDC_CMN_OPTION_REMOVE_TMP_FILE, m_bKeepTmpFile);
		DDX_Text(pDX, IDC_CMN_OPTION_MAX_THREADS, m_nbMaxThreads);
		DDX_Check(pDX, IDC_CMN_OPTION_HXGRID, m_bUseHxGrid);
	}


	BEGIN_MESSAGE_MAP(COptionAdvanced, CMFCPropertyPage)
	END_MESSAGE_MAP()

	/////////////////////////////////////////////////////////////////////////////
	// COptionAdvanced message handlers




	void COptionAdvanced::OnOK()
	{
		//if page is init
		if (GetDlgItem(IDC_CMN_OPTION_MAX_DISTANCE1)->GetSafeHwnd() != NULL)
		{
			WBSF::CRegistry option("ExecuteCtrl");
			option.WriteProfileInt("MaxDistFromLOC", m_maxDistFromLOC);
			option.WriteProfileInt("MaxDistFromPoint", m_maxDistFromPoint);
			option.WriteProfileInt("RunEvenFar", m_bRunEvenFar);
			option.WriteProfileInt("RunWithMissingYear", m_bRunWithMissingYear);
			option.WriteProfileInt("KeepTmpOutputFile", m_bKeepTmpFile);
			option.WriteProfileInt("NbMaxThreads", m_nbMaxThreads);
			option.WriteProfileInt("UseHxGrid", m_bUseHxGrid);
		}

		CMFCPropertyPage::OnOK();
	}
}