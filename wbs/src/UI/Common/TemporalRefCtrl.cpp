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
#include "StdAfx.h"
#include "TemporalRefCtrl.h"

namespace WBSF
{

	BEGIN_MESSAGE_MAP(CTemporalRefCtrl, CWnd)

		ON_WM_CREATE()
	END_MESSAGE_MAP()


	CTemporalRefCtrl::CTemporalRefCtrl(void)
	{
	}

	CTemporalRefCtrl::~CTemporalRefCtrl(void)
	{
	}


	int CTemporalRefCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CWnd::OnCreate(lpCreateStruct) == -1)
			return -1;

		if (m_ref.m_type != CTRef::UNKNOWN)
		{
			m_type = m_ref.m_type;
		}

		ASSERT(m_period.IsInside(m_ref));

		CreateControls();

		return 0;
	}

	void CTemporalRefCtrl::CreateControls()
	{
		//on pourrait utiliser un champ formatté...avec ellipsis
		//(yyyy/mm/dd/hh): boutton calandrier
		//(yyyy/mm/dd):    bouton calendrier
		//(yyyy/mm): 
		//(yyyy): 
		//(mm/dd/hh): 
		//(mm/dd): 
		//(mm): 
		//(-): 
		//(reference): 
		/*switch(m_type)
		{
		case ANNUAL:
		case ANNUAL_MONTHLY:
		case ANNUAL_DAILY:
		case ANNUAL_HOURLY:
		case REFERENCE:
		}*/
	}

}