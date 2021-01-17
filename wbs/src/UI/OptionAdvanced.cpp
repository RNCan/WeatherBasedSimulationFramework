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
		WBSF::CRegistry registry("ExecuteCtrl");

		m_maxDistFromLOC = registry.GetProfileInt("MaxDistFromLOC", 300);
		m_maxDistFromPoint = registry.GetProfileInt("MaxDistFromPoint", 500);
		m_bRunEvenFar = registry.GetProfileInt("RunEvenFar", FALSE);
		m_bRunWithMissingYear = registry.GetProfileInt("RunWithMissingYear", FALSE);
		m_bKeepTmpFile = registry.GetProfileInt("KeepTmpOutputFile", FALSE);
		m_nbMaxThreads = registry.GetProfileInt("NbMaxThreads", omp_get_num_procs());
		m_bUseHxGrid = registry.GetProfileInt("UseHxGrid", FALSE);
		m_nbLagMin  = registry.GetValue("LagMin", 10);
		m_nbLagMax  = registry.GetValue("LagMax", 40);
		m_nbLagStep = registry.GetValue("LagStep", 5);
		m_lagDistMin  = registry.GetValue("LagDistMin", 0.5f);
		m_lagDistMax  = registry.GetValue("LagDistMax", 10.0f);
		m_lagDistStep = registry.GetValue("LagDistStep", 0.5f);
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

		DDX_Text(pDX, IDC_CMN_OPTION_LAG_MIN, m_nbLagMin);
		DDX_Text(pDX, IDC_CMN_OPTION_LAG_MAX, m_nbLagMax);
		DDX_Text(pDX, IDC_CMN_OPTION_LAG_STEP,m_nbLagStep);
		DDX_Text(pDX, IDC_CMN_OPTION_DIST_MIN, m_lagDistMin);
		DDX_Text(pDX, IDC_CMN_OPTION_DIST_MAX, m_lagDistMax);
		DDX_Text(pDX, IDC_CMN_OPTION_DIST_STEP, m_lagDistStep);

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
			WBSF::CRegistry registry("ExecuteCtrl");
			registry.WriteProfileInt("MaxDistFromLOC", m_maxDistFromLOC);
			registry.WriteProfileInt("MaxDistFromPoint", m_maxDistFromPoint);
			registry.WriteProfileInt("RunEvenFar", m_bRunEvenFar);
			registry.WriteProfileInt("RunWithMissingYear", m_bRunWithMissingYear);
			registry.WriteProfileInt("KeepTmpOutputFile", m_bKeepTmpFile);
			registry.WriteProfileInt("NbMaxThreads", m_nbMaxThreads);
			registry.WriteProfileInt("UseHxGrid", m_bUseHxGrid);
			registry.SetValue("LagMin", m_nbLagMin);
			registry.SetValue("LagMax", m_nbLagMax);
			registry.SetValue("LagStep", m_nbLagStep);
			registry.SetValue("LagDistMin", m_lagDistMin);
			registry.SetValue("LagDistMax", m_lagDistMax);
			registry.SetValue("LagDistStep", m_lagDistStep);
		}

		CMFCPropertyPage::OnOK();
	}
}