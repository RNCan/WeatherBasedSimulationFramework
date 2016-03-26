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



namespace WBSF
{

	class CTemporalRefCtrl : public CWnd
	{
	public:
		CTemporalRefCtrl(void);
		~CTemporalRefCtrl(void);

		void SetType(short type){ m_type = type; }
		void SetTemporalreference(CTRef& ref){ m_ref = ref; }
		void SetPeriod(const CTPeriod& period){ m_period = period; }

	protected:

		void CreateControls();
		DECLARE_MESSAGE_MAP()

		short m_type;
		CTRef m_ref;
		CTPeriod    m_period;

	public:
		afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	};

}