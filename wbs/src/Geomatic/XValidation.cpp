//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************
#include "stdafx.h"
#include <algorithm>

#include "XValidation.h"


using namespace std;
namespace WBSF
{


	CXValidation& CXValidation::operator =(const CXValidation& in)
	{
		if (&in != this)
		{
			CXValElemVector::operator=(in);

			m_interpolMethodName = in.m_interpolMethodName;
			m_R2 = in.m_R2;
			m_intercept = in.m_intercept;
			m_slope = in.m_slope;
			m_S = in.m_S;
			m_t = in.m_t;
			m_MSE = in.m_MSE;
		}

		ASSERT(*this == in);

		return *this;
	}

	bool CXValidation::operator == (const CXValidation& in)const
	{
		bool bEgal = true;

		if (!equal(begin(), end(), in.begin()))bEgal = false;
		if (m_interpolMethodName != in.m_interpolMethodName)bEgal = false;
		if (fabs(m_R2 - in.m_R2) > EPSILON_DATA)bEgal = false;
		if (fabs(m_intercept - in.m_intercept) > EPSILON_DATA)bEgal = false;
		if (fabs(m_slope - in.m_slope) > EPSILON_DATA)bEgal = false;
		if (fabs(m_S - in.m_S) > EPSILON_DATA)bEgal = false;
		if (fabs(m_t - in.m_t) > EPSILON_DATA)bEgal = false;
		if (fabs(m_MSE - in.m_MSE) > EPSILON_DATA)bEgal = false;



		return bEgal;
	}

}