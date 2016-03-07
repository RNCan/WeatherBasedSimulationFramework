//******************************************************************************
//  project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
//******************************************************************************

#include "stdafx.h"

#include "Basic/SnowAnalysis.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;

namespace WBSF
{

	const double MINIMUM_SNOW_DEPTH = 2;//mm
	const size_t NB_DAY_MIN = 7;		//day

/*
	double CGSInfo::GetGST(const CDataInterface& data)const
	{
		ASSERT(m_type >= 0 && m_type<NB_TT_TEMPERATURE);

		double T = -999;
		if (data.IsHourly())
		{
			switch (m_type)
			{
			case TT_TMIN:	
			case TT_TMEAN:	
			case TT_TMAX:	ASSERT(false); break;
			case TT_TNOON:  T = data[H_TAIR]; break;
			default: ASSERT(false);
			}
		}
		else
		{
			switch (m_type)
			{
			case TT_TMIN:	T = data[H_TMIN][MEAN]; break;
			case TT_TMEAN:	T = data[H_TAIR][MEAN]; break;
			case TT_TMAX:	T = data[H_TMAX][MEAN]; break;
			case TT_TNOON:  ASSERT(false); break;
			default: ASSERT(false);
			}
		}
		return T;
	}
*/
	void CSnowAnalysis::Execute(const CWeatherStation& weather, CModelStatVector& output)const
	{
		output.Init(weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), 2);

		for (size_t y = 0; y < weather.size(); y++)
		{
			CTRef l = GetLastSnowTRef(weather[y]);
			CTRef f = GetFirstSnowTRef(weather[y]);
			output[y][S_LAST_JDAY] = l.GetRef();
			output[y][S_FIRST_JDAY] = f.GetRef();
		}
	}
	
	

	CTRef CSnowAnalysis::GetLastSnowTRef(const CWeatherYear& weather)const
	{
		CTRef lastTRef;

		int year = weather.GetTRef().GetYear();
		CTM TM = weather.GetTM();

		CTPeriod period = weather.GetEntireTPeriod();
		CTRef midSeason(year, JULY, 14, 12, TM);
		//CTRef TRef(year, JANUARY, FIRST_DAY);

		size_t nbTRefPerDay = weather.IsHourly() ? 24 : 1;
		size_t nbTRef = 0;
		for (CTRef TRef = midSeason; TRef >= period.Begin() && !lastTRef.IsInit(); TRef--)
		{
			if (weather[TRef][H_SNDH][MEAN] > m_minimum_snow_depth)//more than 2 cm
			{
				nbTRef++;
				if (nbTRef>m_nb_day_min*nbTRefPerDay)
				{
					lastTRef = TRef + m_nb_day_min;
				}
			}
			else
			{
				//snow again, reset
				nbTRef = 0;
			}
		}

		return lastTRef;
	}

	CTRef CSnowAnalysis::GetFirstSnowTRef(const CWeatherYear& weather)const
	{
		CTRef firstTRef;
		

		int year = weather.GetTRef().GetYear();
		CTM TM = weather.GetTM();

		CTPeriod period = weather.GetEntireTPeriod();
		CTRef midSeason(year, JULY, 14, 12, TM);

		size_t nbTRefPerDay = weather.IsHourly() ? 24 : 1;
		size_t nbTRef = 0;
		for (CTRef TRef = midSeason; TRef >= period.End() && !firstTRef.IsInit(); TRef++)
		{
			if (weather[TRef][H_SNDH][MEAN] > m_minimum_snow_depth)
			{
				nbTRef++;
				if (nbTRef>m_nb_day_min*nbTRefPerDay)
					firstTRef = TRef - m_nb_day_min;
			}
			else
			{
				nbTRef = 0;
			}
		}

		return firstTRef;
	}


}//namespace WBSF