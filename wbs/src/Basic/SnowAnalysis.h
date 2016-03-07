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
	//class CGSInfo
	//{
	//public:
	//	
	//	enum TTemperature{ TT_TMIN, TT_TMEAN, TT_TMAX, TT_TNOON, NB_TT_TEMPERATURE };
	//	size_t	m_type;
	//	size_t	m_nbDays;
	//	double	m_threshold;


	//	double GetGST(const CDataInterface& data)const;
	//	CTRef GetFirst(CWeatherYear& weather, char sign = '>')const;
	//	CTRef GetLast(CWeatherYear& weather, char sign = '>')const;
	//};
	//

	class CSnowAnalysis
	{
	public:
		
		enum TStat{S_LAST_JDAY, S_FIRST_JDAY, NB_STAT};
		static const double _MINIMUM_SNOW_DEPTH;
		static const size_t _NB_DAY_MIN;


		double m_minimum_snow_depth;
		size_t m_nb_day_min;

		
		CSnowAnalysis(double minimum_snow_depth = _MINIMUM_SNOW_DEPTH, size_t nb_day_min = _NB_DAY_MIN)
		{
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