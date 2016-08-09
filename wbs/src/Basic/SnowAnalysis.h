//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************

#pragma once

#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "Basic/Statistic.h"


namespace WBSF
{

	class CSnowAnalysis
	{
	public:
		
		enum TStat{S_LAST_JDAY, S_FIRST_JDAY, NB_STAT};
		static const double MINIMUM_SNOW_DEPTH;
		static const size_t NB_DAY_MIN;

		HOURLY_DATA::TVarH m_variable;
		double m_minimum_snow_depth;
		size_t m_nb_day_min;

		
		CSnowAnalysis(double minimum_snow_depth = MINIMUM_SNOW_DEPTH, size_t nb_day_min = NB_DAY_MIN)
		{
			
			m_variable = HOURLY_DATA::H_SNDH;
			m_minimum_snow_depth = minimum_snow_depth;
			m_nb_day_min = nb_day_min;
		}


		//General method
		void Execute(const CWeatherStation& station, CModelStatVector& output)const;
		void Transform(const CTTransformation& TT, const CModelStatVector& input, CTStatMatrix& output);
		
		CTRef GetLastSnowTRef(const CWeatherYear& weather)const;
		CTRef GetFirstSnowTRef(const CWeatherYear& weather)const;
		
	};

}