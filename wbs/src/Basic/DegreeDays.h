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

	//*************************************************************
	//CDegreeDays

	class CDegreeDays
	{
	public:

		enum TOuput{ S_DD, NB_OUTPUT };
		static const char HEADER[];
		
		enum TDailyMethod{ DAILY_AVERAGE, DAILY_AVERAGE_ADJUSTED, SINGLE_TRIANGLE, DOUBLE_TRIANGLE, SINGLE_SINE, DOUBLE_SINE, ALLEN_WAVE = DOUBLE_SINE, MODIFIED_ALLEN_WAVE, BASKERVILLE_EMIN, NB_DAILY_METHOD };
		enum THourlyMethod{ BIOSIM_HOURLY, NB_HOURLY_METHOD };
		enum TCutoff{ HORIZONTAL_CUTOFF, INTERMEDIATE_CUTOFF, VERTICAL_CUTOFF, NB_CUTOFF };

		size_t m_method;
		double m_lowerThreshold;
		double m_upperThreshold;
		size_t m_cutoffType;
		COverheat m_overheat;

		
		CDegreeDays( size_t method = DAILY_AVERAGE, double lowerThreshold = 0, double upperThreshold = 999, size_t cutoffType = HORIZONTAL_CUTOFF)
		{
			//m_aType = aType;
			m_method = method;
			m_lowerThreshold = lowerThreshold;
			m_upperThreshold = upperThreshold;
			m_cutoffType = cutoffType;
		}



		//General method
		void Execute(const CWeatherYears& station, CModelStatVector& output);
		void Execute(const CWeatherYear& weather, CModelStatVector& output);
	
		double GetDD(const CWeatherDay& in)const;
		double GetDD(const CWeatherMonth& in, const CTPeriod& p = CTPeriod())const;
		double GetDD(const CWeatherYear& in, const CTPeriod& p = CTPeriod())const;


		//specific method
		double GetAverageDD(const CWeatherDay& in)const;
		double GetAverageAdjustedDD(const CWeatherDay& in)const;
		double GetModifiedAllenWaveDD(const CWeatherDay& in)const;
		double GetTriangleDD(const CWeatherDay& in)const;
		double GetDoubleTriangleDD(const CWeatherDay& in)const;
		double GetSineDD(const CWeatherDay& in)const;
		double GetDoubleSineDD(const CWeatherDay& in)const;
		double GetBaskervilleEminDD(const CWeatherDay& in)const;


		//usefull method
		double GetTmin(const CWeatherDay& in)const { return m_overheat.GetTmin(in); }
		double GetTmax(const CWeatherDay& in)const { return m_overheat.GetTmax(in); }
		double GetTnTx(const CWeatherDay& in)const { return (GetTmin(in) + GetTmax(in)) / 2.0;  }
		
		static double GetModifiedAllenWaveDD(const CWeatherDay& in, double threshLow, double threshHigh);
		static double GetModifiedAllenWaveDD(double tmin1, double tmax, double tmin2, double threshLow, double threshHigh);
		static double GetTriangleVerticalCutoff(double Tmin, double Tmax, double Tl, double Tu);
		static double GetTriangleDD(double Tmin, double Tmax, double Th);
		static double GetTriangleDD(double Tmin, double Tmax, double Tl, double Tu, size_t cutoffType);
		static double GetSineVerticalCutoff(double Tn, double Tx, double Tl, double Tu);
		static double GetSineDD(double Tn, double Tx, double th);
		static double GetSineDD(double Tn, double Tx, double Tlth, double Tuth, size_t cutoffType);

	};



	class CDegreeHours
	{
	public:

		enum TOuput{ S_DH, NB_OUTPUT };
		static const char HEADER[];

		enum TCutoff{ HORIZONTAL_CUTOFF, INTERMEDIATE_CUTOFF, VERTICAL_CUTOFF, NB_CUTOFF };

		bool m_bCumulative;
		double m_lowerThreshold;
		double m_upperThreshold;
		size_t m_cutoffType;
		//COverheat m_overheat;


		CDegreeHours(double lowerThreshold = 0, double upperThreshold = 999, bool bCumulative = true, size_t cutoffType = HORIZONTAL_CUTOFF)
		{
			m_lowerThreshold = lowerThreshold;
			m_upperThreshold = upperThreshold;
			m_bCumulative = bCumulative;
			m_cutoffType = cutoffType;
		}

		//General method
		void Execute(const CWeatherYears& station, CModelStatVector& output);
		double GetDH(const CHourlyData& in)const;
	};

}