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
// 15-09-2008	Rémi Saint-Amant	Created from old file
//******************************************************************************
#include "stdafx.h"
#include "Basic/WeatherCorrection.h"

 
namespace WBSF
{
	using namespace GRADIENT;

	//**************************************************************
	//CWeatherCorrection
	CWeatherCorrections::CWeatherCorrections()
	{
		reset();
	}

	CWeatherCorrections::~CWeatherCorrections()
	{
	}


	void CWeatherCorrections::reset()
	{
		m_bXVal = false;
		m_target.clear();
		m_variables.reset();
		m_allowDerivedVariables.set();
	}

	double CWeatherCorrections::GetCorrection(const CLocation& pt, CTRef TRef, size_t v)const
	{
		return (v == HOURLY_DATA::H_PRCP) ? 1 : 0;
	}
	/*double CWeatherCorrections::GetCorrectionB0(const CLocation& pt, CTRef TRef, size_t v)const
	{
		return 0;
	}
	
	double CWeatherCorrections::GetCorrectionB1(const CLocation& pt, CTRef TRef, size_t v)const
	{
		return 1;
	}*/

}