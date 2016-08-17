#pragma once

#include <vector>
#include "Basic/ERMsg.h"
#include "Basic/utilTime.h"
#include "Basic/WeatherStation.h"


namespace WBSF
{

	//************************************************************
	//CMPBCTResult

	class CMPBCTResult
	{
	public:

		CMPBCTResult()
		{
			Reset();
		}

		CMPBCTResult(int year, double Tmin, double Psurv)
		{
			Reset();

			m_year = year;
			m_Tmin = Tmin;
			m_Psurv = Psurv;
		}

		CMPBCTResult(int year, size_t day, double Tmin, double Tmax, double p1, double p3, double Ct, double LT50, double Psurv, double Pmort)
		{
			Reset();

			m_year = year;
			m_day = day;
			m_Tmin = Tmin;
			m_Tmax = Tmax;
			m_p1 = p1;
			m_p3 = p3;
			m_Ct = Ct;
			m_LT50 = LT50;
			m_Psurv = Psurv;
			m_Pmort = Pmort;
		}
		void Reset()
		{
			m_year = -999;
			m_day = -999;
			m_Tmin = m_Tmax = m_p1 = m_p3 = m_Ct = m_LT50 = m_Psurv = m_Pmort = -999;
		}

		//annual result
		int m_year;
		double m_Tmin;
		double m_Psurv;

		//additionnal daily result
		size_t m_day;
		double m_Tmax;
		double m_p1;
		double m_p3;
		double m_Ct;
		double m_LT50;
		double m_Pmort;
	};

	typedef std::vector<CMPBCTResult> CMPBCTResultVector;

	//************************************************************
	//CMPBColdTolerance

	
	class CMPBColdTolerance
	{
	public:

		//input parameter
		bool m_bMicroClimate;


		CMPBColdTolerance();
		virtual ~CMPBColdTolerance();

		void ComputeAnnual(const CWeatherStation& weather);
		void ComputeDaily(const CWeatherStation& weather);

		const CMPBCTResult& operator[](int i)const { return m_result[i]; }
		const CMPBCTResult& operator[](CTRef date)const { return m_result[date - m_firstDate]; }
		//the result is a vector with a size of nbYear
		//the first year is not completly initialised
		const CMPBCTResultVector& GetResult()const{ return m_result; }
	private:

		double m_RhoG;
		double m_MuG;
		double m_SigmaG;
		double m_KappaG;
		double m_RhoL;
		double m_MuL;
		double m_SigmaL;
		double m_KappaL;
		double m_Lambda0;
		double m_Lambda1;


		//output
		CTRef m_firstDate;
		CMPBCTResultVector m_result;
	};
}