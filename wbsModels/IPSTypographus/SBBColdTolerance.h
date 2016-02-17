#pragma once

#include <vector>
#include "ERMsg/ERMsg.h"
#include "basic/utilTime.h"
#include "basic/ModelStat.h"

namespace WBSF
{
	enum TSBBSuperCoolingPoint{ TMAX, P1, P2, P3, CT, LT50, PMORT, NB_SUPER_COOLING_POINT };

	typedef CModelStatVectorTemplate<NB_SUPER_COOLING_POINT> CSBBSuperCoolingPointStat;
	//************************************************************
	//CMPBCTResult
	/*
	class CMPBCTResult
	{
	public:

	CMPBCTResult()
	{
	Reset();
	}

	CMPBCTResult(short year, double Tmin, double Psurv)
	{
	Reset();

	m_year=year;
	m_Tmin=Tmin;
	m_Psurv=Psurv;
	}

	CMPBCTResult(short year, short day, double Tmin, double Tmax, double p1, double p3, double Ct, double LT50, double Psurv, double Pmort)
	{
	Reset();

	m_year=year;
	m_day=day;
	m_Tmin=Tmin;
	m_Tmax=Tmax;
	m_p1=p1;
	m_p3=p3;
	m_Ct=Ct;
	m_LT50=LT50;
	m_Psurv=Psurv;
	m_Pmort=Pmort;
	}
	void Reset()
	{
	m_year=m_day=-999;
	m_Tmin=m_Tmax=m_p1=m_p3=m_Ct=m_LT50=m_Psurv=m_Pmort=-999;
	}

	//annual result
	short m_year;
	double m_Tmin;
	double m_Psurv;

	//additionnal daily result
	short m_day;
	double m_Tmax;
	double m_p1;
	double m_p3;
	double m_Ct;
	double m_LT50;
	double m_Pmort;
	};

	typedef std::vector<CMPBCTResult> CMPBCTResultVector;
	*/

	//************************************************************
	//CSBBColdTolerance

	class CWeather;
	class CSBBColdTolerance
	{
	public:

		//input parameter
		bool m_bMicroClimate;

		CSBBColdTolerance();
		void ExecuteDaily(const CWeatherStation& weather, CSBBSuperCoolingPointStat& stat);
	
	protected:

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

	};
}