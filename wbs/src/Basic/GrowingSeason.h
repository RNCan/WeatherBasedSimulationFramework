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
	class CGSInfo
	{
	public:
		
		enum TTemperature{ TT_TMIN, TT_TMEAN, TT_TMAX, TT_TNOON, NB_TT_TEMPERATURE };
		size_t	m_type;
		size_t	m_nbDays;
		double	m_threshold;


		double GetGST(const CWeatherDay& data)const;
		CTRef GetFirst(CWeatherYear& weather, char sign = '>')const;
		CTRef GetLast(CWeatherYear& weather, char sign = '>')const;
	};
	

	class CGrowingSeason
	{
	public:
		
		//enum TAccumulationType{ AT_HOURLY, AT_DAILY };

		CGSInfo		m_begin;
		CGSInfo		m_end;
		//bool		m_bAlwaysFillPeriod;
		//bool bAlwaysFillPeriod = true, 
		CGrowingSeason(size_t TtypeBegin = CGSInfo::TT_TMIN, size_t nbDaysBegin = 3, double threasholdBegin = 0, size_t TtypeEnd = CGSInfo::TT_TMIN, size_t nbDaysEnd = 3, double threasholdEnd = 0)
		{
			//m_bAlwaysFillPeriod = bAlwaysFillPeriod;
			m_begin.m_type = TtypeBegin;
			m_begin.m_nbDays = nbDaysBegin;
			m_begin.m_threshold = threasholdBegin;
			m_end.m_type = TtypeEnd;
			m_end.m_nbDays = nbDaysEnd;
			m_end.m_threshold = threasholdEnd;
		}

		

		//General method
		void Execute(const CWeatherStation& station, CModelStatVector& output)const;
		void Transform(const CTTransformation& TT, const CModelStatVector& input, CTStatMatrix& output);
		

		CTPeriod GetGrowingSeason(const CWeatherYear& weather)const;
		CTPeriod GetFrostFreePeriod(const CWeatherYear& weather)const;
		
	};

}