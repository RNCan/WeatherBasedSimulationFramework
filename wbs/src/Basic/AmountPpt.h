//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//	CAmountPpt: the amout of precipitation from a beta distribution
//******************************************************************************

#pragma once

#include <crtdbg.h>
#include "Basic/UtilMath.h"


namespace WBSF
{

	class CAmountPpt
	{
	public:

		static const size_t NUMBER_SPCLASS = 20;

		CAmountPpt();
		virtual ~CAmountPpt();

		int GetClass(double Sp)const;
		double GetPprecip(double min, double max, double tot_prec, double Sp);
		double XfromP(double Sp, double p)const
		{
			return m_beta[GetClass(Sp)].XfromP(p);
		}

		const CBetaDistribution& operator[](int index)const
		{
			_ASSERTE(index >= 0 && index < NUMBER_SPCLASS);
			return m_beta[index];
		}

	private:

		CBetaDistribution m_beta[NUMBER_SPCLASS];
	};

}