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


		enum TAccumulationType{ AT_HOURLY, AT_DAILY };
		enum TDailyMethod{ DAILY_AVERAGE, DAILY_AVERAGE_ADJUSTED, SINGLE_TRIANGLE, DOUBLE_TRIANGLE, SINGLE_SINE, DOUBLE_SINE, ALLEN_WAVE = DOUBLE_SINE, MODIFIED_ALLEN_WAVE, NB_DAILY_METHOD };
		enum THourlyMethod{ BIOSIM_HOURLY, NB_HOURLY_METHOD };
		enum TCutoff{ HORIZONTAL_CUTOFF, INTERMEDIATE_CUTOFF, VERTICAL_CUTOFF, NB_CUTOFF };

		size_t m_aType;
		size_t m_method;
		double m_lowerThreshold;
		double m_upperThreshold;
		size_t m_cutoffType;


		CDegreeDays(size_t aType = AT_DAILY, size_t method = DAILY_AVERAGE, double lowerThreshold = 0, double upperThreshold = 999, size_t cutoffType = HORIZONTAL_CUTOFF)
		{
			m_aType = aType;
			m_method = method;
			m_lowerThreshold = lowerThreshold;
			m_upperThreshold = upperThreshold;
			m_cutoffType = cutoffType;
		}



		//General method
		virtual void Execute(CWeatherStation& station, CModelStatVector& output);
		
//		void Transform(const CTM& TM, const CModelStatVector& in, CModelStatVector& out){ Transform(CTTransformation(in.GetTPeriod(), TM), out); }
	//	void Transform(const CTTransformation& TT, const CModelStatVector& in, CModelStatVector& out){ Transform(CTTransformation(in.GetTPeriod(), TM), out); }
		//void Transform(const CTM& TM, const CModelStatVector& in, CTStatMatrix& out){ Transform(CTTransformation(in.GetTPeriod(), TM), out); }
		//void Transform(const CTTransformation& TT, const CModelStatVector& in, CTStatMatrix& out);

		double Get(const CDataInterface& in)const;
		double GetDD(const CHourlyData& in)const;
		double GetDD(const CWeatherDay& in)const;
		double GetDH(const CHourlyData& in)const;
		double GetDH(const CWeatherDay& in)const;


		//specific method
		double GetAverageDD(const CWeatherDay& in)const;
		double GetAverageAdjustedDD(const CWeatherDay& in)const;
		double GetAverageStrictDD(const CWeatherDay& in)const;
		double GetModifiedAllenWaveDD(const CWeatherDay& in)const;
		double GetTriangleDD(const CWeatherDay& in)const;
		double GetDoubleTriangleDD(const CWeatherDay& in)const;
		double GetSineDD(const CWeatherDay& in)const;
		double GetDoubleSineDD(const CWeatherDay& in)const;


		static double GetModifiedAllenWaveDD(const CWeatherDay& in, double threshLow, double threshHigh);
		static double GetTriangleVerticalCutoff(double Tmin, double Tmax, double Tl, double Tu);
		static double GetTriangleDD(double Tmin, double Tmax, double Th);
		static double GetTriangleDD(double Tmin, double Tmax, double Tl, double Tu, size_t cutoffType);
		static double GetSineVerticalCutoff(double Tn, double Tx, double Tl, double Tu);
		static double GetSineDD(double Tn, double Tx, double th);
		static double GetSineDD(double Tn, double Tx, double Tlth, double Tuth, size_t cutoffType);

	};

}