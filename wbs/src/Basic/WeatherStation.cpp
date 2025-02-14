﻿//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
// 24-07-2023	Rémi Saint-Amant	in ComputeHourlyTdew, Kr is alway 12 to alway get the same Tdew with or without SRad
// 11-09-2018	Rémi Saint-Amant	Resolve confusion in unit of vapor pressure.
//									thread safe correction in GetData() if hourly object
//									Correction of bug in generation or hourly prcp from daily
// 22-04-2017	Rémi Saint-Amant	After some test, AllenWave seem to give better result than algo devlopped
// 13-09-2016	Rémi Saint-Amant	Change Tair and Trng by Tmin and Tmax
// 01-01-2016	Rémi Saint-Amant	Include into Weather-based simulation framework
// 18-11-2015	Rémi Saint-Amant	Change in the daily accumulation
// 08-12-2014	Rémi Saint-Amant	Integration of CWeather into CWeatherStation
// 08-04-2013	Rémi Saint-Amant	Initial version from old code
//****************************************************************************


//todo: add the time zone to the computation of hourly value. a delta zone must compute between 
//the time zone of the simulation point and the time zone of the weather station

#include "stdafx.h"
#include <locale>
#include <mutex>

#include "Basic/WeatherStation.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/WeatherCorrection.h"

#include "WeatherBasedSimulationString.h"
#include "OpenMP.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;
using namespace WBSF::WEATHER;
using namespace WBSF::GRADIENT;


enum TAgregation { ACCUMUL_12_00, ACCUMUL_18_00, ACCUMUL_18_18, ACCUMUL_22_22, ACCUMUL_00_00 };
static const size_t DAILY_AGREGATION = ACCUMUL_00_00;


namespace WBSF
{


	//from : http://agsys.cra-cin.it/tools/evapotranspiration/help/Soil_heat_flux.html
	//The soil heat flux, G, is the energy that is utilized in heating the soil. G is positive when the soil is warming and negative when the soil is cooling. 
	// LAI [in]:	crop leaf area index [m2 m-2]
	// G [out]:		soil heat flux energy [MJ m-2 hr-1]
	double G(double Rn, double LAI)
	{

		//G beneath a dense cover of grass does not correlate well with air temperature. 
		//where Rn hourly (MJ m-2 h-1) net radiation, KG=0.4 during daytime (Rn>0), KG=1.6-2.0 during nighttime (Rn<0)

		double Ghr = 0;
		if (LAI >= 0)
		{
			double Kg = (Rn > 0) ? 0.4 : 1.8;
			Ghr = Kg * Rn * exp(-0.5 * LAI);
		}
		else
		{
			//Coefficients cd and cn are daytime and nighttime factors, 
			//respectively (cd=0.1, cn=0.5 for short grass; cd=0.04, cn=0.2 for tall grass).
			//We only use short grass here
			static const double Cd = 0.1;
			static const double Cn = 0.5;
			double C = (Rn > 0) ? Cd : Cn;
			Ghr = C * Rn;
		}

		return Ghr;
	}


	CStatistic GetDailyStat(size_t v, CWeatherDay& weather)
	{
		ASSERT(weather.IsHourly());
		ASSERT(v >= H_PRCP);

		CWeatherAccumulator accumulator(CTM(CTM::DAILY, CTM::FOR_EACH_YEAR));
		//if (weather.GetParent() != NULL)
		//{
		//	//Compute noon-noon, 06-06, 18-18 from the previous and current day
		//	const CWeatherDay& previousDay = weather.GetPrevious();

		//	//for (size_t h = 12; h<24; h++)
		//	for (size_t h = 18; h<24; h++)
		//		accumulator.Add(previousDay[h].GetTRef(), v, previousDay[h][v]);

		//}

		for (size_t h = 0; h < 24; h++)
			accumulator.Add(weather[h].GetTRef(), v, weather[h][v]);

		//flush data
		//accumulator.ResetMidnight();
		//if (weather.GetParent() != NULL)
		//{
		//	//Compute noon-noon, 06-06, 18-18 from the previous and current day
		//	const CWeatherDay& nextDay = weather.GetNext();

		//	//for (size_t h = 12; h<24; h++)
		//	for (size_t h = 0; h<06; h++)//<=06 to fluch the data
		//		accumulator.Add(nextDay[h].GetTRef(), v, nextDay[h][v]);

		//}

		return accumulator[v];
	}

	//**************************************************************************************************************
	//COverheat

	double COverheat::GetTmin(const CWeatherDay& weather)const
	{
		return weather[H_TMIN][MEAN];
	}

	double COverheat::GetTmax(const CWeatherDay& weather)const
	{
		if (m_overheat == 0)//by optimisation, avoid compute statistic
			return weather[H_TMAX][MEAN];

		return weather[H_TMAX][MEAN] + weather[H_TRNG2][MEAN] * m_overheat;
	}

	double COverheat::GetOverheat(const CWeatherDay& weather, size_t h, size_t hourTmax)const
	{
		double OH = 0;
		if (m_overheat != 0 && weather[H_TMIN].IsInit() && weather[H_TMAX].IsInit())
		{
			double Fo = 0.5 * (1 + cos((double(hourTmax) - h) / 12.0 * PI));
			double maxOverheat = weather[H_TRNG2][MEAN] * m_overheat;
			OH = maxOverheat * Fo;
		}

		return OH;
	}

	double COverheat::GetT(const CWeatherDay& weather, size_t h, size_t hourTmax)const
	{
		ASSERT(hourTmax < 24);


		double T = -999;
		if (weather[H_TMIN].IsInit() && weather[H_TMAX].IsInit())
		{
			const CWeatherDay& d1 = weather.GetPrevious();
			const CWeatherDay& d2 = weather;
			const CWeatherDay& d3 = weather.GetNext();

			double Tmin[3] = { GetTmin(d1), GetTmin(d2), GetTmin(d3) };
			double Tmax[3] = { GetTmax(d1), GetTmax(d2), GetTmax(d3) };

			T = WBSF::GetDoubleSine(Tmin, Tmax, h, hourTmax - 12, hourTmax);
		}

		return T;
	}
	//**************************************************************************************************************
	//CWeatherAccumulator

	const CStatistic CWeatherAccumulator::EMPTY_STAT;
	CWeatherAccumulator::CWeatherAccumulator(const CTM& TM)
	{
		m_TM = TM;

		m_deltaHourMin = 20;
		m_minimumHours.fill(6);
		m_minimumHours[HOURLY_DATA::H_PRCP] = 20;
		m_minimumHours[HOURLY_DATA::H_SRAD] = 20;
		m_minimumHours[HOURLY_DATA::H_SNDH] = 1;
		m_minimumHours[HOURLY_DATA::H_SWE] = 1;
		m_minimumDays.fill(22);

		//m_midnightVariables.fill(CStatistic());
		//for (size_t h = 0; h<m_midnightTRefMatrix.size(); h++)
		//	m_midnightTRefMatrix[h].fill(0);

		//m_midnightVariablesTmp.fill(CStatistic());
		//for (size_t h = 0; h<m_midnightTRefMatrixTmp.size(); h++)
		//	m_midnightTRefMatrixTmp[h].fill(0);

		m_noonVariablesTmp.fill(CStatistic());
		for (size_t h = 0; h < m_noonTRefMatrixTmp.size(); h++)
			m_noonTRefMatrixTmp[h].fill(0);

		m_06VariablesTmp.fill(CStatistic());
		for (size_t h = 0; h < m_06TRefMatrixTmp.size(); h++)
			m_06TRefMatrixTmp[h].fill(0);

		m_18VariablesTmp.fill(CStatistic());
		for (size_t h = 0; h < m_18TRefMatrixTmp.size(); h++)
			m_18TRefMatrixTmp[h].fill(0);


		m_22VariablesTmp.fill(CStatistic());
		for (size_t h = 0; h < m_22TRefMatrixTmp.size(); h++)
			m_22TRefMatrixTmp[h].fill(0);


		ResetStat();
		ResetMidnight();
		ResetNoon();
		Reset06();
		Reset18();
		Reset22();
	}


	ERMsg CWeatherAccumulator::Add(const StringVector& data, const CWeatherFormat& format)
	{
		ERMsg msg;


		CTRef Tref = format.GetTRef(data);

		if (!Tref.IsInit())
		{
			msg.ajoute(GetString(IDS_BSC_INVALID_TIME_REF));
			return msg;
		}

		//CStatistic Tair;

		for (size_t i = 0; i < (int)format.size() && i < data.size(); i++)
		{
			if (IsVariable(format[i].m_var))
			{
				CStatistic stat;
				if (!data[i].empty())
				{
					double value = ToDouble(data[i]);
					if (value > format.GetNoData())
						stat += value;
				}//element is an non empty variable

				Add(Tref, format[i].m_var, stat);//always add stat to reset day
			}
		}//for all element


		return msg;
	}

	//static bool TRefIsChanging(CTM m_TM, CTRef m_lastTRef, CTRef Tref, int shift = 0)
	//{ 
	//	bool bIsInit = m_lastTRef.IsInit();
	//	CTRef T1 = bIsInit ? (Tref - shift).Transform(m_TM) : CTRef();
	//	CTRef T2 = bIsInit ? (m_lastTRef - shift).Transform(m_TM) : CTRef();
	//
	//	return bIsInit && T1 != T2;
	//}

	void CWeatherAccumulator::Add(CTRef Tref, size_t v, const CStatistic& value)
	{
		assert(Tref.GetTM().Type() == CTM::HOURLY || Tref.GetTM().Type() == CTM::DAILY);

		if (Tref.GetTM().Type() == CTM::HOURLY)
		{
			if (m_bStatComputed)
				ResetStat();

			//transfer data after added
			if (TRefIsChanging(Tref, -12))
				ResetNoon();

			if (TRefIsChanging(Tref, -06))
				Reset18();

			if (TRefIsChanging(Tref, -02))
				Reset22();

			if (TRefIsChanging(Tref, 00))
				ResetMidnight();

			if (TRefIsChanging(Tref, 06))
				Reset06();

			if (value.IsInit())
			{
				m_midnightTRefMatrix[Tref.GetHour()][v] += (int)value[NB_VALUE];
				m_midnightVariables.m_bInit = true;
				m_midnightVariables[v] += value;
				m_midnightVariables.m_period += Tref;

				//m_midnightTRefMatrixTmp[Tref.GetHour()][v] += (int)value[NB_VALUE];
				//m_midnightVariablesTmp.m_bInit = true;
				//m_midnightVariablesTmp[v] += value;
				//m_midnightVariablesTmp.m_period += Tref;

				m_noonTRefMatrixTmp[Tref.GetHour()][v] += (int)value[NB_VALUE];
				m_noonVariablesTmp.m_bInit = true;
				m_noonVariablesTmp[v] += value;
				m_noonVariablesTmp.m_period += Tref;

				m_06TRefMatrixTmp[Tref.GetHour()][v] += (int)value[NB_VALUE];
				m_06VariablesTmp.m_bInit = true;
				m_06VariablesTmp[v] += value;
				m_06VariablesTmp.m_period += Tref;

				m_18TRefMatrixTmp[Tref.GetHour()][v] += (int)value[NB_VALUE];
				m_18VariablesTmp.m_bInit = true;
				m_18VariablesTmp[v] += value;
				m_18VariablesTmp.m_period += Tref;

				m_22TRefMatrixTmp[Tref.GetHour()][v] += (int)value[NB_VALUE];
				m_22VariablesTmp.m_bInit = true;
				m_22VariablesTmp[v] += value;
				m_22VariablesTmp.m_period += Tref;
			}

			m_lastTRef = Tref;
		}
		else if (Tref.GetTM().Type() == CTM::DAILY)
		{
			ASSERT(Tref.GetTM().Type() == CTM::DAILY);

			if (TRefIsChanging(Tref))
			{
				ResetStat();

				Reset06();
				Reset18();
				Reset22();
				ResetNoon();
				ResetMidnight();
			}

			if (value.IsInit())
			{
				m_variables.m_bInit = true;
				m_variables[v] += value;
				m_variables.m_period += Tref;
			}

			m_lastTRef = Tref;
		}
	}

	const CStatistic& CWeatherAccumulator::GetStat(size_t v)const
	{
		if (m_TM.Type() != CTM::HOURLY)
		{
			ComputeStatistic();
			return m_variables[v];
		}


		return m_midnightVariables[v];
	}

	void CWeatherAccumulator::ComputeStatistic()const
	{
		assert(m_TM.Type() != CTM::HOURLY);

		if (!m_bStatComputed)
		{
			CWeatherAccumulator& me = const_cast<CWeatherAccumulator&>(*this);

			if (m_TM.Type() == CTM::DAILY && GTRef().GetTM().Type() == CTM::HOURLY)
			{
				for (size_t v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					me.m_variables[v].clear();//reset var

					//CStatistic stat = GetStat(v, MIDNIGHT_MIDNIGHT);
					//CStatistic stat = (v == H_TMIN) ? GetStat(v, P18_18) : GetStat(v, MIDNIGHT_MIDNIGHT);

					//CStatistic stat = (v == H_TMIN) ? GetStat(v, NOON_NOON) : GetStat(v, MIDNIGHT_MIDNIGHT) ;

					CStatistic stat;
					switch (DAILY_AGREGATION)
					{
					case ACCUMUL_12_00: stat = (v == H_TMIN) ? GetStat(v, NOON_NOON) : GetStat(v, MIDNIGHT_MIDNIGHT); break;
					case ACCUMUL_18_00:	stat = (v == H_TMIN) ? GetStat(v, P18_18) : GetStat(v, MIDNIGHT_MIDNIGHT); break;
					case ACCUMUL_18_18:	stat = (v == H_TMIN) ? GetStat(v, P18_18) : GetStat(v, P18_18); break;
					case ACCUMUL_22_22: stat = (v == H_TMIN) ? GetStat(v, P22_22) : GetStat(v, P22_22); break;
					case ACCUMUL_00_00:	stat = (v == H_TMIN) ? GetStat(v, MIDNIGHT_MIDNIGHT) : GetStat(v, MIDNIGHT_MIDNIGHT); break;
						//case ACCUMUL_18_06:	stat = (v == H_TMIN) ? GetStat(v, P18_18) : GetStat(v, P06_06); break;
					default: ASSERT(false);
					}

					//CStatistic stat = (v == H_TMIN) ? GetStat(v, P18_18) : GetStat(v, P06_06);
					//CStatistic stat = (v == H_TMIN) ? GetStat(v, NOON_NOON) : GetStat(v, MIDNIGHT_MIDNIGHT);
					//if (!stat.IsInit() && v == H_TMAX)
						//stat = GetStat(v, MIDNIGHT_MIDNIGHT);

					if (stat.IsInit())
					{
						switch (v)
						{
						case H_TMIN:	me.m_variables[v] = stat[LOWEST]; break;
						case H_TMAX:	me.m_variables[v] = stat[HIGHEST]; break;
						case H_PRCP:	me.m_variables[v] = stat[SUM]; break;
						default:		me.m_variables[v] = stat[MEAN];
						}

					}
				}//for all variables

				if (!me.m_variables[H_TMIN].IsInit() || !me.m_variables[H_TMAX].IsInit())
				{
					me.m_variables[H_TMIN].clear();//reset var
					me.m_variables[H_TMAX].clear();//reset var
				}
				else
				{
					ASSERT(me.m_variables[H_TMAX][MEAN] >= me.m_variables[H_TMIN][MEAN]);

					//in some case noon-noon minimum can be greather than midnight-midnight max
					//a vérifier: quoi faire dans ce cas ???
					if (me.m_variables[H_TMAX][MEAN] < me.m_variables[H_TMIN][MEAN])
						Switch(me.m_variables[H_TMAX], me.m_variables[H_TMIN]);
				}

				//			ASSERT(me.m_variables[H_TMAX][MEAN] >= me.m_variables[H_TMIN][MEAN]);
			}//for all variable	

			me.m_variables.m_bInit = true;
			me.m_bStatComputed = true;
		}

	}
	const CStatistic& CWeatherAccumulator::GetStat(size_t v, int sourcePeriod)const
	{
		assert(m_TM.Type() == CTM::DAILY && GTRef().GetTM().Type() == CTM::HOURLY);

		bool bValid = true;

		if (sourcePeriod == MIDNIGHT_MIDNIGHT)
		{
			//in case of the last TRef is in the night, we take the current accumulation 
			ASSERT(m_midnightTRefMatrix.size() == 24);

			if (v == H_TMAX)
			{
				bValid = false;
				for (size_t h = 14; h <= 16 && !bValid; h++)
					if (m_midnightTRefMatrix[h][v] > 0)
						bValid = true;

				if (!bValid)
				{
					if (m_midnightVariables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 14; h <= 16 && !bValid; h++)
							if (m_midnightTRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmax
					}
				}
			}
			else if (v == H_TMIN)
			{
				bValid = false;
				for (size_t h = 3; h <= 6 && !bValid; h++)
					if (m_midnightTRefMatrix[h][v] > 0)
						bValid = true;

				if (!bValid)
				{
					if (m_midnightVariables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 3; h <= 6 && !bValid; h++)
							if (m_midnightTRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmax
					}
				}
			}
			else
			{
				CStatistic NbTRef;
				for (size_t h = 0; h < m_midnightTRefMatrix.size(); h++)
					if (m_midnightTRefMatrix[h][v] > 0)
						NbTRef += (int)h;

				if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
					m_midnightVariables[v][NB_VALUE] < m_minimumHours[v])
				{
					bValid = false;
				}
			}

			if (bValid)
				return m_midnightVariables[v];
			//}
			//else
			//{
			//	ASSERT(m_midnightTRefMatrix2.size() == 24);
			//	CStatistic NbTRef;
			//	for (size_t h = 0; h < m_midnightTRefMatrix2.size(); h++)
			//		if (m_midnightTRefMatrix2[h][v]>0)
			//			NbTRef += (int)h;

			//	if (v == H_TMAX)
			//	{
			//		bValid = false;
			//		for (size_t h = 14; h <= 16 && !bValid; h++)
			//			if (m_midnightTRefMatrix2[h][v] > 0)
			//				bValid = true;

			//		if (!bValid)
			//		{
			//			if (m_midnightVariables2[H_TAIR][NB_VALUE] >= m_minimumHours[v])
			//			{
			//				for (size_t h = 14; h <= 16 && !bValid; h++)
			//					if (m_midnightTRefMatrix2[h][H_TAIR] > 0)
			//						bValid = true;

			//				if (bValid)
			//					v = H_TAIR; //use Tair stat instead of Tmax
			//			}
			//		}
			//	}
			//	else
			//	{
			//		if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
			//			m_midnightVariables2[v][NB_VALUE] < m_minimumHours[v])
			//		{
			//			bValid = false;
			//		}
			//	}

			//	if (bValid)//|| (GTRef().GetTM().Type() != CTM::HOURLY)
			//		return m_midnightVariables2[v];
			//}
		}
		else if (sourcePeriod == NOON_NOON)
		{
			ASSERT(m_noonTRefMatrix.size() == 24);

			if (v == H_TMIN)
			{
				bValid = false;
				if (m_noonVariables[v][NB_VALUE] >= m_minimumHours[v])
				{
					for (size_t h = 3; h <= 6 && !bValid; h++)
						if (m_noonTRefMatrix[h][v] > 0)
							bValid = true;
				}


				//try with Tair 
				if (!bValid)
				{
					if (m_noonVariables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 3; h <= 6 && !bValid; h++)
							if (m_noonTRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmin
					}
				}
			}
			else
			{
				CStatistic NbTRef;
				for (size_t h = 0; h < m_noonTRefMatrix.size(); h++)
					if (m_noonTRefMatrix[h][v] > 0)
						NbTRef += (int)h;

				if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
					m_noonVariables[v][NB_VALUE] < m_minimumHours[v])
				{
					bValid = false;
				}
			}

			if (bValid)
				return m_noonVariables[v];
		}
		else if (sourcePeriod == P06_06)
		{
			ASSERT(m_06TRefMatrix.size() == 24);

			if (v == H_TMAX)
			{
				bValid = false;
				for (size_t h = 14; h <= 16 && !bValid; h++)
					if (m_06TRefMatrix[h][v] > 0)
						bValid = true;

				if (!bValid)
				{
					if (m_06Variables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 14; h <= 16 && !bValid; h++)
							if (m_06TRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmax
					}
				}
			}
			else
			{
				CStatistic NbTRef;
				for (size_t h = 0; h < m_06TRefMatrix.size(); h++)
					if (m_06TRefMatrix[h][v] > 0)
						NbTRef += (int)h;

				if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
					m_06Variables[v][NB_VALUE] < m_minimumHours[v])
				{
					bValid = false;
				}
			}

			if (bValid)
				return m_06Variables[v];
		}
		else if (sourcePeriod == P18_18)
		{
			ASSERT(m_18TRefMatrix.size() == 24);

			if (v == H_TMAX)
			{
				bValid = false;
				for (size_t h = 14; h <= 16 && !bValid; h++)
					if (m_18TRefMatrix[h][v] > 0)
						bValid = true;

				if (!bValid)
				{
					if (m_18Variables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 14; h <= 16 && !bValid; h++)
							if (m_18TRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmax
					}
				}
			}
			else if (v == H_TMIN)
			{
				bValid = false;
				if (m_18Variables[v][NB_VALUE] >= m_minimumHours[v])
				{
					for (size_t h = 3; h <= 6 && !bValid; h++)
						if (m_18TRefMatrix[h][v] > 0)
							bValid = true;
				}


				//try with Tair 
				if (!bValid)
				{
					if (m_18Variables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 3; h <= 6 && !bValid; h++)
							if (m_18TRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmin
					}
				}
			}
			else
			{
				CStatistic NbTRef;
				for (size_t h = 0; h < m_18TRefMatrix.size(); h++)
					if (m_18TRefMatrix[h][v] > 0)
						NbTRef += (int)h;

				if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
					m_18Variables[v][NB_VALUE] < m_minimumHours[v])
				{
					bValid = false;
				}
			}

			if (bValid)
				return m_18Variables[v];
		}
		else if (sourcePeriod == P22_22)
		{
			ASSERT(m_22TRefMatrix.size() == 24);

			if (v == H_TMAX)
			{
				bValid = false;
				for (size_t h = 14; h <= 16 && !bValid; h++)
					if (m_22TRefMatrix[h][v] > 0)
						bValid = true;

				if (!bValid)
				{
					if (m_22Variables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 14; h <= 16 && !bValid; h++)
							if (m_22TRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmax
					}
				}
			}
			else if (v == H_TMIN)
			{
				bValid = false;
				if (m_22Variables[v][NB_VALUE] >= m_minimumHours[v])
				{
					for (size_t h = 3; h <= 6 && !bValid; h++)
						if (m_22TRefMatrix[h][v] > 0)
							bValid = true;
				}


				//try with Tair 
				if (!bValid)
				{
					if (m_22Variables[H_TAIR][NB_VALUE] >= m_minimumHours[v])
					{
						for (size_t h = 3; h <= 6 && !bValid; h++)
							if (m_22TRefMatrix[h][H_TAIR] > 0)
								bValid = true;

						if (bValid)
							v = H_TAIR; //use Tair stat instead of Tmin
					}
				}
			}
			else
			{
				CStatistic NbTRef;
				for (size_t h = 0; h < m_22TRefMatrix.size(); h++)
					if (m_22TRefMatrix[h][v] > 0)
						NbTRef += (int)h;

				if (NbTRef[HIGHEST] - NbTRef[LOWEST] < m_deltaHourMin ||
					m_22Variables[v][NB_VALUE] < m_minimumHours[v])
				{
					bValid = false;
				}
			}

			if (bValid)
				return m_22Variables[v];
		}


		return EMPTY_STAT;
	}
	//**************************************************************************************************************
	//CDataInterface
	void CDataInterface::SetData(const CWeatherAccumulator& weatherStat)
	{
		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			SetStat(v, weatherStat.GetStat(v));
	}

	//***********************************************************************************************************


	CHourlyData& CHourlyData::operator=(const CHourlyData& in)
	{
		if (&in != this)
		{
			CWeatherVariables::operator=(in);

			if (m_pParent == NULL)
				m_TRef = in.m_TRef;
		}

		return *this;
	}

	void CHourlyData::Reset()
	{
		for (iterator it = begin(); it != end(); it++)
			*it = WEATHER::MISSING;
	}


	const CStatistic& CHourlyData::GetData(HOURLY_DATA::TVarH v)const
	{
		//keyword thread_local let this function to be thred safe: do not remove
		static thread_local CStatistic STATISTIC_TMP;// [256];

		size_t num_thread = 0;// omp_get_thread_num() % 256;
		ASSERT(num_thread == 0);

		STATISTIC_TMP.Reset();
		//if (at(v)>WEATHER::MISSING)
		if (!IsMissing(at(v)))
			STATISTIC_TMP = CWeatherVariables::at(v);

		return STATISTIC_TMP;
	}

	CStatistic& CHourlyData::GetData(HOURLY_DATA::TVarH v)
	{
		//keyword thread_local let this function to be thred safe: do not remove
		static thread_local CStatistic STATISTIC_TMP;// [256];
		//size_t num_thread = 0;// omp_get_thread_num() % 256;
		//ASSERT(num_thread == 0);

		STATISTIC_TMP.Reset();

		if (!IsMissing(at(v)))
			STATISTIC_TMP = CWeatherVariables::at(v);

		return STATISTIC_TMP;
	}

	bool CHourlyData::GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const
	{
		if (!IsMissing(at(v)))
			stat = at(v);
		return stat.IsInit();
	}

	void CHourlyData::SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat)
	{
		if (stat.IsInit())
		{
			switch (v)
			{
			case H_SNOW:
			case H_PRCP:	CWeatherVariables::at(v) = (float)stat[SUM]; break;
			default:		CWeatherVariables::at(v) = (float)stat[MEAN];
			}
		}
		else
		{
			CWeatherVariables::at(v) = WEATHER::MISSING;
		}
	}

	//Ra[Out]: extraterrestrial radiation for 1-Hour Periods [MJ m-2 h-1]
	double CHourlyData::GetExtraterrestrialRadiation()const
	{
		const CLocation& loc = GetLocation();
		return CASCE_ETsz::GetExtraterrestrialRadiationH(GetTRef(), loc.m_lat, loc.m_lon, loc.m_alt);
	}


	//Fcd[In]	: Fcd of previous time step
	//Fcd[In]	: Fcd of current time step	
	double CHourlyData::GetNetRadiation(double& Fcd)const
	{
		const CHourlyData& me = *this;

		const CLocation& loc = GetLocation();
		double Ra = CASCE_ETsz::GetExtraterrestrialRadiationH(GetTRef(), loc.m_lat, loc.m_lon, loc.m_alt);
		if (Ra > 0)//if daytime update Fcd
		{
			double Rs = me[H_SRMJ];
			double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, loc.m_alt);
			Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);
		}

		//compute new Fcd
		double Rnl = CASCE_ETsz::GetNetLongWaveRadiationH(me[H_TAIR], me[H_EA], Fcd);
		double Rns = CASCE_ETsz::GetNetShortWaveRadiation(me[H_SRMJ]);

		return  CASCE_ETsz::GetNetRadiation(Rns, Rnl);// hourly incoming radiation [MJ/(m²·h)]
	}


	//l[out]: latent heat of vaporization of water[MJ kg - 1]
	double CHourlyData::GetLatentHeatOfVaporization()const
	{
		const CHourlyData& me = *this;
		return 2.5023 - 0.00243054 * me[H_TAIR];
	}

	CStatistic CHourlyData::GetVarEx(HOURLY_DATA::TVarEx v)const
	{
		const CHourlyData& me = *this;
		const CLocation& loc = GetLocation();
		CTRef TRef = GetTRef();

		CStatistic stat;
		switch (v)
		{
		case H_KELV:	stat = K(); break;	//temperature in kelvin
		case H_PSYC:	stat = !WEATHER::IsMissing(at(H_PRES)) ? CASCE_ETsz::GetPsychrometricConstant(at(H_PRES) / 10) : WEATHER::MISSING; break;
		case H_SSVP:	stat = !WEATHER::IsMissing(at(H_TAIR)) ? CASCE_ETsz::GetSlopeOfSaturationVaporPressure(at(H_TAIR)) : WEATHER::MISSING; break;
		case H_LHVW:	stat = GetLatentHeatOfVaporization(); break;	// latent heat of vaporization of water [MJ kg-1]
		case H_FNCD:	stat = !WEATHER::IsMissing(at(H_SRAD)) ? CASCE_ETsz::GetCloudinessFunction(me[H_SRMJ], me[H_CSRA]) : WEATHER::MISSING; break;
		case H_CSRA:	stat = CASCE_ETsz::GetClearSkySolarRadiation(GetExtraterrestrialRadiation(), loc.m_alt); break;//faudrait chnager pour W/m²
		case H_EXRA:	stat = GetExtraterrestrialRadiation(); break;//faudrait chnager pour W/m²
		case H_SWRA:	stat = !WEATHER::IsMissing(at(H_SRAD)) ? CASCE_ETsz::GetNetShortWaveRadiation(me[H_SRMJ]) : WEATHER::MISSING; break;
		case H_ES:		stat = !WEATHER::IsMissing(at(H_TAIR)) ? eᵒ(at(H_TAIR)) : WEATHER::MISSING; break;//[kPa]
		case H_EA:		stat = !WEATHER::IsMissing(at(H_TDEW)) ? eᵒ(at(H_TDEW)) : WEATHER::MISSING; break;//[kPa]
		case H_VPD:		stat = !WEATHER::IsMissing(at(H_TAIR)) && !WEATHER::IsMissing(at(H_TDEW)) ? max(0.0, eᵒ(at(H_TAIR)) - eᵒ(at(H_TDEW))) : WEATHER::MISSING; break; //[kPa]
		case H_TNTX:	stat = !WEATHER::IsMissing(at(H_TAIR)) ? at(H_TAIR) : WEATHER::MISSING; break;
		case H_TRNG2:	stat = !WEATHER::IsMissing(at(H_TAIR)) ? 0 : WEATHER::MISSING; break;
		case H_SRMJ:	stat = !WEATHER::IsMissing(at(H_SRAD)) ? at(H_SRAD) * 3600.0 / 1000000 : WEATHER::MISSING; break; //[MJ/m²]
		default:ASSERT(false);
		}


		return stat;
	}



	CDailyWaveVector& CHourlyData::GetHourlyGeneration(CDailyWaveVector& t, size_t method, size_t step, double PolarDayLength, const COverheat& overheat) const
	{
		assert(false);
		//don't call GetAllenWave on hourly data
		return t;
	}

	void CHourlyData::WriteStream(ostream& stream, const CWVariables& variable, bool /*asStat*/)const
	{
		const CHourlyData& me = *this;
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				write_value(stream, me[v]);
	}

	void CHourlyData::ReadStream(istream& stream, const CWVariables& variable, bool /*asStat*/)
	{
		const CHourlyData& me = *this;
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				read_value(stream, me[v]);
	}


	//****************************************************************************************************************
	CWeatherDay& CWeatherDay::operator=(const CWeatherDay& in)
	{
		if (&in != this)
		{
			if (in.m_pHourlyData.get())
			{
				ASSERT(IsHourly());
				ManageHourlyData();
				*m_pHourlyData = *in.m_pHourlyData;

				/*if (IsHourly())
				{
					ManageHourlyData();
					*m_pHourlyData = *in.m_pHourlyData;
				}
				else
				{
					in.CompileDailyStat();
				}*/
			}

			m_dailyStat = in.m_dailyStat;
		}

		return *this;
	}

	void CWeatherDay::Reset()
	{
		if (IsHourly())
		{
			for (iterator it = begin(); it != end(); it++)
				it->Reset();
		}

		m_dailyStat.clear();
	}

	bool CWeatherDay::operator==(const CWeatherDay& in)const
	{
		bool bEqual = true;// IsHourly() == in.IsHourly();
		if (IsHourly() && in.IsHourly())
		{
			if (m_pHourlyData.get() && in.m_pHourlyData.get())
				if (*m_pHourlyData != *in.m_pHourlyData)bEqual = false;
		}
		else
		{
			if (m_dailyStat != in.m_dailyStat)bEqual = false;
		}


		return bEqual;
	}

	bool CWeatherDay::GetStat(HOURLY_DATA::TVarH v, CStatistic& stat)const
	{
		if (HourlyDataExist())
			CompileDailyStat();

		stat = m_dailyStat[v];
		return stat.IsInit();
	}


	void CWeatherDay::SetStat(HOURLY_DATA::TVarH v, const CStatistic& stat)
	{
		m_dailyStat[v] = stat;
		m_dailyStat.m_bInit = false;
	}


	//does we have to elimininate not enaugh data here ????
	void CWeatherDay::CompileDailyStat(bool bFoceCompile)const
	{
		//STATISTIC_MUTEX.lock();
		if (!m_dailyStat.m_bInit || bFoceCompile)
		{
			CWeatherDay& me = const_cast<CWeatherDay&>(*this);

			if (IsHourly())
			{
				bool bHourlyComputed = GetWeatherYears().IsHourlyComputed();
				bool bIsCompilingHourly = GetWeatherYears().IsCompilingHourly();
				if (!bHourlyComputed && !bIsCompilingHourly)
				{
					CWeatherAccumulator accumulator(CTM(CTM::DAILY, CTM::FOR_EACH_YEAR));
					accumulator.m_minimumHours.fill(0);
					accumulator.m_minimumDays.fill(0);

					//if (DAILY_AGREGATION == ACCUMUL_22_22)
					//{
					//	if (m_pParent != NULL && IsYearInit((GetTRef() - 1).GetYear()))
					//	{
					//		//Compute noon-noon from the previous and current day
					//		const CWeatherDay& previousDay = me.GetPrevious();
					//		for (size_t h = 22; h < 24; h++)
					//		{
					//			accumulator.Add(previousDay[h].GetTRef(), H_TMIN, previousDay[h][H_TMIN]);
					//			accumulator.Add(previousDay[h].GetTRef(), H_TAIR, previousDay[h][H_TAIR]);
					//			accumulator.Add(previousDay[h].GetTRef(), H_TMAX, previousDay[h][H_TMAX]);
					//		}
					//	}
					//}
					//else 
					if (DAILY_AGREGATION != ACCUMUL_00_00)
					{
						if (m_pParent != NULL && IsYearInit((GetTRef() - 1).GetYear()))
						{
							//Compute noon-noon from the previous and current day
							const CWeatherDay& previousDay = me.GetPrevious();
							for (size_t h = 12; h < 24; h++)
							{
								accumulator.Add(previousDay[h].GetTRef(), H_TMIN, previousDay[h][H_TMIN]);
								accumulator.Add(previousDay[h].GetTRef(), H_TAIR, previousDay[h][H_TAIR]);
								accumulator.Add(previousDay[h].GetTRef(), H_TMAX, previousDay[h][H_TMAX]);
							}

						}
					}


					me.m_dailyStat.clear();
					for (size_t h = 0; h < 24; h++)
					{
						//always add var to update flush
						for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
							accumulator.Add(me[h].GetTRef(), v, me[h][v]);


						if (me[h].HaveData())
						{
							if (IsMissing(me[h][H_RELH]) && !IsMissing(me[h][H_TAIR]) && !IsMissing(me[h][H_TDEW]))
								me[h][H_RELH] = (float)Td2Hr(me[h][H_TAIR], me[h][H_TDEW]);

							if (IsMissing(me[h][H_TDEW]) && !IsMissing(me[h][H_TAIR]) && !IsMissing(me[h][H_RELH]))
								me[h][H_TDEW] = (float)Hr2Td(me[h][H_TAIR], me[h][H_RELH]);

							for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
							{
								if (!IsMissing(me[h][v]))
								{
									//accumulator.Add(me[h].GetTRef(), v, me[h][v]);

									if (v != H_TMIN && v != H_TMAX)
										me.m_dailyStat[v] += me[h][v];
								}
							}

							me.m_dailyStat.m_period += me[h].GetTRef();
						}


					}

					//if (DAILY_AGREGATION == ACCUMUL_18_06)
					//{
					//	if (m_pParent != NULL && IsYearInit((GetTRef() + 1).GetYear()))
					//	{
					//		//Compute 06-06 of the next day
					//		const CWeatherDay& nextDay = GetNext();
					//		for (size_t h = 0; h < 12; h++)//<= 06 flush de data
					//		{
					//			accumulator.Add(nextDay[h].GetTRef(), H_TMIN, nextDay[h][H_TMIN]);
					//			accumulator.Add(nextDay[h].GetTRef(), H_TAIR, nextDay[h][H_TAIR]);
					//			accumulator.Add(nextDay[h].GetTRef(), H_TMAX, nextDay[h][H_TMAX]);
					//		}

					//	}
					//}

					ASSERT(!m_dailyStat[H_TAIR].IsInit() || (m_dailyStat[H_TAIR][MEAN] >= -90 && m_dailyStat[H_TAIR][MEAN] < 90));
					ASSERT(!m_dailyStat[H_TMIN].IsInit() || (m_dailyStat[H_TMIN][MEAN] >= -90 && m_dailyStat[H_TMIN][MEAN] < 90));
					ASSERT(!m_dailyStat[H_TMAX].IsInit() || (m_dailyStat[H_TMAX][MEAN] >= -90 && m_dailyStat[H_TMAX][MEAN] < 90));

					CStatistic Tmin = accumulator.GetStat(H_TMIN);
					CStatistic Tmax = accumulator.GetStat(H_TMAX);
					if (Tmin.IsInit() && Tmax.IsInit())
					{
						ASSERT(Tmax[MEAN] >= Tmin[MEAN]);

						me.m_dailyStat[H_TMIN] = Tmin;
						me.m_dailyStat[H_TMAX] = Tmax;
					}
				}


			}
			else
			{
				me.m_dailyStat.m_period = CTPeriod(m_TRef, m_TRef);
			}


			me.m_dailyStat.m_bInit = true;
		}

		//STATISTIC_MUTEX.unlock();
	}


	static double GetTdaylightEstimate(double Tmax, double Tmean)
	{
		_ASSERTE(!IsMissing(Tmax) && !IsMissing(Tmean));

		static const double TDAYCOEF = 0.45;  // (dim) daylight air temperature coefficient (dim) 
		return ((Tmax - Tmean) * TDAYCOEF) + Tmean;
	}

	double CWeatherDay::GetTdaylight()const
	{
		const CWeatherDay& me = (const CWeatherDay&)(*this);
		CStatistic Tdaylight;
		if (IsHourly())
		{
			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			size_t sunrise = Round(sun.GetSunrise(GetTRef()));
			size_t sunset = min(23ll, Round(sun.GetSunset(GetTRef())));

			for (size_t h = sunrise; h <= sunset; h++)
				Tdaylight += me[h][H_TAIR];
		}
		else
		{
			Tdaylight = GetTdaylightEstimate(me[H_TMAX][MEAN], me[H_TAIR][MEAN]);
		}


		return Tdaylight[MEAN];
	}
	//
	//double CWeatherDay::GetCloudiness()const
	//{
	//	const CWeatherDay& me = *this;
	//
	//	//Julian day, in one base
	//	int J = int(GetTRef().GetJDay()+1);
	//
	//	double Rs = max(0.0, me[H_SRAD][SUM]);
	//	double Ra = CASCE_ETsz::GetExtraterrestrialRadiation(lat, J);
	//	double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, z);
	//	double Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);
	//
	//	return Fcd;
	//}
	//
	//double CWeatherDay::GetNetLongWaveRadiation(double Fcd)const
	//{
	//	const CWeatherDay& me = *this;
	//
	//	double Rnl = 0;
	//	if (IsHourly())
	//	{
	//		ASSERT(false);
	//		//for (size_t h = 0; h < 24; h++)
	//		//{ 
	//		//	//input Fcd is not use here
	//		//	double Fcd = me[h].GetCloudiness();
	//		//	Rnl += me[h].GetNetLongWaveRadiation(Fcd);
	//		//}
	//	}
	//	else
	//	{
	//		//double Fcd = GetCloudiness();
	//		double Tmin = me[H_TAIR][LOWEST];
	//		double Tmax = me[H_TAIR][HIGHEST];
	//		double Ea = me[H_EA][MEAN];
	//		Rnl = CASCE_ETsz::GetNetLongWaveRadiation(Tmin, Tmax, Ea, Fcd);
	//
	//	}
	//
	//	return Rnl;
	//}
	//
	//double CWeatherDay::GetNetShortWaveRadiation()const
	//{
	//	const CWeatherDay& me = *this;
	//
	//	double Rns = 0;
	//	if (IsHourly())
	//	{
	//		for (size_t h = 0; h < 24; h++)
	//			Rns += me[h].GetNetShortWaveRadiation();
	//	}
	//	else
	//	{
	//		double Rs = me[H_SRAD][MEAN];
	//		Rns = CASCE_ETsz::GetNetShortWaveRadiation(Rs);
	//	}
	//
	//	return Rns;
	//}

	double CWeatherDay::GetNetRadiation(double& Fcd)const
	{
		const CWeatherDay& me = *this;

		double Rn = 0;
		if (IsHourly())
		{
			for (size_t h = 0; h < 24; h++)
			{
				Rn += me[h].GetNetRadiation(Fcd); //MJ/(m²·h)
			}
		}
		else
		{
			const CLocation& loc = GetLocation();
			double Tmin = me[H_TMIN][MEAN];
			double Tmax = me[H_TMAX][MEAN];
			double Ea = me[H_EA][MEAN];		//vapor pressure [kPa]
			double Rs = me[H_SRMJ][SUM];	//net radiation in MJ/(m²·d)
			int J = int(GetTRef().GetJDay() + 1);

			double Ra = CASCE_ETsz::GetExtraterrestrialRadiation(loc.m_lat, J);
			double Rso = CASCE_ETsz::GetClearSkySolarRadiation(Ra, loc.m_alt);
			Fcd = CASCE_ETsz::GetCloudinessFunction(Rs, Rso);//compute new Fcd

			double Rnl = CASCE_ETsz::GetNetLongWaveRadiation(Tmin, Tmax, Ea, Fcd);
			double Rns = CASCE_ETsz::GetNetShortWaveRadiation(Rs);
			Rn = CASCE_ETsz::GetNetRadiation(Rns, Rnl);// daily incoming radiation [MJ/(m²·d)]
		}

		return Rn;
	}



	CStatistic CWeatherDay::GetVarEx(HOURLY_DATA::TVarEx v)const
	{

		ASSERT(m_pParent);
		const CWeatherDay& me = *this;
		const CLocation& loc = GetLocation();
		CTRef TRef = GetTRef();

		CStatistic stat;


		if (v == H_TNTX || v == H_TRNG2)
		{
			const CStatistic& Tmin = GetStat(H_TMIN);
			const CStatistic& Tmax = GetStat(H_TMAX);

			if (Tmin.IsInit() && Tmax.IsInit())
			{
				//ASSERT(Tmax[MEAN] >= Tmin[MEAN]);

				if (v == H_TNTX)
					stat = (Tmax[MEAN] + Tmin[MEAN]) / 2;
				else if (v == H_TRNG2)
					stat = Tmax[MEAN] - Tmin[MEAN];
			}
		}
		else
		{
			if (IsHourly())
			{
				for (size_t h = 0; h < 24; h++)
					stat += me[h].GetVarEx(v);
			}
			else
			{
				switch (v)
				{
				case H_KELV:
				{
					CStatistic Tmean = me[H_TNTX];
					stat = Tmean.IsInit() ? Tmean[MEAN] + 273.15 : WEATHER::MISSING;
					break;
				}
				case H_PSYC:	stat = me[H_PRES].IsInit() ? CASCE_ETsz::GetPsychrometricConstant(me[H_PRES][MEAN] / 10) : WEATHER::MISSING; break;
				case H_SSVP:	stat = me[H_TAIR].IsInit() ? CASCE_ETsz::GetSlopeOfSaturationVaporPressure(me[H_TAIR][MEAN]) : WEATHER::MISSING; break;
				case H_LHVW:	stat = me[H_TAIR].IsInit() ? 2.5023 - 0.00243054 * me[H_TAIR][MEAN] : WEATHER::MISSING; break;	// latent heat of vaporization of water [MJ kg-1]
				case H_FNCD:	stat = me[H_SRAD].IsInit() ? CASCE_ETsz::GetCloudinessFunction(me[H_SRMJ][SUM], CWeatherDay::GetVarEx(H_CSRA)[SUM]) : WEATHER::MISSING; break;
				case H_CSRA:	stat = CASCE_ETsz::GetClearSkySolarRadiation(CWeatherDay::GetVarEx(H_EXRA), loc.m_z); break;
				case H_EXRA:	stat = CASCE_ETsz::GetExtraterrestrialRadiation(loc.m_lat, int(TRef.GetJDay() + 1)); break;
				case H_SWRA:	stat = me[H_SRAD].IsInit() ? CASCE_ETsz::GetNetShortWaveRadiation(me[H_SRMJ][SUM]) : WEATHER::MISSING; break;
				case H_ES:		stat = (me[H_TMIN].IsInit() && me[H_TMAX].IsInit()) ? eᵒ(me[H_TMIN], me[H_TMAX]) : WEATHER::MISSING; break;//[kPa]
				case H_EA:		stat = me[H_TDEW].IsInit() ? eᵒ(me[H_TDEW][LOWEST], me[H_TDEW][HIGHEST]) : WEATHER::MISSING; break;//[kPa]
				case H_VPD:		stat = (me[H_TMIN].IsInit() && me[H_TMAX].IsInit() && me[H_TDEW].IsInit()) ? max(0.0, me[H_ES][MEAN] - me[H_EA][MEAN]) : WEATHER::MISSING; break;//[kPa]
				case H_SRMJ:	stat = me[H_SRAD].IsInit() ? me[H_SRAD][MEAN] * 24 * 3600 / 1000000 : WEATHER::MISSING; break;
				case H_TNTX:	//apply only for daily value, see upper
				case H_TRNG2:
				default:ASSERT(false);
				}
			}
		}

		return stat;
	}

	double CWeatherDay::GetDoubleSine(double h, double PolarDayLength)const
	{
		double Tair = WEATHER::MISSING;

		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& me = *this;
		const CWeatherDay& dn = GetNext();

		if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
			&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
		{

			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			//int sunrise = Round(sun.GetSunrise(GetTRef()));
			//int noon = Round(sun.GetSolarNoon(GetTRef()));
			double sunrise = sun.GetSunrise(GetTRef());
			double noon = sun.GetSolarNoon(GetTRef());
			double D = sun.GetDayLength(GetTRef());

			double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
			double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

			if (D < PolarDayLength)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D > (24 - PolarDayLength))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				Tair = WBSF::GetDoubleSine(Tmin, Tmax, h, sunrise, noon + 2);
			}
		}

		return Tair;
	}


	double CWeatherDay::GetAllenT(double h, size_t hourTmin, size_t hourTmax, double PolarDayLength)const
	{
		ASSERT(hourTmin < hourTmax);
		ASSERT(hourTmax >= 12 && hourTmax < 24);

		double Tair = WEATHER::MISSING;

		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& me = *this;
		const CWeatherDay& dn = GetNext();

		if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
			&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
		{

			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			//int sunrise = Round(sun.GetSunrise(GetTRef()));
			//int noon = Round(sun.GetSolarNoon(GetTRef()));
			//double sunrise = sun.GetSunrise(GetTRef());
			//double noon = sun.GetSolarNoon(GetTRef());
			double D = sun.GetDayLength(GetTRef());

			double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
			double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

			if (D < PolarDayLength)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D > (24 - PolarDayLength))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				Tair = WBSF::GetDoubleSine(Tmin, Tmax, h, hourTmin, hourTmax);
			}
		}

		return Tair;
	}


	//sine-exponential method
	//Evaluation and calibration of three models for daily cycle of air temperature
	//Wann 1984
	//Brandsma (2006)
	double CWeatherDay::GetSineExponential(double h, size_t method, double PolarDayLength)const
	{
		double Tair = WEATHER::MISSING;

		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& me = *this;
		const CWeatherDay& dn = GetNext();

		if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
			&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
		{

			double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
			double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			double Tsr = sun.GetSunrise(GetTRef());
			double Tss = sun.GetSunset(GetTRef());
			double D = sun.GetDayLength(GetTRef());

			if (D < PolarDayLength)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D > (24 - PolarDayLength))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				Tair = WBSF::GetSineExponential(Tmin, Tmax, h, Tsr, Tss, method);
			}
		}

		return Tair;
	}

	double CWeatherDay::GetSinePower(double h, double PolarDayLength)const
	{
		double Tair = WEATHER::MISSING;

		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& me = *this;
		const CWeatherDay& dn = GetNext();

		if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
			&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
		{

			double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
			double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };


			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			double Tsr = sun.GetSunrise(GetTRef());
			double Tss = sun.GetSunset(GetTRef());
			double D = sun.GetDayLength(GetTRef());

			if (D < PolarDayLength)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D > (24 - PolarDayLength))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				Tair = WBSF::GetSinePower(Tmin, Tmax, h, Tsr, Tss);
			}
		}

		return Tair;
	}



	double CWeatherDay::GetErbs(double h, double PolarDayLength)const
	{
		double Tair = WEATHER::MISSING;

		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& me = *this;
		const CWeatherDay& dn = GetNext();

		if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
			&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
		{
			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);
			//int sunrise = Round(sun.GetSunrise(GetTRef()));
			//int noon = Round(sun.GetSolarNoon(GetTRef()));
			double sunrise = sun.GetSunrise(GetTRef());
			double noon = sun.GetSolarNoon(GetTRef());
			double D = sun.GetDayLength(GetTRef());

			double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
			double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

			if (D < PolarDayLength)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D > (24 - PolarDayLength))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				Tair = WBSF::GetErbs(Tmin, Tmax, h);
			}
		}

		return Tair;
	}



	//
	//double CWeatherDay::GetPolarInterpol(size_t h)const
	//{
	//	double Tair = WEATHER::MISSING;
	//
	//	const CWeatherDay& dp = GetPrevious();
	//	const CWeatherDay& me = *this;
	//	const CWeatherDay& dn = GetNext();
	//
	//	if (me[H_TMIN].IsInit() && dn[H_TMIN].IsInit()
	//		&& dp[H_TMAX].IsInit() && me[H_TMAX].IsInit())
	//	{
	//		double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
	//		double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };
	//
	//		Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
	//	}
	//
	//	return Tair;
	//}
	//An improved model for determining degree-day values from daily temperature data
	//Allen Wave Allen, 1976 J.C. Allen
	//A modified sine wave method for calculating degree-days Environ. Entomol., 5 (1976), pp. 388–396
	CDailyWaveVector& CWeatherDay::GetHourlyGeneration(CDailyWaveVector& t, size_t method, size_t step, double PolarDayLength, const COverheat& overheat) const
	{
		_ASSERTE(m_pParent);
		_ASSERTE(fmod(24.0, step) == 0);
		_ASSERTE(method < NB_HOURLY_GENERATION);
		_ASSERTE(step > 0 && step <= 24);


		if (!t.IsInit())
		{
			t.m_period = CTPeriod(GetTRef(), GetTRef());
		}
		else
		{
			t.m_period.End() = GetTRef();
		}


		if (IsHourly())
		{
			size_t nbStep = size_t(24.0 / step);
			for (size_t s = 0; s < nbStep; s++)
			{
				size_t h = size_t(s * step);
				double T = overheat.GetT(*this, h, 13);
				t.push_back((float)T);
			}
		}
		else
		{
			//ATTENTION : il n'y a pas de overheat ici????
					//ASSERT(false);

			const CWeatherDay& dp = GetPrevious();
			const CWeatherDay& me = *this;
			const CWeatherDay& dn = GetNext();
			ASSERT(dp[H_TMIN].IsInit() && me[H_TMIN].IsInit() && dn[H_TMIN].IsInit());
			ASSERT(dp[H_TMAX].IsInit() && me[H_TMAX].IsInit() && dn[H_TMAX].IsInit());

			//		double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
				//	double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

					//By RSA 23/01/2018
			double Tmin[3] = { overheat.GetTmin(dp), overheat.GetTmin(me), overheat.GetTmin(dn) };
			double Tmax[3] = { overheat.GetTmax(dp), overheat.GetTmax(me), overheat.GetTmax(dn) };


			//double-sine method
			const CLocation& loc = GetLocation();
			CSun sun(loc.m_lat, loc.m_lon);

			double Tsr = sun.GetSunrise(GetTRef());
			double Tsn = sun.GetSolarNoon(GetTRef());
			double Tss = sun.GetSunset(GetTRef());
			double D = sun.GetDayLength(GetTRef());

			int nbStep = int(24.0 / step);
			for (int s = 0; s < nbStep; s++)
			{
				int h = int(s * step);

				double Tair = -999;
				if (D < 3)
				{
					Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
				}
				else if (D > (24 - 3))
				{
					Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
				}
				else
				{
					switch (method)
					{
					case HG_DOUBLE_SINE: Tair = WBSF::GetDoubleSine(Tmin, Tmax, h, Tsr, Tsn + 2); break;
					case HG_SINE_EXP_BRANDSMA: Tair = WBSF::GetSineExponential(Tmin, Tmax, h, Tsr, Tss, SE_BRANDSMA); break;
					case HG_SINE_EXP_SAVAGE: Tair = WBSF::GetSineExponential(Tmin, Tmax, h, Tsr, Tss, SE_SAVAGE); break;
					case HG_SINE_POWER: Tair = WBSF::GetSinePower(Tmin, Tmax, h, Tsr, Tss); break;
					case HG_ERBS: Tair = WBSF::GetErbs(Tmin, Tmax, h); break;
					case HG_ALLEN_WAVE: Tair = WBSF::GetDoubleSine(Tmin, Tmax, h, 3, 15); break;
					case HG_POLAR: Tair = WBSF::GetPolarWinter(Tmin, Tmax, h); break;
					default: ASSERT(false);
					}
				}

				t.push_back(Tair);
			}//for all hours
		}

		return t;
	}


	void CWeatherDay::WriteStream(ostream& stream, const CWVariables& variable, bool asStat)const
	{
		assert(!IsHourly());

		for (size_t v = 0; v < variable.size(); v++)
		{
			if (variable[v])
			{
				if (asStat)
				{
					write_value(stream, m_dailyStat[v]);
				}
				else
				{
					float value = m_dailyStat[v].IsInit() ? m_dailyStat[v][MEAN] : -999;
					write_value(stream, value);
				}
			}
		}
	}


	void CWeatherDay::ReadStream(istream& stream, const CWVariables& variable, bool asStat)
	{
		assert(!IsHourly());

		for (size_t v = 0, u = 0; v < variable.size() && u < variable.count(); v++)
		{
			if (variable[v])
			{
				if (asStat)
				{
					read_value(stream, m_dailyStat[v]);
				}
				else
				{
					float value = -999;
					read_value(stream, value);
					if (value != -999)
						m_dailyStat[v] = value;
				}


				u++;
			}

		}


	}



	//*****************************************************************************************
	//Temperature

	void CWeatherDay::ComputeHourlyTair(size_t method)
	{
		_ASSERTE(m_pParent);

		CWeatherDay& me = *this;
		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& dn = GetNext();
		if (!dp[H_TMIN].IsInit() || !me[H_TMIN].IsInit() || !dn[H_TMIN].IsInit() ||
			!dp[H_TMAX].IsInit() || !me[H_TMAX].IsInit() || !dn[H_TMAX].IsInit())
			return;

		double Tmin[3] = { dp[H_TMIN][MEAN], me[H_TMIN][MEAN], dn[H_TMIN][MEAN] };
		double Tmax[3] = { dp[H_TMAX][MEAN], me[H_TMAX][MEAN], dn[H_TMAX][MEAN] };

		//double-sine method
		const CLocation& loc = GetLocation();
		CSun sun(loc.m_lat, loc.m_lon);
		//int sunrise = Round(sun.GetSunrise(GetTRef()));
		//int noon = Round(sun.GetSolarNoon(GetTRef()));


		double Tsr = sun.GetSunrise(GetTRef());
		double Tsn = sun.GetSolarNoon(GetTRef());
		double Tss = sun.GetSunset(GetTRef());
		double D = sun.GetDayLength(GetTRef());

		//From MELODIST
		//bool bPolars = sun.GetDayLength(GetTRef()) < 3;
		for (int h = 0; h < 24; h++)
		{
			double Tair = -999;
			if (D < 3)
			{
				Tair = WBSF::GetPolarWinter(Tmin, Tmax, h);
			}
			else if (D > (24 - 3))
			{
				Tair = WBSF::GetPolarSummer(Tmin, Tmax, h);
			}
			else
			{
				switch (method)
				{
				case HG_DOUBLE_SINE: Tair = WBSF::GetDoubleSine(Tmin, Tmax, h, Tsr, Tsn + 2); break;
				case HG_SINE_EXP_BRANDSMA: Tair = WBSF::GetSineExponential(Tmin, Tmax, h, Tsr, Tss, SE_BRANDSMA); break;
				case HG_SINE_EXP_SAVAGE: Tair = WBSF::GetSineExponential(Tmin, Tmax, h, Tsr, Tss, SE_SAVAGE); break;
				case HG_SINE_POWER: Tair = WBSF::GetSinePower(Tmin, Tmax, h, Tsr, Tss); break;
				case HG_ERBS: Tair = WBSF::GetErbs(Tmin, Tmax, h); break;
				case HG_ALLEN_WAVE: Tair = WBSF::GetDoubleSine(Tmin, Tmax, h, 3, 15); break;
				case HG_POLAR: Tair = WBSF::GetPolarWinter(Tmin, Tmax, h); break;
				default: ASSERT(false);
				}
			}

			me[h][H_TAIR] = float(Tair);
		}


	}


	//
	//void ComputeHourlyTrng(CWeatherDay& day)
	//{
	//	for (size_t h = 0; h < 24; h++)//hourly TRange set to zero
	//		day[h][H_TRNG] = 0;// day[H_TAIR][RANGE];
	//	
	//}

	enum TrClass { LOWER_8, BETWEEN_8_12, BETWEEN_12_16, GREATER_16, NB_TR_CALSS };
	enum TParams { ALPHA1, BETA1, ALPHA2, BETA2, NB_TR_PARAMS };

	static TrClass GetTrClass(float Tair)
	{
		TrClass Tr = TrClass(-1);

		if (Tair <= 8)
			Tr = LOWER_8;
		else if (Tair <= 12)
			Tr = BETWEEN_8_12;
		else if (Tair <= 16)
			Tr = BETWEEN_12_16;
		else
			Tr = GREATER_16;

		return Tr;
	}

	//*****************************************************************************************
	//Precipitation
	void CWeatherDay::ComputeHourlyPrcp()
	{
		CWeatherDay& me = *this;
		//const CWeatherDay& dp = GetPrevious();
		//const CWeatherDay& dn = GetNext();
		//if (!dp[H_PRCP].IsInit() || !me[H_PRCP].IsInit() || !dn[H_PRCP].IsInit())
			//return;
		
		if (!me[H_PRCP].IsInit())
			return;

		//CStatistic stats[3] = { dp[H_PRCP], me[H_PRCP], dn[H_PRCP] };
		double daily_prcp = Round(me[H_PRCP][SUM], 1);
		if (daily_prcp > 0)
		{

			//bool bBefor12 = stats[0][SUM] > 0;
			//bool bAfter12 = stats[2][SUM] > 0;
			//
			//short nbHourBefor12 = 0;
			//short nbHourAfter12 = 0;
			//
			//if (bBefor12 && bAfter12 || !bBefor12 && !bAfter12)
			//{
			//	if (stats[1][SUM] >= 9.6)
			//	{
			//		nbHourBefor12 = nbHourAfter12 = 12;
			//	}
			//	else if (stats[1][SUM] >= 1.6)
			//	{
			//		nbHourBefor12 = nbHourAfter12 = (short)floor(stats[1][SUM] / (0.8));
			//	}
			//	else
			//	{
			//		nbHourBefor12 = nbHourAfter12 = 1;
			//	}
			//}
			//else if (bBefor12)
			//{
			//	if (stats[1][SUM] >= 4.8)
			//	{
			//		nbHourBefor12 = 12;
			//	}
			//	else if (stats[1][SUM] >= 0.8)
			//	{
			//		nbHourBefor12 = (short)floor(stats[1][SUM] / (0.4));
			//	}
			//	else
			//	{
			//		nbHourBefor12 = 1;
			//	}
			//}
			//else
			//{
			//
			//	if (stats[1][SUM] >= 4.8)
			//	{
			//		nbHourAfter12 = 12;
			//	}
			//	else if (stats[1][SUM] >= 0.8)
			//	{
			//		nbHourAfter12 = (short)floor(stats[1][SUM] / (0.4));
			//	}
			//	else
			//	{
			//		nbHourAfter12 = 1;
			//	}
			//}
			//
			//if (bBefor12 || bAfter12)
			//{
			//	for (size_t h = 0; h < nbHourBefor12; h++)
			//		me[h][H_PRCP] = (float)int(10.0 * (stats[1][SUM] / (nbHourBefor12 + nbHourAfter12))) / 10.0;//truck at 0.1
			//
			//	for (size_t h = nbHourBefor12; h < 12; h++)
			//		me[h][H_PRCP] = 0;
			//
			//	for (size_t h = 0; h < nbHourAfter12; h++)
			//		me[23 - h][H_PRCP] = (float)int(10.0 * (stats[1][SUM] / (nbHourBefor12 + nbHourAfter12))) / 10.0;//truck at 0.1
			//
			//	for (size_t h = nbHourAfter12; h < 12; h++)
			//		me[23 - h][H_PRCP] = 0;
			//
			//	//compute difference du to rounding and add it to first hour
			//	double daily_sum = 0;
			//	for (size_t h = 0; h < 24; h++)
			//		daily_sum += me[h][H_PRCP];
			//
			//
			//	double diff = stats[1][SUM] - daily_sum;
			//
			//	if (nbHourBefor12 > 0)
			//		me[0][H_PRCP] += diff;
			//	else //nbHourAfter12
			//		me[23][H_PRCP] += diff;
			//
			//	ASSERT(me[0][H_PRCP] >= 0 && me[23][H_PRCP] >= 0);
			//}
			//else
			//{
			//	for (size_t h = 0; h < nbHourBefor12; h++)
			//		me[11 - h][H_PRCP] = (float)int(10.0 * (stats[1][SUM] / (nbHourBefor12 + nbHourAfter12))) / 10.0;//truck at 0.1
			//
			//	for (size_t h = nbHourBefor12; h < 12; h++)
			//		me[11 - h][H_PRCP] = 0;
			//
			//	for (size_t h = 0; h < nbHourAfter12; h++)
			//		me[12 + h][H_PRCP] = (float)int(10.0 * (stats[1][SUM] / (nbHourBefor12 + nbHourAfter12))) / 10.0;//truck at 0.1
			//
			//	for (size_t h = nbHourAfter12; h < 12; h++)
			//		me[12 + h][H_PRCP] = 0;
			//
			//	//compute difference du to rounding and add it to first hour
			//	double daily_sum = 0;
			//	for (size_t h = 0; h < 24; h++)
			//		daily_sum += me[h][H_PRCP];
			//
			//	double diff = stats[1][SUM] - daily_sum;
			//	if (nbHourBefor12 > 0)
			//		me[11][H_PRCP] += diff;
			//	else //nbHourAfter12
			//		me[12][H_PRCP] += diff;
			//
			//	ASSERT(me[11][H_PRCP] >= 0 && me[12][H_PRCP] >= 0);
			//}

		
			static const double P[NB_TR_CALSS][12][NB_TR_PARAMS] =
			{
				{
					{ -0.00284407541630448, 1.00113018767093, -0.00184565592484555, 0.982582019284295 },
					{-0.00291924547711281, 0.947203182645993, 0.000917735819975669, 1.0864231968764},
					{-0.00498210381984841, 1.88031295898049, 0.00142819525154501, 7.72408992207765 },
					{-0.00637392760554769, 1.94339978554979, 0.00214176173684284, -1.33058568862311},
					{0.00647473916416413, 1.39122671741714, 0.00220057722286499, 1.10872907340077  },
					{-0.00715749998932849, -0.069924403724782, 0.00306569606715863, 1.64884381334031},
					{-0.00622353293637693, 1.91581213991504, -0.000783472070386602, 1.41413353477661},
					{-0.00695111991343828, 1.87568088035894, 0.00307106644226698, 1.19362864982338	},
					{0.00226096151533121, 1.29642869911666, 0.00403105833171282, 1.1735417874032	},
					{ 0.00213179170676173, 8.32068087864758, 0.000866537412155696, 2.31886176934777	},
					{ 0.00206096303888215, -0.29030480064342, 0.000917510774277572, 1.02557702865428},
					{ -0.00260255875158911, 0.90769026873834, 0.00171352809419077, 0.998831529132042},
				},
				{
					{0.00924970121111457, 2.13683918134351, -0.00138042183698871, 0.984023641049206	},
					{0.009090558329706, 2.19419705523707, 0.00156690948538597, 1.15158481291706		},
					{0.00515718402225976, 0.866091612307948, 0.00426705233147077, 1.15831732502239	},
					{0.00932266073618154, 0.974580102953293, -0.00726471363281726, 1.89134657009226	},
					{0.016612385134461, -0.126368095139731, 0.00653549167548792, -1.1970499951173	},
					{0.0146845778496101, -0.161471241929103, -0.00744831322397603, 1.46155735128842	},
					{0.0213786947312919, -2.16945471893298, -0.0145393123462066, 1.47357012470363	},
					{0.0182422192683209, 0.799222315722166, 0.00882257332148857, 0.689345797383271	},
					{0.0193799068317784, 0.8148780118566, -0.014559883743585, 0.984519240421192		},
					{ 0.012778375456505, 0.868152119576047, 0.00185800453926523, -0.165859097157394	},
					{ 0.00862096994100147, 0.999658661053731, 0.00572021607231544, -0.603274965237277},
					{ 0.00409662718830537, 0.978831088768085, -0.00156553277243986, 1.5544116692213	 },
				},
				{
					{0.00317483748327035, 1.36401724602646, -0.00663955820868844, 1.42029365327545	 },
					{0.00829398579820174, 0.927783175299693, 0.00480843631774363, -0.669480855652638 },
					{0.014127568178258, 0.895861364380957, 0.0136160777908243, 2.81879442014052		 },
					{0.0229325987075367, -1.09102823489481, 0.00574950634520312, 1.27224178793115	 },
					{0.0242056957883524, -2.14530847951066, 0.00939795298521483, 0.340520641574183	 },
					{0.0291573647123203, 1.86664602319336, 0.0115923908678945, -0.214085937105532	 },
					{0.0411408819133923, -2.19177439456169, -0.0222720621002542, 0.530741376055701	 },
					{0.0391262386404851, -4.15603075512214, -0.0204321464646298, 0.0706198931558513	 },
					{0.0353639057547716, -0.114854480619116, 0.0159504633145749, -0.155391062368276	 },
					{ 0.013098686892585, -0.0920908553514463, -0.0123308593521949, 0.615576310396964 },
					{ 0.0091715592570439, 0.74160906564169, 0.00474405570841586, 0.911812498247948	 },
					{ 0.00746372114632317, -0.707411382703413, -0.00818997731863396, 0.552662089547024},
				},
				{
					{0.0137755908629707, 1.10858960927034, 0.00263961225438576, 1.29152321276311	  },
					{-0.0038798713351804, 0.219443240892891, 0.00998332335297548, 1.56279912236947	  },
					{-0.0217233994743617, 0.173632205904224, -0.00593630018018127, 1.0066353264767	  },
					{0.0239264984165522, -2.16646507717926, -0.0152124903638748, 0.0627030322284112	  },
					{0.0327987680950779, -1.17312224501802, 0.0132068629250331, -0.142878930085262	  },
					{-0.0327938281691465, -0.69498912567035, -0.0210543050866947, 0.538689454291165	  },
					{0.0510721096608351, 0.799500970922798, -0.0314043223261809, -0.954036175255462	  },
					{0.0562388851927469, -3.17906154574439, 0.0397186646063544, -0.188823760856662	  },
					{0.0504624917188252, -5.13416114192974, -0.0309550523275174, -1.34876609650143	  },
					{ 0.0197878541523585, 1.84374936727499, -0.02264145326154, 2.63397816596379		  },
					{ 0.00413471419182675, 0.971210011055863, 0.0120890248219109, 0.923634447783323	  },
					{ -0.00896273403146839, 1.86778498417809, -0.00701528830736807, 1.42360051605648  },
				}
			};

			ASSERT(me[H_TAIR].IsInit());

			TrClass Tr = GetTrClass(me[H_TAIR][MEAN]);
			size_t m = me.GetTRef().GetMonth();

			double hourly_sum = 0;
			double final_sum = 0;
			double max_prcp = 0;
			size_t max_prcp_h = 0;
			for (size_t h = 0; h < 24; h++)
			{
				me[h][H_PRCP] = 0;

				double h_prcp = daily_prcp * max(0.0, 1.0 / 24.0 + P[Tr][m][ALPHA1] * cos(2 * PI * (h / 24.0 - P[Tr][m][BETA1])) + P[Tr][m][ALPHA2] * cos(4 * PI * (h / 24.0 - P[Tr][m][BETA2])));
				hourly_sum += h_prcp;

				if (hourly_sum >= 0.1)
				{
					me[h][H_PRCP] = floor(10.0 * hourly_sum) / 10.0;//trunk to the nearest 0.1
					hourly_sum -= me[h][H_PRCP];
				}

				final_sum += me[h][H_PRCP];
				if (me[h][H_PRCP] > max_prcp)
				{
					max_prcp = me[h][H_PRCP];
					max_prcp_h = h;
				}

			}

			//adjust final hourly precipitation 
			double diff = Round(daily_prcp - final_sum, 1);
			me[max_prcp_h][H_PRCP] = max(0.0, me[max_prcp_h][H_PRCP] + diff);

		}
		else
		{
			for (size_t h = 0; h < 24; h++)
				me[h][H_PRCP] = 0;
		}

		//compute difference du to rounding and add it to first hour
#ifdef _DEBUG
		double daily_sum = 0;
		for (size_t h = 0; h < 24; h++)
			daily_sum += me[h][H_PRCP];
		ASSERT(fabs(daily_prcp - daily_sum) < 0.1);
#endif
	}

	//*****************************************************************************************
	//Humidity
	void CWeatherDay::ComputeHourlyTdew()
	{
		CWeatherDay& me = *this;
		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& dn = GetNext();

		if (!dp[H_TDEW].IsInit() || !me[H_TDEW].IsInit() || !dn[H_TDEW].IsInit())
			return;


		CStatistic stats[3] = { dp[H_TDEW], me[H_TDEW], dn[H_TDEW] };

		const CWeatherMonth* pMonth = static_cast<CWeatherMonth*>(GetParent());
		//double SRADmean = pMonth->GetStat(H_SRMJ)[MEAN];
		//double Kr = SRADmean > 8.64 ? 6 : 12;
		//always used Kr = 12 because SRAD is not always define. For stability
		static const double Kr = 12;

		for (size_t h = 0; h < 24; h++)
		{
			//*****************************************************************************************
			//Tdew
			double Td1 = h < 12 ? stats[0][MEAN] : stats[1][MEAN];
			double Td2 = h < 12 ? stats[1][MEAN] : stats[2][MEAN];

			//parameters are estimate from the mean of best Tdew and RH
			double Tdp = 0.761 * sin((h - 1.16) * PI / Kr - 3 * PI / 4);
			double moduloTd = double((h + 12) % 24);

			if (!IsMissing(me[h][H_TAIR]))
			{
				double Tdew = min((double)me[h][H_TAIR], Td1 + moduloTd / 24 * (Td2 - Td1) + Tdp);
				me[h][H_TDEW] = (float)Tdew;
			}
		}

	}

	//*****************************************************************************************
	//Relitive Humidity
	void CWeatherDay::ComputeHourlyRelH()
	{
		CWeatherDay& me = *this;

		if (me[H_TAIR].IsInit() && me[H_TDEW].IsInit())
		{
			for (size_t h = 0; h < 24; h++)
			{
				//*****************************************************************************************
				//RH from Tair and Tdew
				if (!WEATHER::IsMissing(me[h][H_TAIR]) && !WEATHER::IsMissing(me[h][H_TDEW]))
				{
					double RH = Td2Hr(me[h][H_TAIR], me[h][H_TDEW]);
					me[h][H_RELH] = (float)RH;
				}
			}
		}

	}

	//*****************************************************************************************
	//wind Speed


	//from: Diurnal and semidiurnal variations in global surface wind and divergence fields
	//		Aiguo Dai and Clara Deser
	//		National Center for Atmospheric Research, Boulder, Colorado

	double CWeatherDay::GetSn(size_t n, size_t h)
	{
		ASSERT(n >= 1 && n <= 2);
		ASSERT(h >= 0 && h < 24);


		//calibrated with Canada-USA 2010
		double m_a[2] = { -2.20524, 0.35608 };
		double m_b[2] = { -1.70917, 0.56297 };

		double t = 2 * PI * h / 24;
		return m_a[n - 1] * cos(n * t) + m_b[n - 1] * sin(n * t);

	}

	double CWeatherDay::GetS(size_t h)
	{
		return GetSn(1, h) + GetSn(2, h);
	}
	void CWeatherDay::ComputeHourlyWndS()
	{
		CWeatherDay& me = *this;
		//const CWeatherDay& dp = GetPrevious();
		//const CWeatherDay& dn = GetNext();
		//if (!dp[H_WNDS].IsInit() || !me[H_WNDS].IsInit() || !dn[H_WNDS].IsInit())
			//return;

		if (!me[H_WNDS].IsInit())
			return;



		//CStatistic stats[3] = { GetPrevious()[H_WNDS], me[H_WNDS], GetNext()[H_WNDS] };
		//ASSERT(stats[1].IsInit());

		static const double P[NB_TR_CALSS][12][NB_TR_PARAMS] =
		{
			{
				{-0.020187593, 0.996419151, 0.039595941, 1.064213352},
				{-0.049161813, 1.055798009, 0.051011291, 1.063370175},
				{-0.103153642, 1.082879939, 0.061623417, 1.080290678},
				{-0.153121077, 1.067248115, 0.054899512, 1.081878233},
				{-0.193328536, 1.080271801, 0.040718263, 1.087800450},
				{-0.228611790, 1.084095576, 0.042772458, 1.083241400},
				{-0.243511870, 1.086241625, 0.046582452, 1.086541162},
				{-0.218249001, 1.082525768, 0.062202171, 1.078983944},
				{-0.151786713, 1.071186382, 0.070230079, 1.069706245},
				{-0.087626779, 1.054884340, 0.062839398, 1.053244737},
				{-0.031819390, 1.053087610, 0.045347043, 1.041543568},
				{-0.025168486, 1.042880945, 0.040691816, 1.038616258},
			},
			{
				{-0.090411056, 1.141991882, 0.063846001, 1.057647376 },
				{-0.138847859, 1.113342257, 0.088159952, 1.072172612 },
				{-0.209279753, 1.113236627, 0.093900249, 1.083136686 },
				{-0.273753972, 1.103806915, 0.080628297, 1.081758605 },
				{-0.321886426, 1.098481230, 0.062108069, 1.082258282 },
				{-0.347237622, 1.095350415, 0.054004218, 1.092839099 },
				{-0.368158830, 1.104360496, 0.057035317, 1.094614498 },
				{-0.380471644, 1.103606759, 0.093798840, 1.088720736 },
				{-0.342484237, 1.089459149, 0.114957757, 1.072302927 },
				{-0.249799978, 1.080845495, 0.105907106, 1.055214374 },
				{-0.137830732, 1.091445363, 0.083515359, 1.049700428 },
				{-0.102835293, 1.103766910, 0.075336508, 1.054739066 },
			},
			{
				{ -0.207415091, 1.117613084, 0.093731266, 1.058443878},
				{ -0.226603835, 1.145349138, 0.090665762, 1.070828518},
				{ -0.283073800, 1.131687644, 0.089205812, 1.087004611},
				{ -0.347740061, 1.123935602, 0.069405205, 1.086386879},
				{ -0.387845987, 1.122182480, 0.048511060, 1.087708064 },
				{ -0.377891218, 1.129963044, 0.028325717, 2.112914317},
				{ -0.418711159, 1.132625395, 0.059165598, -1.891045366},
				{ -0.440842155, 1.129170764, 0.086623561, 0.599536253 },
				{ -0.431854367, 1.105368639, 0.138719118, 1.080846042 },
				{ -0.313573794, 1.084296701, 0.135335762, 1.068088477 },
				{ -0.239441216, 1.097117238, 0.119014421, 1.050401787 },
				{ -0.203696981, 1.132992018, 0.097977366, 1.046225403 },
			},
			{
				{ -0.364172397, 1.157222140, 0.09770804, 1.071392918	  },
				{ -0.380584626, 1.151741704, 0.105766676, 1.07874914  },
				{ -0.398110976, 1.161723874, 0.077987432, 1.091208285 },
				{ -0.447970461, 1.154649351, 0.064727596, 1.091237222 },
				{ -0.454959198, 1.143115233, -0.045642203, 1.356886685},
				{ -0.443397608, 1.162259384, -0.049457942, 0.907169871},
				{ -0.464645485, 0.178709703, -0.086097738, 0.901774767},
				{ -0.496486272, 0.176105439, -0.126241272, 0.390110242},
				{ -0.450448117, 1.160262722, -0.132905519, 1.356532459},
				{ -0.430148987, 1.132624896, 0.142257953, 1.07171873  },
				{ -0.437498464, 1.131869564, 0.142591794, 1.05709974  },
				{ -0.341164900, 1.149757034, 0.124803235, 1.070341045	  },
			}
		};


		TrClass Tr = GetTrClass(me[H_TAIR][MEAN]);
		size_t m = me.GetTRef().GetMonth();

		for (size_t h = 0; h < 24; h++)
		{
			double h_wnds = me[H_WNDS][MEAN] * max(0.0, 1.0 + P[Tr][m][ALPHA1] * cos(2 * PI * (h / 24.0 - P[Tr][m][BETA1])) + P[Tr][m][ALPHA2] * cos(4 * PI * (h / 24.0 - P[Tr][m][BETA2])));
			me[h][H_WNDS] = (float)Round(max(0.0, h_wnds),1); //in km/h
		}

	}

	//copy from WndS
	void CWeatherDay::ComputeHourlyWnd2()
	{
		CWeatherDay& me = *this;
		const CWeatherDay& dp = GetPrevious();
		const CWeatherDay& dn = GetNext();
		if (!dp[H_WND2].IsInit() || !me[H_WND2].IsInit() || !dn[H_WND2].IsInit())
			return;

		CStatistic stats[3] = { GetPrevious()[H_WND2], me[H_WND2], GetNext()[H_WND2] };
		ASSERT(stats[1].IsInit());


		for (size_t h = 0; h < 24; h++)
		{
			double w1 = h < 12 ? stats[0][MEAN] : stats[1][MEAN];
			double w2 = h < 12 ? stats[1][MEAN] : stats[2][MEAN];
			double moduloW = double((h + 12) % 24);
			double Wday = w1 + moduloW / 24 * (w2 - w1);
			double Wh = Wday + GetS(h);

			me[h][H_WND2] = (float)max(0.0, Wh); //in km/h

		}

	}

	void CWeatherDay::ComputeHourlyWndD()
	{
		CWeatherDay& me = *this;
		ASSERT(me[H_WNDD].IsInit());

		for (size_t h = 0; h < 24; h++)
			me[h][H_WNDD] = me[H_WNDD][MEAN];

	}
	//*****************************************************************************************
	//Solar radiation

	void CWeatherDay::ComputeHourlySRad()
	{
		CWeatherDay& me = *this;
		if (!me[H_SRAD].IsInit())
			return;

		const CLocation& loc = GetLocation();

		array<CStatistic, 24> hourlySolarAltitude;


		double δ = 23.45 * (PI / 180) * sin(2 * PI * (284 + GetTRef().GetJDay() + 1) / 365);
		double ϕ = Deg2Rad(loc.m_lat);

		for (size_t t = 0; t < 3600 * 24; t += 60)
		{
			size_t h = size_t(Round(double(t) / 3600.0)) % 24;//take centered on the hour
			double w = 2 * PI * (double(t) / 3600 - 12) / 24;
			double solarAltitude = max(0.0, sin(ϕ) * sin(δ) + cos(ϕ) * cos(δ) * cos(w));

			hourlySolarAltitude[h] += solarAltitude;
		}

		double sumSolarAltitude = 0;
		for (size_t h = 0; h < 24; h++)
			sumSolarAltitude += hourlySolarAltitude[h][MEAN];

		for (size_t h = 0; h < 24; h++)
		{
			me[h][H_SRAD] = 0;
			if (sumSolarAltitude > 0)
			{
				double r = hourlySolarAltitude[h][MEAN] / sumSolarAltitude;
				me[h][H_SRAD] = (float)(r * 24 * me[H_SRAD][MEAN]);//daily solar radiation is the mean of 24 hours
			}
		}

	}


	void CWeatherDay::ComputeHourlyPres()
	{
		CWeatherDay& me = *this;
		ASSERT(me[H_PRES].IsInit());

		double P[3] = { GetPrevious()[H_PRES][MEAN], me[H_PRES][MEAN], GetNext()[H_PRES][MEAN] };

		for (size_t h = 0; h < 24; h++)
		{
			//*****************************************************************************************
			//linear interpolation : it's biased, TODO: need to unbiaise the mean...
			double f[3] = { max(0.0, 12.0 - double(h)) / 12.0, h <= 12 ? (double(h) / 12.0) : ((24.0 - double(h)) / 12.0), max(0.0, double(h) - 12.0) / 12.0 };
			ASSERT(f[0] + f[1] + f[2] == 1);

			me[h][H_PRES] = f[0] * P[0] + f[1] * P[1] + f[2] * P[2];
		}

		//for (size_t h = 0; h<24; h++)
		//me[h][H_PRES] = me[H_PRES][MEAN];
	}


	//WARNING: IsHourlyAdjusted() must be call on weather before calling this method
	void CWeatherDay::ComputeHourlyVariables(CWVariables variables, std::string options)
	{
		ManageHourlyData();

		/*StringVector op(options, ",;|");

		map<string, string> ops;
		for (size_t i = 0; i < op.size(); i ++)
		{
			StringVector pair(options, "=:");
			ASSERT(pair.size() == 2);
			ops[pair[0]] = pair[1];
		}*/


		//size_t Tmethod = !ops["Tmethod"].empty() ? as<size_t>(ops["Tmethod"]) : HG_DOUBLE_SINE;// HG_SINE_EXP_SAVAGE;
		size_t Tmethod = HG_DOUBLE_SINE;

		CWeatherDay& me = *this;
		for (TVarH v = H_FIRST_VAR; v < variables.size(); v++)
		{
			if (variables[v])
			{
				ASSERT(me[v].IsInit() || (v == H_TAIR && me[H_TMIN].IsInit() && me[H_TMAX].IsInit()));

				switch (v)
				{
				case H_TMIN: break;
				case H_TAIR: ComputeHourlyTair(Tmethod); break;
				case H_TMAX: break;
				case H_PRCP: ComputeHourlyPrcp(); break;
				case H_TDEW: ComputeHourlyTdew(); break;
				case H_RELH: ComputeHourlyRelH();  break;
				case H_WNDS: ComputeHourlyWndS(); break;
				case H_WND2: ComputeHourlyWnd2(); break;
				case H_WNDD: ComputeHourlyWndD(); break;//to do
				case H_SRAD: ComputeHourlySRad(); break;
				case H_PRES: ComputeHourlyPres(); break;

				case H_SNOW://take daily value devide by 24
					for (size_t h = 0; h < 24; h++)
						at(h)[v] = me[v][MEAN] / 24;//to do ...
					break;

				case H_SNDH://take daily value 
				case H_SWE:
					for (size_t h = 0; h < 24; h++)
						at(h)[v] = me[v][MEAN];
					break;

					//case H_ES2://from hourly temperature
					//	for (size_t h = 0; h<24; h++)
					//		at(h)[v] = WBSF::eᵒ(at(h)[H_ES2]);
					//	break;

					//case H_EA2:
					//	for (size_t h = 0; h<24; h++)
					//		at(h)[v] = WBSF::eᵒ(at(h)[H_TDEW]);//compute from Tdew
					//	break;

					//case H_VPD2:
					//	for (size_t h = 0; h<24; h++)
					//		at(h)[v] = at(h)[H_ES] - at(h)[H_EA];
					//	break;

				default: assert(false);//other variables to do
				}

				ASSERT(v == H_TAIR || GetStat(v).IsInit());
			}
		}
	}

	double CWeatherDay::GetDayLength()const
	{
		return GetLocation().GetDayLength(GetTRef());
	}


	//****************************************************************************************************************
	CWeatherMonth& CWeatherMonth::operator=(const CWeatherMonth& in)
	{
		if (&in != this)
			C31Days::operator=(in);

		return *this;
	}

	void CWeatherMonth::Reset()
	{
		for (iterator it = begin(); it != end(); it++)
			it->Reset();

		m_stat.clear();
	}


	bool CWeatherMonth::operator==(const CWeatherMonth& in)const
	{
		return std::equal(begin(), end(), in.begin());
	}

	void CWeatherMonth::CompileStat(const CTPeriod& p)const
	{
		if (!m_stat.m_bInit || m_stat.m_period != p)
		{
			CWeatherMonth& me = const_cast<CWeatherMonth&>(*this);
			CTRef TRef = GetTRef();

			me.m_stat.clear();
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				for (size_t d = 0; d < size(); d++)
				{
					if (p.IsInside(CTRef(TRef.GetYear(), TRef.GetMonth(), d, 0, p.GetTM())))
					{
						me.m_stat[v] += at(d).GetStat(v);
					}
				}
			}
			for (TVarEx v = H_FIRST_VAR_EX; v < NB_VAR_ALL; v++)
			{
				for (size_t d = 0; d < size(); d++)
				{
					if (p.IsInside(CTRef(TRef.GetYear(), TRef.GetMonth(), d, 0, p.GetTM())))
					{
						me.m_stat[v] += at(d).GetVarEx(v);
					}
				}
			}

			me.m_stat.m_bInit = true;
			me.m_stat.m_period = p;
		}
	}



	CDailyWaveVector& CWeatherMonth::GetHourlyGeneration(CDailyWaveVector& t, size_t method, size_t step, double PolarDayLength, const COverheat& overheat) const
	{
		for (const_iterator it = begin(); it != end(); it++)
			it->GetHourlyGeneration(t, method, step, PolarDayLength, overheat);

		return t;
	}

	void CWeatherMonth::WriteStream(ostream& stream, const CWVariables& variable, bool /*asStat*/)const
	{
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				write_value(stream, m_stat[v]);
	}


	void CWeatherMonth::ReadStream(istream& stream, const CWVariables& variable, bool /*asStat*/)
	{
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				read_value(stream, m_stat[v]);

	}

	CStatistic CWeatherMonth::GetVarEx(HOURLY_DATA::TVarEx v)const
	{
		CStatistic stat;
		for (size_t i = 0; i < size(); i++)
			stat += at(i).GetVarEx(v);

		return stat;
	}

	//Fcd[In]	: Fcd of previous time step
	//Fcd[In]	: Fcd of current time step	
	double CWeatherMonth::GetNetRadiation(double& Fcd)const
	{
		CStatistic stat;
		for (size_t i = 0; i < size(); i++)
			stat += at(i).GetNetRadiation(Fcd);

		return stat[SUM]; //incoming radiation [MJ/m²]
	}

	void CWeatherMonth::ResetStat()
	{
		m_stat.clear();
	}

	//****************************************************************************************************************
	CWeatherYear& CWeatherYear::operator=(const CWeatherYear& in)
	{
		if (&in != this)
			C12Months::operator=(in);

		return *this;
	}

	void CWeatherYear::Reset()
	{
		for (iterator it = begin(); it != end(); it++)
			it->Reset();

		m_stat.clear();
	}



	void CWeatherYear::CompileStat(const CTPeriod& p)const
	{
		CWeatherDay day;

		if (!m_stat.m_bInit || m_stat.m_period != p)
		{
			CWeatherYear& me = const_cast<CWeatherYear&>(*this);

			me.m_stat.clear();
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				for (size_t m = 0; m < 12; m++)
				{
					me.m_stat[v] += at(m).GetStat(v, p);
				}
			}

			for (TVarEx v = H_FIRST_VAR_EX; v < NB_VAR_ALL; v++)
			{
				for (size_t m = 0; m < 12; m++)
				{
					me.m_stat[v] += at(m).GetStat(v, p);
				}
			}
			me.m_stat.m_bInit = true;
			me.m_stat.m_period = p;
		}
	}

	bool CWeatherYear::IsComplete(CWVariables variables)const
	{
		bool bCompte = true;

		CWVariablesCounter count = GetVariablesCount();
		size_t nbRefs = GetNbDays() * (IsHourly() ? 24 : 1);

		for (TVarH v = H_FIRST_VAR; v < NB_VAR_H && bCompte; v++)
		{
			assert(count[v].first <= nbRefs);
			if (variables[v] && count[v].first < nbRefs)
				bCompte = false;
		}

		return bCompte;
	}

	CDailyWaveVector& CWeatherYear::GetHourlyGeneration(CDailyWaveVector& t, size_t method, size_t step, double PolarDayLength, const COverheat& overheat) const
	{
		for (const_iterator it = begin(); it != end(); it++)
			it->GetHourlyGeneration(t, method, step, PolarDayLength, overheat);

		return t;
	}

	void CWeatherYear::WriteStream(ostream& stream, const CWVariables& variable, bool /*asStat*/)const
	{
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				write_value(stream, m_stat[v]);
	}


	void CWeatherYear::ReadStream(istream& stream, const CWVariables& variable, bool /*asStat*/)
	{
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				read_value(stream, m_stat[v]);

	}

	ERMsg CWeatherYear::SaveData(const std::string& filePath, CTM TM, char separator)const
	{
		ERMsg msg;

		if (!TM.IsInit())
			TM = GetTM();

		ofStream file;
		msg = file.open(filePath);


		if (msg)
		{
			CStatistic::SetVMiss(MISSING);
			CWVariables variable(GetVariables());

			CWeatherFormat format(TM, variable);//get default format

			string header = format.GetHeader() + "\n";
			file.write(header.c_str(), header.length());

			msg = SaveData(file, TM, format, separator);
		}

		return msg;
	}

	ERMsg CWeatherYear::SaveData(ostream& file, CTM TM, const CWeatherFormat& format, char separator)const
	{
		ERMsg msg;

		//write data
		const CWeatherYear* pYear = this;

		if (TM.Type() != CTM::ANNUAL)
		{
			std::string str[12];
			for (size_t m = 0; m < 12; m++)
			{
				CWeatherYear::const_iterator itM = pYear->begin() + m;

				if (TM.Type() != CTM::MONTHLY)
				{
					str[m].reserve(TM.Type() == CTM::HOURLY ? 24 * 100 : 100);


					for (CWeatherMonth::const_iterator itD = itM->begin(); itD != itM->end(); itD++)
					{
						if (TM.Type() == CTM::HOURLY)
						{
							for (CWeatherDay::const_iterator itH = itD->begin(); itH != itD->end(); itH++)
							{
								if (itH->HaveData())
								{
									//Write reference
									CTRef TRef = itH->GetTRef();
									str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1) + separator + ToString(TRef.GetHour());

									//Write variables
									for (size_t v = 0; v < format.size(); v++)
									{
										if (IsVariable(format[v].m_var))//&& format[v].m_var != H_TRNG
										{
											str[m] += separator;
											double value = itH->at(format[v].m_var);
											if (!IsMissing(value))
												str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", value);
											else
												str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
										}
									}

									str[m] += '\n';
								}
							}
						}
						else if (TM.Type() == CTM::DAILY)
						{
							if (itD->HaveData())
							{
								//Write reference
								CTRef TRef = itD->GetTRef();
								str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1);

								//Write variables
								for (size_t v = 0; v < format.size(); v++)
								{
									if (IsVariable(format[v].m_var)/* && format[v].m_var != H_TRNG*/)
									{
										str[m] += separator;

										CStatistic stat = itD->GetStat(TVarH(format[v].m_var));//Tair and Trng are transformed into Tmin and Tmax

										if (stat.IsInit())
											str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
										else
											str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
										//}
									}//id element is a variable
								}//for all element

								str[m] += '\n';
							}//have data
						}//time type : HOURLY or DAILY
					}//Day
				}
				else
				{
					//save month stats
					if (itM->HaveData())
					{
						//Write reference
						CTRef TRef = itM->GetTRef();
						str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1);

						//Write variables
						for (size_t v = 0; v < format.size(); v++)
						{
							if (format[v].m_var != H_SKIP)
							{
								str[m] += separator;

								CStatistic stat = itM->GetStat(format[v].m_var);
								if (stat.IsInit())
									str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
								else
									str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";


							}//id element is a variable
						}//for all element

						str[m] += '\n';
					}//have data
				}//MONTH format
			}//month

			for (size_t m = 0; m < 12; m++)
				if (!str[m].empty())
					file.write(str[m].c_str(), str[m].length());
		}
		else
		{
			string str;

			//save month stats
			if (pYear->HaveData())
			{
				//Write reference
				CTRef TRef = pYear->GetTRef();
				str += ToString(TRef.GetYear());

				//Write variables
				for (size_t v = 0; v < format.size(); v++)
				{
					if (format[v].m_var != H_SKIP)
					{
						str += separator;

						CStatistic stat = pYear->GetStat(format[v].m_var);
						if (stat.IsInit())
							str += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
						else
							str += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";

					}//id element is a variable
				}//for all element

				str += '\n';
				file.write(str.c_str(), str.length());
			}//have data
		}


		return msg;
	}


	CStatistic CWeatherYear::GetVarEx(HOURLY_DATA::TVarEx v)const
	{
		CStatistic stat;
		for (size_t i = 0; i < size(); i++)
			stat += at(i).GetVarEx(v);

		return stat;
	}

	//Fcd[In]	: Fcd of previous time step
	//Fcd[In]	: Fcd of current time step	
	double CWeatherYear::GetNetRadiation(double& Fcd)const
	{
		CStatistic stat;
		for (size_t i = 0; i < size(); i++)
			stat += at(i).GetNetRadiation(Fcd);

		return stat[SUM]; //incoming radiation [MJ/m²]
	}

	void CWeatherYear::ResetStat()
	{
		m_stat.clear();
		for (size_t i = 0; i < size(); i++)
			at(i).ResetStat();
	}

	//****************************************************************************************************************
	void CWeatherYears::clear()
	{
		CWeatherYearMap::clear();
		//m_bModified = false;
		m_bCompilingHourly = false;
		m_stat.clear();
		m_format.clear();//?? humm je réessais pour voir si ca cause encore une problème
		m_bHourlyComputed = false;
		//m_years.clear();
	}

	CWeatherYears::CWeatherYears(bool bIsHourly)
	{
		//m_bModified = false;
		m_pParent = NULL;
		m_bHourly = bIsHourly;
		m_bCompilingHourly = false;
		m_bHourlyComputed = false;
		//	m_hourlyGenerationMethod = HG_SINE_EXP_SAVAGE;
	}

	CWeatherYears::CWeatherYears(const CWeatherYears& in)
	{
		//m_bModified = false;
		m_pParent = NULL;
		operator=(in);
	}

	CWeatherYears& CWeatherYears::operator=(const CWeatherYears& in)
	{
		if (&in != this)
		{
			clear();
			m_bHourly = in.m_bHourly;
			m_format = in.m_format;
			for (const_iterator it = in.begin(); it != in.end(); it++)
			{
				CWeatherYear* pYear = new CWeatherYear;
				//CWeatherYearPtr pYear();
				pYear->Initialize(it->second->GetTRef(), this);
				*pYear = *(it->second);
				insert(make_pair(it->first, CWeatherYearPtr(pYear)));
			}

			//m_bModified = in.m_bModified;
			m_bCompilingHourly = in.m_bCompilingHourly;
			m_bHourlyComputed = in.m_bHourlyComputed;
		}

		return *this;
	}

	void CWeatherYears::Reset()
	{
		for (iterator it = begin(); it != end(); it++)
		{
			if (it->second)
				it->second->Reset();
		}

		m_stat.clear();
	}

	bool CWeatherYears::operator==(const CWeatherYears& in)const
	{
		const CWeatherYears& me = *this;

		bool bRep = true;

		if (m_bHourly != in.m_bHourly) bRep = false;
		if (size() != in.size())bRep = false;
		if (m_format != in.m_format)bRep = false;
		//if (m_bHourlyComputed != in.m_bHourlyComputed)equal = false; ????

		for (const_iterator it = in.begin(); it != in.end() && bRep; it++)
		{
			if (IsYearInit(it->first))
			{
				ASSERT(it->second.get());
				bRep = me[it->first] == *(it->second);
			}
			else
			{
				bRep = false;
			}
		}

		return bRep;
	}



	bool CWeatherYears::IsComplete(CWVariables variables, CTPeriod period)const
	{
		bool bComplete = false;

		if (!empty())
		{

			if (!period.IsInit())
				period = GetEntireTPeriod();

			bComplete = true;

			//for (const_iterator it = begin(); it != end() && bCompte; it++)
			for (int year = period.GetFirstYear(); year <= period.GetLastYear() && bComplete; year++)
			{
				bool test = IsYearInit(year);
				if (!IsYearInit(year) || !at(year).IsComplete(variables))
					bComplete = false;
			}
		}


		return bComplete;
	}


	void CWeatherYears::CleanUnusedVariable(CWVariables variables)
	{

		CStatistic EMPTY_STAT;

		for (iterator it = begin(); it != end(); it++)
		{
			CTPeriod p = it->second->GetEntireTPeriod();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
					if (!variables[v])
						Get(TRef).SetStat(v, EMPTY_STAT);
		}

	}

	void CWeatherYears::CompileStat(const CTPeriod& p)const
	{
		CWeatherDay day;

		if (!m_stat.m_bInit || m_stat.m_period != p)//|| bFoceCompile
		{
			CWeatherYears& me = const_cast<CWeatherYears&>(*this);

			me.m_stat.clear();

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				for (const_iterator it = begin(); it != end(); it++)
				{
					if (it->second->GetStat(v, p).IsInit())
					{
						switch (v)
						{
						case H_TMIN:
						case H_TAIR:
						case H_TMAX:
						case H_TDEW:
						case H_RELH:
						case H_WNDS:
						case H_WNDD:
						case H_PRES:
						case H_SNDH:
						case H_SWE:
						case H_WND2:
						case H_SRAD:
						case H_ADD1:
						case H_ADD2: me.m_stat[v] += it->second->GetStat(v, p)[MEAN]; break;
						case H_PRCP:
						case H_SNOW: me.m_stat[v] += it->second->GetStat(v, p)[SUM]; break;
						default: _ASSERTE(false);
						}
					}
				}
			}

			for (TVarEx v = H_FIRST_VAR_EX; v < NB_VAR_ALL; v++)
			{
				for (const_iterator it = begin(); it != end(); it++)
				{
					if (it->second->GetStat(v, p).IsInit())
						me.m_stat[v] += it->second->GetStat(v, p)[MEAN];
				}
			}

			me.m_stat.m_bInit = true;
			me.m_stat.m_period = p;
		}
	}

	CWeatherYears& CWeatherYears::append(const CWeatherYears& in)
	{
		if (&in != this && !in.empty())
		{
			insert(in.begin(), in.end());
		}

		return *this;
	}

	std::set<int> CWeatherYears::GetYears()const
	{
		std::set<int> years;
		for (const_iterator it = begin(); it != end(); it++)
			years.insert(it->first);

		return years;
	}

	CTRef CWeatherYears::GetLastTref(const std::string& filepath)
	{
		CTRef TRef;

		ifStream file;
		if (file.open(filepath))
		{
			string header;
			getline(file, header);
			CWeatherFormat format(header.c_str(), " \",;\t");
			size_t min_col = format.GetTM().Type() == CTM::HOURLY ? 4 : 3;

			std::string line = getLastLine(file);
			StringVector tmp(line, ",");
			if (tmp.size() >= min_col)
			{
				TRef = format.GetTRef(tmp);
				ASSERT(TRef.IsValid());
				if (!TRef.IsValid())
					TRef.clear();
			}

			file.close();
		}

		return TRef;
	}

	ERMsg CWeatherYears::SaveData(const std::string& filePath, CTM TM, char separator)const
	{
		assert(MISSING == -999);

		ERMsg msg;

		if (!TM.IsInit())
			TM = GetTM();

		ofStream file;
		msg = file.open(filePath);


		if (msg)
		{
			msg = SaveData(file, TM, separator);

		}//msg

		return msg;
	}

	ERMsg CWeatherYears::SaveData(ostream& file, CTM TM, char separator)const
	{
		assert(MISSING == -999);

		ERMsg msg;

		if (!TM.IsInit())
			TM = GetTM();

		CStatistic::SetVMiss(MISSING);
		CWVariables variable(GetVariables());

		CWeatherFormat format(TM, variable);//get default format
		return SaveData(file, TM, format, separator);
	}

	ERMsg CWeatherYears::SaveData(ostream& file, CTM TM, const CWeatherFormat& format, char separator)const
	{
		ERMsg msg;

		string header = format.GetHeader() + "\n";
		std::replace(header.begin(), header.end(), ',', separator);
		file.write(header.c_str(), header.length());

		//write data
		for (CWeatherYears::const_iterator itA = begin(); itA != end(); itA++)
		{
			const CWeatherYearPtr& ptr = itA->second;
			if (ptr)
			{
				const CWeatherYear* pYear = ptr.get();

				if (TM.Type() != CTM::ANNUAL)
				{
					std::string str[12];
					for (size_t m = 0; m < 12; m++)
					{
						CWeatherYear::const_iterator itM = pYear->begin() + m;

						if (TM.Type() != CTM::MONTHLY)
						{
							str[m].reserve(TM.Type() == CTM::HOURLY ? 24 * 100 : 100);


							for (CWeatherMonth::const_iterator itD = itM->begin(); itD != itM->end(); itD++)
							{
								if (TM.Type() == CTM::HOURLY)
								{
									ASSERT(itD->IsHourly());
									for (CWeatherDay::const_iterator itH = itD->begin(); itH != itD->end(); itH++)
									{
										if (itH->HaveData())
										{
											//Write reference
											CTRef TRef = itH->GetTRef();
											str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1) + separator + ToString(TRef.GetHour());

											//Write variables
											for (size_t v = 0; v < format.size(); v++)
											{
												if (IsVariable(format[v].m_var))//&& format[v].m_var != H_TRNG
												{
													str[m] += separator;
													double value = itH->at(format[v].m_var);
													if (!IsMissing(value))
														str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", value);//ToString(value, format[v].m_var<H_ADD1 ? 1 : 3);
													else
														str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
												}
											}

											str[m] += '\n';
										}
									}
								}
								else if (TM.Type() == CTM::DAILY)
								{
									if (itD->HaveData())
									{
										//Write reference
										CTRef TRef = itD->GetTRef();
										str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1) + separator + ToString(TRef.GetDay() + 1);

										//Write variables
										for (size_t v = 0; v < format.size(); v++)
										{
											if (IsVariable(format[v].m_var)/* && format[v].m_var != H_TRNG*/)
											{
												str[m] += separator;

												CStatistic stat = itD->GetStat(TVarH(format[v].m_var));

												if (stat.IsInit())
													str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
												else
													str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";
											}//id element is a variable
										}//for all element

										str[m] += '\n';
									}//have data
								}//time type : HOURLY or DAILY
							}//Day
						}
						else
						{
							//save month stats
							if (itM->HaveData())
							{
								//Write reference
								CTRef TRef = itM->GetTRef();
								str[m] += ToString(TRef.GetYear()) + separator + ToString(TRef.GetMonth() + 1);

								//Write variables
								for (size_t v = 0; v < format.size(); v++)
								{
									if (format[v].m_var != H_SKIP)
									{
										str[m] += separator;

										CStatistic stat = itM->GetStat(format[v].m_var);
										if (stat.IsInit())
											str[m] += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
										else
											str[m] += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";

									}//id element is a variable
								}//for all element

								str[m] += '\n';
							}//have data
						}//MONTH format
					}//month

					for (size_t m = 0; m < 12; m++)
						if (!str[m].empty())
							file.write(str[m].c_str(), str[m].length());
				}
				else
				{
					string str;

					//save month stats
					if (pYear->HaveData())
					{
						//Write reference
						CTRef TRef = pYear->GetTRef();
						str += ToString(TRef.GetYear());

						//Write variables
						for (size_t v = 0; v < format.size(); v++)
						{
							if (format[v].m_var != H_SKIP)
							{
								str += separator;

								CStatistic stat = pYear->GetStat(format[v].m_var);
								if (stat.IsInit())
									str += FormatA(format[v].m_var < H_ADD1 ? "%.1lf" : "%.3lf", stat[format[v].m_stat]);
								else
									str += format[v].m_var < H_ADD1 ? "-999.0" : "-999.000";

							}//id element is a variable
						}//for all element

						str += '\n';
						file.write(str.c_str(), str.length());
					}//have data
				}//ANNUAL format
			}//have data
		}//for all years

		return msg;
	}

	ERMsg CWeatherYears::Parse(const std::string& str, double nodata)
	{
		ERMsg msg;

		stringstream stream(str);

		string header;
		getline(stream, header);

		m_format.Set(header.c_str(), " \",;\t\r\n", nodata);
		if (m_format.GetTM().Type() != CTM::HOURLY && m_format.GetTM().Type() != CTM::DAILY)
		{
			msg.ajoute("Mandatory fields are \"Year\", \"Month\", \"Day\" (or \"JDay\") and \"Hour\"(When Hourly Data).");
			if (!m_format.GetUnknownFields().empty())
				msg.ajoute("Some fields are unknown (\"" + m_format.GetUnknownFields() + "\").");

			return msg;
		}

		CWeatherAccumulator accumulator(m_format.GetTM());
		SetHourly(m_format.GetTM().Type() == CTM::HOURLY);

		//load entire file
		int lineNo = 1;//header line is not considerate
		string line;

		while (msg && getline(stream, line))
		{
			Trim(line);
			if (!line.empty())
			{
				StringVector data = Tokenize(line, " ,;\t\r\n", true);
				if (!data.empty())
				{
					CTRef TRef = m_format.GetTRef(data);

					msg = accumulator.Add(data, m_format);
					if (msg)
					{
						Get(TRef).SetData(accumulator);
					}
					else
					{
						GetString(IDS_BSC_INVALID_TIME_REF);
						msg.ajoute(FormatMsg(IDS_BSC_INVALID_LINE, ToString(lineNo), line));
					}
				}

				lineNo++;
			}//empty line
		}//for all line

		return msg;
	}

	ERMsg CWeatherYears::LoadData(const std::string& filePath, double nodata, bool bResetContent, const CWeatherYearSectionMap& sectionToLoad)
	{
		ERMsg msg;

		if (bResetContent)
			clear();


		ifStream file;
		msg = file.open(filePath, ios::in | ios::binary);

		if (msg)
		{
			if (sectionToLoad.empty())
			{
				string str = file.GetText();
				msg = Parse(str, nodata);
			}
			else
			{
				string header;
				getline(file, header);

				//m_format.Set(header.c_str(), " ,;\t", nodata);
				m_format.Set(header.c_str(), " \",;\t", nodata);
				if (m_format.GetTM().Type() != CTM::HOURLY && m_format.GetTM().Type() != CTM::DAILY)
				{
					msg.ajoute(FormatMsg(IDS_BSC_INVALID_DATA_FILE, filePath));
					msg.ajoute(GetString(IDS_BSC_MENDATORY_FIELDS));

					if (!m_format.GetUnknownFields().empty())
						msg.ajoute(FormatMsg(IDS_BSC_UNKNOWN_FIELD, m_format.GetUnknownFields()));

					return msg;
				}

				CWeatherAccumulator accumulator(m_format.GetTM());
				SetHourly(m_format.GetTM().Type() == CTM::HOURLY);

				//load only some section
				for (CWeatherYearSectionMap::const_iterator it = sectionToLoad.begin(); it != sectionToLoad.end(); it++)
				{
					string::size_type count = string::size_type(it->second.m_end - it->second.m_begin);
					ASSERT(count > 0 && count < 10000000);

					int lineNo = it->second.m_lineNo;

					string str(count, 0);
					file.seekg(it->second.m_begin);
					file.read(&str[0], count);

					string::size_type begin = 0;
					while (msg && begin < count)
					{
						size_t end = GetNextLinePos(str, begin);
						StringVector data = Tokenize(str, " ,;\t\r\n", true, begin, end);
						if (!data.empty())
						{
							CTRef TRef = m_format.GetTRef(data);

							if (TRef.IsInit())
							{
								msg = accumulator.Add(data, m_format);
								if (msg)
								{
									//CreateYear(TRef);
									Get(TRef).SetData(accumulator);
								}
								else
								{
									string line = str.substr(begin, end - begin);
									msg.ajoute(FormatMsg(IDS_BSC_INVALID_LINE, ToString(lineNo), line));

								}
							}
							else
							{
								string line = str.substr(begin, end - begin);
								msg.ajoute(FormatMsg(IDS_BSC_INVALID_LINE, ToString(lineNo), line));
							}

							begin = end;
						}

						lineNo++;
					}//for all line
				}
			}

			file.close();
			//m_bModified = false;
		}

		return msg;
	}

	void CWeatherYears::WriteStream(ostream& stream, const CWVariables& variable, bool asStat)const
	{
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				write_value(stream, m_stat[v]);
	}


	void CWeatherYears::ReadStream(istream& stream, const CWVariables& variable, bool asStat)
	{
		for (size_t v = 0; v < variable.size(); v++)
			if (variable[v])
				read_value(stream, m_stat[v]);

	}

	CStatistic CWeatherYears::GetVarEx(HOURLY_DATA::TVarEx v)const
	{
		CStatistic stat;
		for (size_t i = 0; i < size(); i++)
			stat += at(i).GetVarEx(v);

		return stat;
	}

	//Fcd[In]	: Fcd of previous time step
	//Fcd[In]	: Fcd of current time step	
	double CWeatherYears::GetNetRadiation(double& Fcd)const
	{
		CStatistic stat;
		for (size_t i = 0; i < size(); i++)
			stat += at(i).GetNetRadiation(Fcd);

		return stat[SUM]; //incoming radiation [MJ/m²]
	}

	void CWeatherYears::ResetStat()
	{
		m_stat.clear();
		for (size_t i = 0; i < size(); i++)
			at(i).ResetStat();
	}


	void CWeatherYears::CompleteSnow()
	{
		CWeatherYears& me = *this;

		//CWVariablesCounter count = GetVariablesCount();
		CWVariables variables = GetVariables();
		if (variables[H_SNDH])
		{
			bool bHaveSnowData = false;
			size_t nbSnow = 0;

			CTPeriod p = GetEntireTPeriod(CTM::DAILY);
			size_t nbPeriod = p.GetNbYears() + 1;
			CTRef beginLastPeriod = p.Begin();
			CTRef endLastPeriod = CTRef(beginLastPeriod.GetYear(), JULY, DAY_15);


			for (size_t i = 0; i < nbPeriod; i++)
			{
				//write data
				for (CTRef d = beginLastPeriod; d <= endLastPeriod; d++)
				{
					if (me[d][H_SNDH].IsInit())
					{
						bHaveSnowData = true;

						if (me[d][H_SNDH][SUM] > 0)
							nbSnow++;
					}
				}


				if (bHaveSnowData)
				{
					if (nbSnow > 5)//suspicious data under 5 observations
					{
						double lastSnowVal = -999;

						for (CTRef d = beginLastPeriod; d <= endLastPeriod; d++)
						{
							if (me[d][H_SNDH].IsInit())
							{
								if (lastSnowVal > 10 && me[d][H_SNDH][SUM] == 0 && d < p.End() &&
									me[d + 1][H_SNDH].IsInit() && me[d + 1][H_SNDH][SUM]>10)
								{
									//bad zero zero easted of missing value
									me[d][H_SNDH].Reset();
								}

								lastSnowVal = me[d][H_SNDH].IsInit() ? me[d][H_SNDH][SUM] : -999;
							}
							else
							{

								if (!IsMissing(lastSnowVal))
								{
									if (me[d][H_PRCP][SUM] == 0 || me[d][H_SNOW][SUM] == 0 ||
										(lastSnowVal == 0 && me[d][H_TMIN].IsInit() && me[d][H_TMIN][MEAN] > 6))
									{
										me[d][H_SNDH] = lastSnowVal;
									}
									else
									{
										lastSnowVal = -999;
									}
								}
							}

							for (CTRef d = endLastPeriod; d >= beginLastPeriod; d--)
							{
								if (me[d][H_SNDH].IsInit())
								{
									lastSnowVal = me[d][H_SNDH][SUM];
								}
								else
								{

									if (!IsMissing(lastSnowVal))
									{
										if (me[d][H_PRCP][SUM] == 0 || me[d][H_SNOW][SUM] == 0 ||
											(lastSnowVal == 0 && me[d][H_TMIN].IsInit() && me[d][H_TMIN][MEAN] > 6))
										{
											me[d][H_SNDH] = lastSnowVal;
										}
										else
										{
											lastSnowVal = -999;
										}
									}
								}
							}
						}
					}
					else
					{
						for (CTRef d = beginLastPeriod; d <= endLastPeriod; d++)
							me[d][H_SNDH].Reset();
					}
				}

				bHaveSnowData = false;
				nbSnow = 0;
				beginLastPeriod = endLastPeriod + 1;
				endLastPeriod = min(p.End(), CTRef(beginLastPeriod.GetYear() + 1, JULY, DAY_15));

			}//for all period

			CWVariables variables = GetVariables();

			if (variables[H_SNDH])
			{

				bool bTest = false;
				for (CTRef d = p.Begin(); d <= p.End(); d++)
				{
					if (me[d][H_SNDH].IsInit() && me[d][H_SNDH][SUM] > 0)
					{
						bTest = true;
					}
				}

				assert(bTest);
			}
		}
	}


	bool CWeatherYears::ComputeHourlyVariables(CWVariables variables, std::string options)
	{
		ASSERT(GetWeatherStation());//Weather station must be define to compute hourly values

		CWeatherYears& me = *this;

		CWVariables vAvail = GetVariables();
		if (vAvail[H_TMIN] && vAvail[H_TMAX])
			vAvail.set(H_TAIR);

		variables &= vAvail;

		if (!variables[H_TDEW] && variables[H_TMIN] && variables[H_TMAX] && variables[H_RELH])
		{
			variables.set(H_TDEW);
			for (size_t y = 0; y < size(); y++)
			{
				for (size_t m = 0; m < me[y].size(); m++)
				{
					for (size_t d = 0; d < me[y][m].size(); d++)
					{
						if (!me[y][m][d][H_TDEW].IsInit() && me[y][m][d][H_TNTX].IsInit() && me[y][m][d][H_RELH].IsInit())
							me[y][m][d].SetStat(H_TDEW, Hr2Td(me[y][m][d][H_TNTX][MEAN], me[y][m][d][H_RELH][MEAN]));
					}
				}
			}
		}



		CWeatherStation copy(*GetWeatherStation());

		//((CLocation&) copy) = me.GetLocation();
		//((CWeatherYears&)copy) = me;//create daily weather


		copy.IsCompilingHourly();
		copy.SetHourly(true);

		//First estimate of hourly data
		for (size_t y = 0; y < copy.size(); y++)
		{
			for (size_t m = 0; m < copy[y].size(); m++)
			{
				for (size_t d = 0; d < copy[y][m].size(); d++)
				{
					copy[y][m][d].ComputeHourlyVariables(variables, options);
				}
			}
		}


		//adjust variables to get the same daily mean
		CWVariables variableToAdjust("TD H WS Z WS2");
		variableToAdjust &= variables;

		for (TVarH v = H_TDEW; v < variableToAdjust.size(); v++)
		{
			if (variableToAdjust[v])
			{
				for (size_t k = 0; k < 5; k++)
				{
					for (size_t y = 0; y < size(); y++)
					{
						for (size_t m = 0; m < me[y].size(); m++)
						{
							for (size_t d = 0; d < me[y][m].size(); d++)
							{

								_ASSERTE(me[y][m][d][v].IsInit());
								CStatistic oldStat = GetDailyStat(v, copy[y][m][d]);
								if (oldStat.IsInit())//only compute ajustement when data is available
								{

									copy[y][m][d].ComputeHourlyVariables(v, options);

									_ASSERTE(me[y][m][d][v].IsInit());
									CStatistic newStat = GetDailyStat(v, copy[y][m][d]);
									_ASSERTE(newStat);

									double delta = me[y][m][d][v][MEAN] - newStat[MEAN];
									copy[y][m][d][v] = max(GetLimitH(v, 0), min(GetLimitH(v, 1), oldStat[MEAN] + delta));
								}
							}
						}//v
					}//d
				}//m
			}//y
		}//k


		//Final calculation and copy original daily values
		SetHourly(true);
		for (size_t y = 0; y < size(); y++)
		{
			for (size_t m = 0; m < me[y].size(); m++)
			{
				for (size_t d = 0; d < me[y][m].size(); d++)
				{
					copy[y][m][d].ComputeHourlyVariables(variables, options);
					for (size_t h = 0; h < me[y][m][d].size(); h++)
					{
						for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
						{
							if (variables[v] && IsMissing(me[y][m][d][h][v]))
							{
								me[y][m][d][h][v] = copy[y][m][d][h][v];
								//if H_TMIN or H_TMAX is required, use H_TAIR: RSA 2020/01/31
								if (IsMissing(me[y][m][d][h][v]) && (v == H_TMIN || v == H_TMAX))
									me[y][m][d][h][v] = copy[y][m][d][h][H_TAIR];
							}
						}
					}
				}
			}
		}

		m_bHourlyComputed = true;

		return true;
	}

	//*****************************************************************************************************************
	CTRef CWeatherDay::GetTRef()const
	{
		ASSERT(m_TRef.GetTM() == CTM(CTRef::DAILY));
		return m_TRef;
	}

	CTRef CWeatherMonth::GetTRef()const
	{
		CTRef ref(at(0).GetTRef());
		ref.Transform(CTM(CTRef::MONTHLY));
		return ref;
	}

	CTRef CWeatherYear::GetTRef()const
	{
		CTRef ref(at(0).at(0).GetTRef());
		ref.Transform(CTM(CTRef::ANNUAL));
		return ref;
	}

	//*****************************************************************************************************************

	CWeatherStation::CWeatherStation(bool bIsHourly) :
		CWeatherYears(bIsHourly)
	{
		m_pAgent = NULL;
		CWeatherYears::Initialize(this);

		//m_bHourlyComputed = false;
	}

	CWeatherStation::CWeatherStation(const CWeatherStation& in)
	{
		m_pAgent = NULL;
		operator=(in);
	}

	void CWeatherStation::clear()
	{
		CLocation::clear();
		CWeatherYears::clear();
		//m_bModified = false;
		//m_bHourlyComputed = false;
	}




	CWeatherStation& CWeatherStation::operator=(const CWeatherStation& in)
	{
		if (&in != this)
		{
			CWeatherYears::Initialize(this);

			CLocation::operator=(in);
			CWeatherYears::operator=(in);
			m_pAgent = in.m_pAgent;
			m_hxGridSessionID = in.m_hxGridSessionID;
			//m_bHourlyComputed = in.m_bHourlyComputed;
		}


		ASSERT(in == *this);

		return *this;
	}

	bool CWeatherStation::operator==(const CWeatherStation& in)const
	{
		bool equal = true;
		if (CLocation::operator!=(in))equal = false;
		if (CWeatherYears::operator!=(in))equal = false;
		if (m_pAgent != in.m_pAgent)equal = false;
		if (m_hxGridSessionID != in.m_hxGridSessionID)equal = false;
		//if (m_bHourlyComputed != in.m_bHourlyComputed)equal = false;
		if (m_bHourly != in.m_bHourly)equal = false;

		return equal;
	}


	void CWeatherStationVector::GetMean(CWeatherStation& station, CTPeriod p, size_t mergeType)const
	{
		assert(p.IsAnnual());
		ASSERT(mergeType == MERGE_FROM_MEAN || size() <= 2);
		ASSERT(size() > 0);


		const CWeatherStationVector& me = *this;

		for (CTRef Tref = p.Begin(); Tref <= p.End(); Tref++)
		{
			int year = Tref.GetYear();
			if (station.IsYearInit(year))
			{

				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					CStatistic stat;
					for (size_t i = 0; i < size(); i++)
					{
						if (me[i].IsYearInit(year))
						{
							const CDataInterface& data = me[i][Tref];

							CStatistic stat2;
							if (data.GetStat(v, stat2))
							{

								bool exclude = stat.IsInit() && mergeType == MERGE_FROM_DB1;
								bool overwrite = stat.IsInit() && mergeType == MERGE_FROM_DB2;

								if (!exclude)
								{
									stat += stat2;
								}

								if (overwrite)
								{
									stat = stat2;
								}
							}
						}
					}//for all station

					if (stat.IsInit())
					{
						station[Tref].SetStat(v, stat[MEAN]);
					}
				}//all variables
			}//wanted year?
		}//all data
	}

	void CWeatherStation::FillGaps()
	{
		CWVariables variables = GetVariables();

		if (IsHourly())
		{
			CTPeriod p = GetEntireTPeriod();
			//bool bTair = variables[H_TMIN] || variables[H_TAIR] || variables[H_TMAX];

			////Fill Tmin, Tair and Tmax
			//if (bTair)
			//{
			//	for (CTRef TRef = p.Begin(); TRef != p.End(); TRef++)
			//	{
			//		CHourlyData& weaᵒ = GetHour(TRef);
			//		if (!IsMissing(weaᵒ[H_TAIR]))
			//		{
			//			if (IsMissing(weaᵒ[H_TMIN]))
			//				weaᵒ[H_TMIN] = weaᵒ[H_TAIR];
			//			if (IsMissing(weaᵒ[H_TMAX]))
			//				weaᵒ[H_TMAX] = weaᵒ[H_TAIR];
			//		}
			//		else
			//		{
			//			if (!IsMissing(weaᵒ[H_TMIN]) && !IsMissing(weaᵒ[H_TMAX]))
			//				weaᵒ[H_TAIR] = (weaᵒ[H_TMIN] + weaᵒ[H_TMIN]) / 2.0;
			//		}
			//		
			//	}
			//}

			for (CTRef TRef = p.Begin(); TRef != p.End(); TRef++)
			{
				CHourlyData& weaᵒ = GetHour(TRef);

				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables[v])
					{
						if (IsMissing(weaᵒ[v]))
						{
							const CHourlyData& wea¯¹ = weaᵒ.GetPrevious();
							if (!IsMissing(wea¯¹[v]))
							{
								CTRef TRef2 = TRef + 1;
								CHourlyData wea¹ = GetHour(TRef2);

								while (IsMissing(wea¹[v]) && TRef2 <= TRef + 6 && TRef2 <= p.End())//change by RSA(2017)
								{
									wea¹ = GetHour(TRef2);
									TRef2++;
								}

								if (!IsMissing(wea¹[v]))
								{
									ASSERT(TRef2 - TRef > 0);

									switch (v)
									{
									case H_PRCP: break;
									case H_SRAD: break;
									default:
										float mean = wea¯¹[v];
										float range = wea¹[v] - wea¯¹[v];

										for (CTRef TRef3 = TRef; TRef3 < TRef2; TRef3++)
										{
											double f = (double)(TRef3 - TRef + 1) / (TRef2 - TRef + 1);
											ASSERT(f >= 0 && f <= 1);

											CHourlyData& weaᵒ = GetHour(TRef3);
											weaᵒ.SetStat(v, mean + range * f);
										}
									}//switch
								}//if both exist
							}
						}
					}
				}
			}
		}
		else
		{
			CTPeriod p = GetEntireTPeriod();


			for (CTRef TRef = p.Begin(); TRef != p.End(); TRef++)
			{
				CDay& weaᵒ = GetDay(TRef);

				for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
				{
					if (variables[v])
					{
						if (!weaᵒ[v].IsInit())
						{
							const CDay& wea¯¹ = weaᵒ.GetPrevious();
							CDay wea¹ = weaᵒ.GetNext();

							if (wea¯¹[v].IsInit() && wea¹[v].IsInit())
							{
								switch (v)
								{
								case H_PRCP: break;
								case H_SRAD: break;
								default:
									CStatistic stat = wea¯¹[v];
									stat += wea¹[v];
									weaᵒ.SetStat(v, stat[MEAN]);
								}//switch

							}//if both exist
						}
					}
				}
			}
		}
	}

	void CWeatherStation::ApplyCorrections(const CWeatherCorrections& correction)
	{
		CWeatherStation& me = *this;

		CWVariables vars = GetVariables();
		CTPeriod p = GetEntireTPeriod();

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)
			{
				if (vars[v] && correction.m_variables[v])
				{
					if (me[TRef][v].IsInit())
					{
						if (v == H_PRCP)
						{
							ASSERT(!IsMissing(me[TRef][v][SUM]));
							double c = correction.GetCorrection(me, TRef, H_PRCP, TRef.GetYear());
							double prcp = me[TRef][v][SUM] * c;
							me[TRef].SetStat(v, prcp);
						}
						else
						{
							ASSERT(!IsMissing(me[TRef][v][MEAN]));
							double c = correction.GetCorrection(me, TRef, v, TRef.GetYear());
							double value = me[TRef][v][MEAN] + c;
							ASSERT(value > -99);

							me[TRef].SetStat(v, value);
						}
					}
				}
			}
		}
	}




	void CWeatherStation::WriteStream(ostream& stream, bool asStat)const
	{
		CStatistic::SetVMiss(MISSING);

		UINT64 version = 1;
		string locStr = zen::to_string(((CLocation&)(*this)), "Location", "1");
		CTPeriod p = GetEntireTPeriod();
		CWVariables variable(GetVariables());


		write_value(stream, version);
		//write_value(stream, asStat);
		WriteBuffer(stream, locStr);
		write_value(stream, p);
		write_value(stream, variable);

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			Get(TRef).WriteStream(stream, variable);
	}

	ERMsg CWeatherStation::ReadStream(istream& stream, bool asStat)
	{
		ERMsg msg;

		clear();

		UINT64 version = read_value<UINT64>(stream);
		if (version == 1)
		{
			string locStr;
			CTPeriod p;
			CWVariables variable;

			ReadBuffer(stream, locStr);
			read_value(stream, p);
			read_value(stream, variable);
			assert(p.IsInit());

			msg = zen::from_string(((CLocation&)(*this)), locStr);
			if (msg)
			{
				SetHourly(p.GetTM().Type() == CTM::HOURLY);
				//CreateYears(p);
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
				{
					Get(TRef).ReadStream(stream, variable);
				}
			}
		}
		else
		{
			msg.ajoute(GetString(IDS_BSC_ERROR_BAD_STREAM_FORMAT));
		}

		return msg;
	}


	typedef array<int, NB_PRIORITY_RULE + 1> PriorityRules;
	typedef vector<PriorityRules> PriorityRulesVector;

	class CByPriorityCriterious
	{
	public:

		CByPriorityCriterious(size_t priorityRules) :m_priorityRules(priorityRules)
		{}

		bool operator ()(const PriorityRules& in1, const PriorityRules& in2)const
		{
			if (m_priorityRules == OLDEST_YEAR)
			{
				if (in1[m_priorityRules] < in2[m_priorityRules])
					return true;
			}
			else
			{
				if (in1[m_priorityRules] > in2[m_priorityRules])
					return true;
			}

			if (in1[m_priorityRules] == in2[m_priorityRules])
			{
				if (in1[LARGEST_DATA] > in2[LARGEST_DATA])
					return true;
			}

			return false;
		}

	protected:


		size_t m_priorityRules;
	};

	void CWeatherStationVector::FillGaps()
	{
		//first fill gap
		for (CWeatherStationVector::iterator it = begin(); it != end(); it++)
			it->FillGaps();
	}

	void CWeatherStationVector::ApplyCorrections(const CWeatherCorrections& correction)
	{
		for (CWeatherStationVector::iterator it = begin(); it != end(); it++)
			it->ApplyCorrections(correction);
	}


	CWeightVector CWeatherStationVector::GetWeight(CWVariables variables, const CLocation& target, bool bTakeElevation, bool bTakeShoreDistance)const
	{
		//all the station have the same years
		assert(size() < 2 || at(0).GetFirstYear() == at(1).GetFirstYear());

		CWeightVector weight;
		if (!empty())
		{
			const CWeatherStationVector& me = *this;
			CTPeriod p = me.front().GetEntireTPeriod();

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)//for all variables
			{
				if (variables[v])//if selected variable
				{
					weight[v].resize(size());

					assert(p == me[0].GetEntireTPeriod());
					for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all time reference
					{
						CStatistic sumXtemp;

						for (size_t i = 0; i < size(); i++)
						{
							//CStatistic stat;
							//if (me[i][TRef].GetStat(v, stat))
							if (me[i][TRef][v].IsInit())
							{
								double Xtemp = target.GetXTemp(me[i], bTakeElevation, bTakeShoreDistance);
								//if (v == H_PRCP && me[i][TRef][v][SUM] < 0.1)//remove station without precipitation in the compution of the weight
									//Xtemp = 0;

								weight[v][i][TRef] = Xtemp;
								sumXtemp += Xtemp;
							}
						}

						if (sumXtemp.IsInit() && sumXtemp[SUM] > 0)
						{
							for (size_t i = 0; i < size(); i++)
								weight[v][i][TRef] /= sumXtemp[SUM];
						}
					}//for all station
				}//if variable selected 
			}//for all variable
		}//if not empty

		return weight;
	}


	void CWeatherStationVector::GetInverseDistanceMean(CWVariables variables, const CLocation& target, CWeatherStation& station, bool bTakeElevation, bool bTakeShoreDistance)const
	{
		((CLocation&)station) = target;

		if (!empty())
		{
			CWeightVector weight = GetWeight(variables, target, bTakeElevation, bTakeShoreDistance);

			const CWeatherStationVector& me = *this;
			CTPeriod p = GetEntireTPeriod();
			bool bIsHourly = p.GetTM().Type() == CTM::HOURLY;
			bool bIsDaily = p.GetTM().Type() == CTM::DAILY;
			station.SetHourly(bIsHourly);

			for (TVarH v = H_FIRST_VAR; v < NB_VAR_H; v++)//for all variables
			{
				if (variables[v])//if selected variable
				{
					assert(p == me[0].GetEntireTPeriod());
					for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all time reference
					{
						CStatistic stat;
						for (size_t i = 0; i < size(); i++)
						{
							CStatistic value;
							if (me[i][TRef].GetStat(v, value))
							{
								assert(value[NB_VALUE] == 1);
								assert(value[SUM] > -999);
								stat += value[MEAN] * weight[v][i][TRef];
							}
						}

						if (stat.IsInit())
						{
							assert(stat[SUM] > -999);
							double value = stat[SUM];
							if (v == H_PRCP)
							{
								if (value < 0)
									value = 0;

								value = Round(value, 1);
							}

							station[TRef].SetStat(v, value);
						}
						//}
					}//for all station
				}//if variable selected 
			}//for all variable
		}//if not empty
	}



	void CWeatherStationVector::MergeStation(CWeatherStation& station, CTM TM, size_t mergeType, size_t priorityRules, std::string& log)const
	{
		//if merge type not equal mean, we must have only 2 station
		ASSERT(mergeType == MERGE_FROM_MEAN || size() <= 2);

		station.clear();
		station.SetHourly(TM.Type() == CTM::HOURLY);
		if (!empty())
		{

			if (size() > 1)//merge of 1 station is not reported
			{
				PriorityRulesVector critVector;
				set<int> years;

				for (CWeatherStationVector::const_iterator it = begin(); it != end(); it++)
				{
					std::set<int> yr = it->GetYears();

					if (!yr.empty())
					{
						CWVariablesCounter var = it->GetVariablesCount();

						PriorityRules PR;
						PR[LARGEST_DATA] = (int)var.GetSum();
						PR[GREATEST_YEARS] = (int)it->GetVariablesCount().GetSum();
						PR[OLDEST_YEAR] = var.GetTPeriod().Begin().GetRef();
						PR[NEWEST_YEAR] = var.GetTPeriod().End().GetRef();
						PR[NB_PRIORITY_RULE] = (int)std::distance(begin(), it);

						critVector.push_back(PR);
						years.insert(yr.begin(), yr.end());
					}
				}

				ASSERT(!years.empty());
				if (!years.empty())
				{
					std::sort(critVector.begin(), critVector.end(), CByPriorityCriterious(priorityRules));
					size_t index = (size_t)critVector.begin()->at(NB_PRIORITY_RULE);

					((CLocation&)station) = at(index);
					for (CWeatherStationVector::const_iterator it = begin(); it != end(); it++)
						station.AppendMergeID(*it);

					for (set<int>::const_iterator it = years.begin(); it != years.end(); it++)
						station.CreateYear(*it);

					CTPeriod p = station.GetEntireTPeriod(TM);


					GetMean(station, p, mergeType);


					for (PriorityRulesVector::const_iterator it = critVector.begin(); it != critVector.end(); it++)
					{
						size_t index = it->at(NB_PRIORITY_RULE);

						if (it == critVector.begin())
							log += station.m_ID + "," + station.m_name + ",";
						else
							log += " , ,";

						log += at(index).m_ID + "," + at(index).m_name + "," + at(index).GetSSI("Source") + ",";

						CTRef TRef;
						//for (int i = 0; i < NB_PRIORITY_RULE; i++)
						log += ToString(it->at(0)) + ",";
						log += ToString(it->at(1)) + ",";
						TRef.SetRef(it->at(2), TM);
						log += TRef.GetFormatedString() + ",";
						TRef.SetRef(it->at(3), TM);
						log += TRef.GetFormatedString() + ",";

						double dist = at(index).GetDistance(station, false, false);
						double deltaElev = at(index).m_elev - station.m_elev;

						log += ToString(dist, 1) + "," + ToString(deltaElev, 1) + "\n";
					}

					station.SetSSI("Source", "Merged " + station.GetSSI("Source"));
				}
			}
			else
			{
				station = front();//only one station
			}
		}
	}

	bool CWeatherStationVector::IsComplete(CWVariables variables, CTPeriod period)const
	{
		bool bComplete = true;
		for (const_iterator it = begin(); it != end() && bComplete; it++)
			bComplete = it->IsComplete(variables, period);


		return bComplete;
	}

	void CWeatherStationVector::CleanUnusedVariable(CWVariables variables)
	{
		for (iterator it = begin(); it != end(); it++)
			it->CleanUnusedVariable(variables);
	}

	CDailyWaveVector& CWeatherYears::GetHourlyGeneration(CDailyWaveVector& t, size_t method, size_t step, double PolarDayLength, const COverheat& overheat) const
	{
		for (const_iterator it = begin(); it != end(); it++)
			it->second->GetHourlyGeneration(t, method, step, PolarDayLength, overheat);

		return t;
	}


}//namespace WBSF


