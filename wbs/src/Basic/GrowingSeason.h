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

		enum TDirection { GET_FIRST, GET_LAST, NB_DIRECTIONS };
		enum TTemperature { TT_TMIN, TT_TMEAN, TT_TMAX, TT_TNOON, NB_TT_TEMPERATURE };
		


		TDirection		m_d;
		TTemperature	m_TT;
		char	m_op;
		double	m_threshold;
		double	m_nbDays;
		


		CGSInfo(TDirection d= GET_LAST, TTemperature TT = TT_TMIN, char op = '<', double threshold = 0, double nbDays = 1);
		double GetGST(const CWeatherDay& data)const;
		double GetGST(const CHourlyData& data)const;

		CTRef GetFirst(const CWeatherYear& weather, size_t first_month, size_t last_month, int shift)const;
		CTRef GetLast(const CWeatherYear& weather, size_t first_month, size_t last_month, int shift)const;

		CTRef GetEvent(const CWeatherYear& weather, size_t first_month, size_t last_month, int shift)const
		{
			return (m_d == GET_FIRST) ? GetFirst(weather, first_month, last_month, shift) : GetLast(weather, first_month, last_month, shift);
		}
	};

	enum {O_GS_BEGIN, O_GS_END, O_GS_LENGTH, O_GS_NB_OUTPUTS };
	class CEventPeriod
	{
	public:

		CGSInfo		m_begin;
		CGSInfo		m_end;

		//General method
		void Execute(const CWeatherStation& station, CModelStatVector& output)const;
		void Transform(const CTTransformation& TT, const CModelStatVector& input, CTStatMatrix& output);


		virtual CTPeriod GetPeriod(const CWeatherYear& weather)const;
	};

	class CGrowingSeason: public CEventPeriod
	{
	public:

		static const CGSInfo DEFAULT_BEGIN;
		static const CGSInfo DEFAULT_END;
		
		CGrowingSeason(const CGSInfo& begin = DEFAULT_BEGIN, const CGSInfo& end = DEFAULT_END)
		{
			m_begin = begin;
			m_end = end;
		}
	};


	class CFrostFreePeriod : public CEventPeriod
	{
	public:

		static const CGSInfo DEFAULT_BEGIN;
		static const CGSInfo DEFAULT_END;


		CFrostFreePeriod(const CGSInfo& begin = DEFAULT_BEGIN, const CGSInfo& end = DEFAULT_END)
		{
			m_begin = begin;
			m_end = end;
		}

	};



}