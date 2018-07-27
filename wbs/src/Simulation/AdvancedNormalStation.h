//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//	
//******************************************************************************
#pragma once

#include "Basic/NormalsStation.h"
#include "Basic/WeatherStation.h"
#include "Basic/statistic.h"


namespace WBSF
{

	typedef std::array<CStatistic, 12> CMonthStatistic;
	typedef std::array < CMonthStatistic, HOURLY_DATA::NB_VAR_H > CANStatisticBase;
	class CANStatistic : public CANStatisticBase
	{
	public:

		CANStatistic()
		{
			Reset();
		}

		void Reset()
		{
			m_year = 0;
			for (size_t v = 0; v < HOURLY_DATA::NB_VAR_H; v++)
				for (size_t m = 0; m < 12; m++)
					(*this)[v][m].clear();

		}

		void SetYear(int year){ m_year = year; }
		int GetYear()const{ return m_year; }

		size_t GetNbDays()const;
		size_t GetNbDaysPerMonthMin(size_t v, size_t m)const;

	protected:


		int m_year;


	};



	typedef std::vector<CANStatistic> CANStatisticVectorBase;
	class CANStatisticVector : public CANStatisticVectorBase
	{
	public:

		void ComputeMonthStatistic(const CWeatherStation& dailyStation);
	};


	class CAdvancedNormalStation : public CNormalsStation
	{
	public:

		enum TKindOfValidation { ONE_VALID, ALL_VALID };

		enum { FULL_MONTH = 0 };

		CAdvancedNormalStation();

		void Reset();
		ERMsg FromDaily(const CWeatherStation& dailyStation, int nbYearMinimum);



		//the last monthStatArray fro the "FromDaily" method
		const CANStatisticVector& GetMonthStatArray()const
		{
			return m_monthStatArray;
		}
		const CANStatistic& GetNormal()const
		{
			return m_monthStat;
		}

		const CANStatistic& GetDailyStat()const
		{
			return m_dailyStat;
		}


		static void SetKindOfValidation(int kind)
		{
			KIND_OF_VALIDATION = kind;
		}
		static int GetNbMissPerMonthMax(size_t v)
		{
			if (v == HOURLY_DATA::H_PRCP || v == HOURLY_DATA::H_SNOW)
				return NB_MISS_PER_MONTH_MAX_PRCP;

			return NB_MISS_PER_MONTH_MAX;
		}
		static void SetNbMissPerMonthMax(size_t v, int nbDay)
		{
			if (v == HOURLY_DATA::H_PRCP || v == HOURLY_DATA::H_SNOW)
				NB_MISS_PER_MONTH_MAX_PRCP = nbDay;
			else 
				NB_MISS_PER_MONTH_MAX = nbDay;
		}

		static ERMsg GetNormalValidity(const CWeatherStation& station, int nbYearMin, bool bValid[HOURLY_DATA::NB_VAR_H], int kindOfValidation);
	private:

		void ComputeTemperature(const CWeatherStation& dailyStation);

		static int NB_MISS_PER_MONTH_MAX;
		static int NB_MISS_PER_MONTH_MAX_PRCP;
		static int KIND_OF_VALIDATION;

		CANStatisticVector m_monthStatArray;

		CANStatistic m_monthStat;
		CANStatistic m_dailyStat;

		double m_Tmin[12];
		double m_Tmax[12];
		double m_minMaxRelation[12];
		double m_sigmaDelta[12];
		double m_sigmaEpsilon[12];
		double m_TCorrelation[12][2][2];


	};

}