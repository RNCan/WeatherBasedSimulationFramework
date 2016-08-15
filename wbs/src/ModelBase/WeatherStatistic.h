//***************************************************************************
// File:	Statistic.h
//
// Class:	CStatistic,
//			CWeatherStatistic
//			CWeatherMonthlyStatistic, 
//
// Abstract:	
//
// Attributs: 	
//
// Description: 
//
// Notes:        note
//***************************************************************************
// 06/03/2004	Rémi Saint-Amant  Initial Version
//****************************************************************************
#pragma once

#include <crtdbg.h>
#include "basic/Statistic.h"
#error
//
//namespace WBSF
//{
//
//
//	//variables available for statistic
//	//TMIN	: minimum temperature (deg C)
//	//TMEAN	: mean temperature (deg C)
//	//TMAX	: maximum temperature (deg C)
//	//PPT	: precipitation (mm)
//	//SNOW	: snow (mm of water) : not validate
//	//SNOW_PACK	: snow water equivalent (mm of water) : not validated
//	//VPD	: vapor pressure deficit (kPa)
//	//PET	: Priestley-Taylor potential evapotranspiration (mm) : not validated
//	//SRAD	: radiation (MJ/m²)
//
//	enum TVariableNew{ STAT_TMIN, STAT_TMAX, STAT_PRCP, STAT_TDEW, STAT_RELH, STAT_WNDS, STAT_SNOW, STAT_SNDH, STAT_SRAD, STAT_ADD1, STAT_ADD2, STAT_ADD3, STAT_ADD4, STAT_T_MN, STAT_DAYLIGHT_VPD, STAT_VPD, STAT_T_RANGE, NB_STAT_VARIABLE };
//
//
//	//*************************************************************
//	//CWeatherStatistic
//
//	//****************************************************************************
//	// abstract:     Compute statistic.
//	//
//	// Description:  
//	//
//	// Input:       
//	//
//	// Output:
//	//
//	// Note:
//	//****************************************************************************
//	class CWeatherStatistic
//	{
//	public:
//
//		CWeatherStatistic();
//		void Reset();
//
//		CStatistic& operator[](short variable)
//		{
//			_ASSERTE(variable >= 0 && variable < NB_STAT_VARIABLE);
//			return m_variableStat[variable];
//		}
//
//		const CStatistic& operator[](short variable)const
//		{
//			_ASSERTE(variable >= 0 && variable < NB_STAT_VARIABLE);
//			return m_variableStat[variable];
//		}
//
//		void operator+=(const CWeatherStatistic& statistic)
//		{
//			for (short i = 0; i < NB_STAT_VARIABLE; i++)
//			{
//				m_variableStat[i] += statistic[i];
//			}
//		}
//
//	private:
//
//		CStatistic m_variableStat[NB_STAT_VARIABLE];
//	};
//
//
//	/*enum TVariable
//	{
//	STAT_TMIN = CWeatherStatistic::TMIN,
//	STAT_TMAX = CWeatherStatistic::TMAX,
//	STAT_PPT = CWeatherStatistic::PPT,
//	STAT_TMEAN = CWeatherStatistic::TMEAN,
//	STAT_SNOW = CWeatherStatistic::SNOW,
//	STAT_SNOW_PACK = CWeatherStatistic::SNOW_PACK,
//	STAT_VPD = CWeatherStatistic::VPD,
//	STAT_PET = CWeatherStatistic::PET,
//	STAT_SRAD = CWeatherStatistic::SRAD,
//	STAT_RAD = CWeatherStatistic::RAD,
//	NB_STAT_VARIABLE
//	};
//	*/
//	//*************************************************************
//	//CWeatherMonthlyStatistic
//
//	class CWeatherMonthlyStatistic
//	{
//	public:
//
//		CWeatherMonthlyStatistic();
//		void Reset();
//
//		CWeatherStatistic& operator[](short month)
//		{
//			_ASSERTE(month >= 0 && month < 12);
//			return m_monhtlyStat[month];
//		}
//
//		const CWeatherStatistic& operator[](short month)const
//		{
//			_ASSERTE(month >= 0 && month < 12);
//			return m_monhtlyStat[month];
//		}
//
//		void operator+=(const CWeatherMonthlyStatistic& statistic)
//		{
//			for (short m = 0; m < 12; m++)
//			{
//				m_monhtlyStat[m] += statistic[m];
//			}
//		}
//
//
//	private:
//
//		CWeatherStatistic m_monhtlyStat[12];
//	};
//
//}