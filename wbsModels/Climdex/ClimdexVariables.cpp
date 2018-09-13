//**********************************************************************
// 08/05/2017 	1.0.0   Rémi Saint-Amant	inspired from climdex FORTRAN code (https://bitbucket.org/climdex/fortran/downloads/)
//**********************************************************************


#include <functional>   // std::greater
#include <algorithm>    // std::sort
#include "Basic/UtilMath.h"
#include "Basic/DegreeDays.h"
#include "Basic/Evapotranspiration.h"
#include "ClimdexVariables.h"
#include "Basic/GrowingSeason.h"
 
using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{

	static const size_t NB_MISSING_M = 3;
	static const size_t NB_MISSING_A = 15;


	CClimdexVariables::CClimdexVariables()
	{
		m_basePeriod = CTPeriod(CTRef(1961, JANUARY, DAY_01), CTRef(1990, DECEMBER, DAY_31));
		m_nn = 15;  //user define precipitation threshold;
		m_bUseBootstrap = true;
	}

	ERMsg CClimdexVariables::Execute(CTM TM, const CWeatherStation& weather, CModelStatVector& output)
	{
		ASSERT(TM.Type() == CTM::ANNUAL || TM.Type() == CTM::MONTHLY);
		ASSERT(weather.GetTM().Type() == CTM::DAILY); //use only daily period

		ERMsg msg;

		if (weather.GetEntireTPeriod().IsInside(m_basePeriod))
		{

			output.Init(weather.GetEntireTPeriod(TM), NB_VARIABLES, -999);

			m_N.m_period = m_basePeriod;
			m_N.m_bUseBootstrap = m_bUseBootstrap;
			m_N.Compute(weather);

			if (TM.Type() == CTM::MONTHLY)
			{
				for (size_t y = 0; y < weather.size(); y++)
				{
					for (size_t v = 0; v < CClimdexVariables::NB_VARIABLES; v++)
					{
						if (v == GSL || v == WSDI || v == CSDI || v == CDD || v == CWD)
						{
							TVarH vv = TVarH((v < RX1D) ? H_TNTX : H_PRCP);
							if (weather[y].GetNbDays() - weather[y][vv][NB_VALUE] <= NB_MISSING_A)
							{

								array<double, 12> value = { 0 };
								switch (v)
								{
								case GSL:	GetGSL(weather[y], value); break;
								case WSDI:  GetWSDI(weather[y], m_N, value); break;
								case CSDI:	GetCSDI(weather[y], m_N, value); break;
								case CDD:	GetCDD(weather[y], value); break;
								case CWD:	GetCWD(weather[y], value); break;
								default: ASSERT(false);
								}

								for (size_t m = 0; m < 12; m++)
									output[y * 12 + m][v] = value[m];
							}
						}
						else
						{
							for (size_t m = 0; m < 12; m++)
								output[y * 12 + m][v] = Get(v, weather[y][m]);
						}
					}
				}
			}
			else if (TM.Type() == CTM::ANNUAL)
			{
				for (size_t y = 0; y < weather.size(); y++)
				{
					for (size_t v = 0; v < CClimdexVariables::NB_VARIABLES; v++)
					{
						output[y][v] = Get(v, weather[y]);
					}
				}
			}
		}
		else
		{
			msg.ajoute("Base period is outside simulation period");
		}

		return msg;
	}

//******************************************************************************************************************************

	//TXx, Monthly maximum value of daily maximum temperature
	//TXn, Monthly minimum value of daily maximum temperature
	//TNx, Monthly maximum value of daily minimum temperature
	//TNn, Monthly minimum value of daily minimum temperature
	//DTR, Daily temperature range : Monthly mean difference between TX and TN
	//Rx1day, Monthly maximum 1 - day precipitation.
	//PRCPTOT: Annual total precipitation in wet days(RR ≥ 1.0mm).

	double CClimdexVariables::GetTXX(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMAX][NB_VALUE] <= NB_MISSING_M) ? weather[H_TMAX][HIGHEST] : -999; }
	double CClimdexVariables::GetTNX(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMIN][NB_VALUE] <= NB_MISSING_M) ? weather[H_TMIN][HIGHEST] : -999; }
	double CClimdexVariables::GetTXN(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMAX][NB_VALUE] <= NB_MISSING_M) ? weather[H_TMAX][LOWEST] : -999; }
	double CClimdexVariables::GetTNN(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMIN][NB_VALUE] <= NB_MISSING_M) ? weather[H_TMIN][LOWEST] : -999; }
	double CClimdexVariables::GetDTR(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TRNG2][NB_VALUE] <= NB_MISSING_M) ? weather[H_TRNG2][MEAN] : -999; }
	double CClimdexVariables::GetRX1DAY(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= NB_MISSING_M) ? weather[H_PRCP][HIGHEST] : -999; }
	double CClimdexVariables::GetPRCP(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= NB_MISSING_M) ? weather[H_PRCP][SUM] : -999; }

	double CClimdexVariables::GetTXX(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMAX][NB_VALUE] <= NB_MISSING_A) ? weather[H_TMAX][HIGHEST] : -999; }
	double CClimdexVariables::GetTNX(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMIN][NB_VALUE] <= NB_MISSING_A) ? weather[H_TMIN][HIGHEST] : -999; }
	double CClimdexVariables::GetTXN(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMAX][NB_VALUE] <= NB_MISSING_A) ? weather[H_TMAX][LOWEST] : -999; }
	double CClimdexVariables::GetTNN(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMIN][NB_VALUE] <= NB_MISSING_A) ? weather[H_TMIN][LOWEST] : -999; }
	double CClimdexVariables::GetDTR(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TRNG2][NB_VALUE] <= NB_MISSING_A) ? weather[H_TRNG2][MEAN] : -999; }
	double CClimdexVariables::GetRX1DAY(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= NB_MISSING_A) ? weather[H_PRCP][HIGHEST] : -999; }
	double CClimdexVariables::GetPRCP(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= NB_MISSING_A) ? weather[H_PRCP][SUM] : -999; }

	//definitions:
	//FD, Number of frost days : Annual count of days when TN(daily minimum temperature) < 0 degree.
	//SU, Number of summer days : Annual count of days when TX(daily maximum temperature) > 25 degree.
	//ID, Number of icing days : Annual count of days when TX(daily maximum temperature) < 0 degree.
	//TR, Number of tropical nights : Annual count of days when TN(daily minimum temperature) > 20 degree.

	double CClimdexVariables::GetNumber(const CWeatherDay& weather, size_t index)
	{
		double value = -999;
		switch (index)
		{
		case FD: value = weather[H_TMIN].IsInit() ? (weather[H_TMIN][LOWEST] < 0) ? 1 : 0 : -999; break;
		case SU: value = weather[H_TMAX].IsInit() ? (weather[H_TMAX][HIGHEST] > 25) ? 1 : 0 : -999; break;
		case ID:value = weather[H_TMAX].IsInit() ? (weather[H_TMAX][HIGHEST] < 0) ? 1 : 0 : -999; break;
		case TR:value = weather[H_TMIN].IsInit() ? (weather[H_TMIN][LOWEST] > 20) ? 1 : 0 : -999; break;
		default: ASSERT(false);
		}

		return value;
	}

	double CClimdexVariables::GetNumber(const CWeatherMonth& weather, size_t index)
	{
		double N = -999;

		
		CStatistic stat;
		
		size_t m = weather.GetTRef().GetMonth();

		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
		{
			double value = GetNumber(weather[d], index);

			if (value>-999)
				stat += value;
		}

		if(stat.IsInit())
			N = stat[SUM];
		

		return N;

	}

	
	// definitions:
	// GSL: Growing Season Length: Annual (1st Jan to 31st Dec in Northern Hemisphere (NH), 
	//  1st July to 30th June in Southern Hemisphere (SH)) count between first span of at least 6 days 
	//  with daily mean temperature TG>5 degree and first span after July 1st (Jan 1st in SH) of 6 days with TG<5 degree. 
	double CClimdexVariables::GetGSL(const CWeatherYear& weather, std::array<double, 12> &gsl)
	{
		if (weather.GetLocation().m_lat < 0 && !weather.HaveNext())             // South Hemisphere
			return -999;


		double GSL = 0;
		gsl.fill(0);

		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		int year = p.Begin().GetYear();
		CTRef mid = CTRef(year,JULY, DAY_01);
		

		if (weather.GetLocation().m_lat < 0)
		{
			p.Begin() = CTRef(year, JULY, FIRST_DAY);
			p.End() = CTRef(year+1, JUNE, LAST_DAY);
			mid = CTRef(year, JANUARY, DAY_01);
		}

		CTPeriod period;

		
		size_t hit = 0;
		for (CTRef TRef = p.Begin(); TRef < mid&&!period.Begin().IsInit(); TRef++)
		{
			if (weather[TRef][H_TNTX].IsInit() && weather[TRef][H_TNTX][MEAN] > 5)
				hit++;
			else
				hit = 0;

			if (hit == 6 )
				period.Begin() = TRef - 5;
		}

		
		hit = 0;
		for (CTRef TRef = mid; TRef <= p.End() && !period.End().IsInit(); TRef++)
		{
			if (weather[TRef][H_TNTX].IsInit() && weather[TRef][H_TNTX][MEAN] < 5)
				hit++;
			else
				hit = 0;

			if (hit == 6)
				period.End() = TRef - 5;
		}

		if (period.Begin().IsInit() && !period.End().IsInit())
			period.End() = p.End();

		if (period.IsInit())
		{
			GSL = period.GetLength();
		
			for (size_t m = 0; m < 12; m++)
			{
				CTPeriod p(CTRef(year, m, FIRST_DAY), CTRef(year, m, LAST_DAY));
				CTPeriod i = period.Intersect(p);

				gsl[m] = i.GetLength();
				if (m == FEBRUARY)
					gsl[m] = min<size_t>(28, gsl[m]);
			}
		}


		return GSL;
	}
	

	//TN10p: Percentage of days when TN < 10th percentile.
	//TX10p : Percentage of days when TX < 10th percentile.
	//TN90p : Percentage of days when TN > 90th percentile
	//TX90p: Percentage of days when TX > 90th percentile
	//To avoid possible inhomogeneity across the in - base and out - base periods,
	//the calculation for the base period requires the use of a bootstrap processure.
	//Details are described in Zhang et al. (2004).
	double CClimdexVariables::GetTP(const CWeatherMonth& weather, const CClimdexNormals& N, TPecentilVar v, TTPecentil p)
	{
		double value = -999;

		size_t m = weather.GetTRef().GetMonth();
		size_t nbDays = GetNbDayPerMonth(m);

		CStatistic stat;
		for (size_t d = 0; d < nbDays; d++)
		{
			double result = N.GetEventPercent(weather[d], v, p);

			if (result > -999)
				stat += result;
		}// for d


		// NOTE that the index allows at most 10 MISSING days in a month ////////////////////////////////////////////////////////
		if (nbDays - stat[NB_VALUE] < 10)
		{
			value = stat[MEAN];
		}

		return value;
	}


	size_t CClimdexVariables::GetPreviousCnt(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar vv, TTPecentil p)
	{
		if (!weather.HavePrevious())
			return 0;



		size_t cnt = 0;
		TVarH v = CClimdexNormals::VARIABLES[vv];


		for (size_t d = 0; d < 6; d++)
		{
			const CWeatherDay& wDay = weather[DECEMBER][DAY_31 - d];

			double f = p == P10 ? -1 : 1;
			double threshold = N.GetTThreshold(365 - d - 1, vv, p);
			
			bool bTest = f*wDay[v][MEAN] > f*threshold;
			if (wDay[v].IsInit() && bTest)
				cnt++;
			else
				d=6;//finish here
		}// day


		return cnt;

	}

	//WSDI: Warm speel duration index : Annual count of days with at least 6 consecutive days when TX > 90th percentile
	//CSDI: Cold speel duration index : Annual count of days with at least 6 consecutive days when TN < 10th percentile
	double CClimdexVariables::GetSDI(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar vv, TTPecentil p, std::array<double, 12> &sdi)
	{
		double SDI = 0;
		sdi.fill(0);

		TVarH v = CClimdexNormals::VARIABLES[vv];
		size_t cnt = GetPreviousCnt(weather, N, vv, p);

		size_t jd = 0;
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++, jd++)
			{
				double f = p == P10 ? -1 : 1;
				double threshold = N.GetTThreshold(jd, vv, p);
				if (weather[m][d][v].IsInit() && f*weather[m][d][v][MEAN] > f*threshold)
				{
					cnt++;
					if (cnt == 6)
						sdi[m] += 6, SDI += 6;
					else if (cnt > 6)
						sdi[m]++, SDI++;
				}
				else
				{
					cnt = 0;
				}
			}// day
		}//month

		return SDI;
	}


	
	//Rx5day, Monthly maximum consecutive 5 - day precipitation.
	double CClimdexVariables::GetRX5DAY(const CWeatherMonth& weather)
	{
		double Rx5 = -999;
		

		CTPeriod period = weather.GetEntireTPeriod(CTM::DAILY);

		if (weather.HavePrevious())
			period.Begin() -= 4; //start 4 day before the begginning

		//if (!weather.HaveNext())
			period.End() -= 4; 


		for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
		{
			CStatistic stat;
			for (size_t dd = 0; dd < 5; dd++)
			{
				//if (weather[TRef + dd][H_PRCP].IsInit())
				stat += weather[TRef+dd][H_PRCP];
			}

			if (stat.IsInit() && stat[SUM] > Rx5)
			{
				Rx5 = stat[SUM];
			}
		}//for all days of the month
	
		return Rx5;
	}
	
	double CClimdexVariables::GetRX5DAY_old(const CWeatherYear& weather)
	{
		
		double Rx5 = -999;
		
		// check if annual PRCP is MISSING
		// if yes, set annual index=MISSING
		if (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= NB_MISSING_A)
		{
			size_t cnt = 0;
			CStatistic stat;

			if (weather.HavePrevious())
			{
				const CWeatherYear& previous = weather.GetPrevious();
				for (size_t d = 0; d < 4; d++, cnt++)
				{
					const CWeatherDay& wDay = previous[DECEMBER][DAY_28 + d];
					stat += wDay[H_PRCP][SUM];
				}
			}

			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++, cnt++)
				{
					//const CWeatherDay& wDay = weather[m][d];
					// get Rx5day
					if (cnt >= 5)   // corrected by Imke, original was 'cnt .gt. 5'  -- 2012.7.9
					{
						double r5prcp = 0;
						for (size_t dd = cnt - 4; dd <= cnt; dd++)
						{
							const CWeatherDay& wDay = weather.GetDay(weather[m][d].GetTRef() - dd);
							if (wDay[H_PRCP].IsInit())
								r5prcp += wDay[H_PRCP][SUM];
						}

						stat += r5prcp;
					}
				}  // loop for day
			}     // loop for month


			Rx5 = stat[HIGHEST];

		}


		return Rx5;
	}

	//R10mm: Annual count of days when PRCP≥ 10mm.
	//R20mm : Annual count of days when PRCP≥ 20mm.
	//Rnnmm : Annual count of days when PRCP≥ nnmm, nn is a user defined threshold.
	//SDII : Simple pricipitation intensity index(on wet days, w PRCP ≥ 1mm).
	CStatistic CClimdexVariables::GetRnnmm(const CWeatherMonth& weather, double nn)
	{
		CStatistic stat;
		size_t m = weather.GetTRef().GetMonth();

		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
		{
			if (weather[d][H_PRCP].IsInit() && weather[d][H_PRCP][SUM] >= nn)
				stat += weather[d][H_PRCP][SUM];
		}

		return stat;
	}

	size_t CClimdexVariables::GetPreviousCD(const CWeatherYear& weather, bool bWet)
	{

		if (!weather.HavePrevious())
			return 0;

		
		const CWeatherYear& previous = weather.GetPrevious();

		size_t cnt=0;
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				const CWeatherDay& wDay = previous[DECEMBER - m][GetNbDayPerMonth(m) - d - 1];
				bool bTest = bWet ? wDay[H_PRCP][SUM] >= 1.0 : wDay[H_PRCP][SUM] < 1.0;

				if (wDay[H_PRCP].IsInit() && bTest)
				{
					cnt++;
				}
				else
				{
					return cnt;
				}
			}//day
		}//month


		//no event find for this years...???
		return 365 + GetPreviousCD(previous, bWet);
	}

	size_t CClimdexVariables::GetNextCD(const CWeatherYear& weatherIn, bool bWet, size_t cnt)
	{
		if (!weatherIn.HaveNext())
			return 0;

		const CWeatherYear& weather = weatherIn.GetNext();
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				bool bTest = bWet ? weather[m][d][H_PRCP][SUM] >= 1.0 : weather[m][d][H_PRCP][SUM] < 1.0;

				if (weather[m][d][H_PRCP].IsInit() && bTest)
				{
					cnt++;
				}
				else
				{
					return cnt;
				}
			}
		}// day


		//no event find for this years...???
		return GetNextCD(weather, bWet, cnt);
	}

	//CDD: Maximum length of dry spell, maximum number of consecutive days with RR < 1mm.
	//CWD : Maximum length of wet spell, maximum number of consecutive days with RR ≥ 1mm
	double CClimdexVariables::GetCD(const CWeatherYear& weather, bool bWet, std::array<double, 12> &cd)
	{
		double cdA = -999;
		cd.fill(-999);
	
		//get annual index
		size_t cnt = GetPreviousCD(weather, bWet);
	
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d=0; d < GetNbDayPerMonth(m); d++)
			{
				bool bTest = bWet ? weather[m][d][H_PRCP][SUM] >= 1.0 : weather[m][d][H_PRCP][SUM] < 1.0;
				if (weather[m][d][H_PRCP].IsInit() && bTest)
				{
					cnt++;
					if (cnt > cdA)
						cdA = cnt;

					if (cnt > cd[m])
						cd[m] = cnt;
				}
				else
				{
					//reset when missing
					cnt = 0;
				}
			}
		}// day

		//cnt = GetNextCD(weather, bWet, cnt);
		//if (cnt > cdA)
			//cdA = cnt;

		//put this value for actual and all previous months
		//if (cnt > cd[DECEMBER])
			//cd[DECEMBER] = cnt;

		return cdA;
	}

	//R95p: Annual total PRCP when RR > 95p (on a wet day RR ≥ 1.0mm).
	//R99p: Annual total PRCP when RR > 99p (on a wet day RR ≥ 1.0mm)
	double CClimdexVariables::GetRp(const CWeatherMonth& weather, const CClimdexNormals& N, TPPecentil p)
	{
		double Rp=-999;

		CStatistic stat;
		size_t m = weather.GetTRef().GetMonth();

		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
		{
			double threshold = N.GetPThreshold(m, p);
			if (weather[d][H_PRCP].IsInit() && weather[d][H_PRCP][SUM] >= 1.0)//take only wet day
			{
				if (weather[d][H_PRCP][SUM] > threshold)
					stat += weather[d][H_PRCP][SUM];
			}
		}// day

		if( stat.IsInit() )
			Rp = stat[SUM];


		return Rp;
	}
	double CClimdexVariables::GetRp(const CWeatherYear& weather, const CClimdexNormals& N, TPPecentil p)
	{
		double Rp = -999;


		CStatistic stat;

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				double threshold = N.GetPThreshold(p);
				if (weather[m][d][H_PRCP].IsInit() && weather[m][d][H_PRCP][SUM] >= 1.0)//take only wet day
					if (weather[m][d][H_PRCP][SUM] > threshold)
						stat += weather[m][d][H_PRCP][SUM];
			}// day
		}

		if (stat.IsInit())
			Rp = stat[SUM];


		return Rp;
	}
	 
	double CClimdexVariables::Get(size_t v, const CWeatherYear& weather, bool bForce)
	{
		double value = -999;

		TVarH vv = TVarH((v < RX1D) ? H_TNTX : H_PRCP);
		if (bForce || weather.GetNbDays() - weather[vv][NB_VALUE] <= NB_MISSING_A)
		{
			std::array<double, 12> junk;
			switch (v)
			{
			case GSL: value = GetGSL(weather, junk); break;
			case TXX: value = GetTXX(weather); break;
			case TNX: value = GetTNX(weather); break;
			case TXN: value = GetTXN(weather); break;
			case TNN: value = GetTNN(weather); break;
			case DTR: value = GetDTR(weather); break;
			case RX1D: value = GetRX1DAY(weather); break;
			case PRCP: value = GetPRCP(weather); break;
			case WSDI: value = GetWSDI(weather, m_N, junk); break;
			case CSDI: value = GetCSDI(weather, m_N, junk); break;
			case CDD:	value = GetCDD(weather, junk); break;
			case CWD:	value = GetCWD(weather, junk); break;
			case R95P:  value = GetR95p(weather, m_N); break;
			case R99P:  value = GetR99p(weather, m_N); break;

			default:
			{

				CStatistic stat;

				for (size_t m = 0; m < weather.size(); m++)
				{
					double val = Get(v, weather[m], true);
					if (val > -999)
						stat += val;
				}

				size_t s = SUM;
				if (v == TN10 || v == TX10 || v == TN90 || v == TX90 || v == SDII)
					s = MEAN;
				else if (v == RX5D)
					s = HIGHEST;

				if (stat.IsInit())
					value = stat[s];
			}
			}
		}
	
		return value;
	}

	double CClimdexVariables::Get(size_t v, const CWeatherMonth& weather, bool bForce)
	{
		double value = -999;

		
		if (v == TN10 || v == TX10 || v == TN90 || v == TX90)
			bForce = true;


		TVarH vv = TVarH((v < RX1D) ? H_TNTX : H_PRCP);
		if (bForce || weather.GetNbDays() - weather[vv][NB_VALUE] <= NB_MISSING_M)
		{
			switch (v)
			{
			case FD:	value = GetNumber(weather, v); break;
			case SU:	value = GetNumber(weather, v); break;
			case ID:	value = GetNumber(weather, v); break;
			case TR:	value = GetNumber(weather, v); break;
			case TXX:	value = GetTXX(weather); break;
			case TNX:	value = GetTNX(weather); break;
			case TXN:	value = GetTXN(weather); break;
			case TNN:	value = GetTNN(weather); break;
			case TN10: value = GetTN10p(weather, m_N); break;
			case TX10: value = GetTX10p(weather, m_N); break;
			case TN90: value = GetTN90p(weather, m_N); break;
			case TX90: value = GetTX90p(weather, m_N); break;
			case DTR:  value = GetDTR(weather); break;
			case RX1D: value = GetRX1DAY(weather); break;
			case RX5D: value = GetRX5DAY(weather); break;
			case SDII:  value = GetSDII(weather); break;
			case R10MM: value = GetR10mm(weather); break;
			case R20MM:	value = GetR20mm(weather); break;
			case RNNMM:	value = GetRnnmm(weather, m_nn)[NB_VALUE]; break;
			case CDD:	value = GetDTR(weather); break;
			case CWD:	value = GetDTR(weather); break;
			case R95P:  value = GetR95p(weather, m_N); break;
			case R99P:  value = GetR99p(weather, m_N); break;
			case PRCP:  value = GetPRCP(weather); break;
			default:	ASSERT(false);
			}
		}

		return value;
	}


	
	//---------------------------------------------------------------------- -
	//Calculate threshold

	
	const TVarH CClimdexNormals::VARIABLES[NB_PERCENTILS_VAR] = { H_TMIN, H_TMAX };

	CClimdexNormals::CClimdexNormals()
	{
		m_bUseBootstrap = true;
		//m_period = period;

		clear();
	}

	void CClimdexNormals::clear()
	{
		for (size_t i = 0; i < m_Tpercentil.size(); i++)
			for (size_t j = 0; j < m_Tpercentil[i].size(); j++)
				for (size_t k = 0; k < m_Tpercentil[i][j].size(); k++)
					for (size_t l = 0; l < m_Tpercentil[i][j][k].size(); l++)
						m_Tpercentil[i][j][k][l].fill(-999);


		m_PpercentilA.fill(-999);
		for (size_t m = 0; m < m_PpercentilM.size(); m++)
			m_PpercentilM[m].fill(-999);
	}

	void CClimdexNormals::Compute(const CWeatherYears& weather)
	{
		ASSERT(m_period.GetNbYears() == 30);

		clear();

		//compute temperature percentil
		Tthreshold(weather, m_period, m_bUseBootstrap, m_Tpercentil);

		//compute precipitation percentil
		Pthreshold(weather, m_period, m_PpercentilA, m_PpercentilM);
	}

	void CClimdexNormals::Tthreshold(const CWeatherYears& weather, CTPeriod period, bool bUseBootstrap, std::vector<std::vector<CTPercentil>>& Tpercentil)
	{
		ASSERT(period.GetNbYears() == 30);

		static const double PERCENTIL[NB_T_PERCENTILS] = { 10, 50, 90 };
		static const double NoMissingThreshold = 0.85;
		static const int WINSIZE = 5;
		static const int SS = int(WINSIZE / 2);
		
		int firstYear = period.Begin().GetYear();
		size_t nbYears = bUseBootstrap ? 30 : 1;
		int Icritical = int(30 * WINSIZE*NoMissingThreshold);

		//extract values for all days
		array< array<array<array<float, NB_PERCENTILS_VAR>, 5>, 30>, 365> buffer;

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				size_t jd = WBSF::GetJDay(m, d);
				for (size_t y = 0; y < 30; y++)
				{
					int year = firstYear + int(y);
					CTRef TRef = CTRef(year, m, d) - SS;
				
					for (size_t dd = 0; dd < 5; dd++, TRef++)
					{
						if (TRef.GetMonth()==FEBRUARY && TRef.GetDay() == DAY_29)
							TRef++;
					
						if (weather.IsYearInit(TRef.GetYear()))
						{
							for (size_t vv = 0; vv < NB_PERCENTILS_VAR; vv++)
							{
								TVarH v = VARIABLES[vv];
								buffer[jd][y][dd][vv] = (weather[TRef][v].IsInit()) ? weather[TRef][v][MEAN] : -999;
							}//for all variables
						}
					}//take consecutive 5 days
				}//for all years
			}//for all days
		}//for all months

		Tpercentil.resize(nbYears);
		//compute temperature percentil
		for (size_t i = 0; i < nbYears; i++)   
		{
			Tpercentil[i].resize(nbYears);

			for (size_t j = 0; j < nbYears; j++)
			{
				if (i == 0 || i!=j)
				{
					for (size_t jd = 0; jd < 365; jd++)
					{
						CStatisticEx stat[NB_PERCENTILS_VAR];
						for (size_t yy = 0; yy < 30; yy++)
						{
							size_t y = yy!=i?yy:j;
							

							for (size_t dd = 0; dd < 5; dd++)
							{
								for (size_t vv = 0; vv < NB_PERCENTILS_VAR; vv++)
								{
									if (buffer[jd][y][dd][vv] > -999)
										stat[vv] += buffer[jd][y][dd][vv];
								}//for all variables
							}//take consecutive 5 days
						}
						
						for (size_t vv = 0; vv < NB_PERCENTILS_VAR; vv++)
						{
							for (size_t p = 0; p < NB_T_PERCENTILS; p++)
								Tpercentil[i][j][jd][vv][p] = -999;

							if (stat[vv][NB_VALUE] >= Icritical)
							{
								for (size_t p = 0; p < NB_T_PERCENTILS; p++)
								{
									double n = PERCENTIL[p] / 100.0 * stat[vv].size();
									size_t n1 = size_t(floor(n));
									size_t n2 = min(n1 + 1, stat[vv].size() - 1);
									double f = n - n1;
									ASSERT(f >= 0 && f <= 1);

									double percentil = (1 - f)*stat[vv].get_sorted(n1) + f*stat[vv].get_sorted(n2);
									Tpercentil[i][j][jd][vv][p] = percentil;
									
								}//for all persentils
							}//if critical
						}
					}//for all jd
				}
				else //i == j
				{
					Tpercentil[i][j] = Tpercentil[0][0];
				}
			}//for j
		}//for i

	}//	end subroutine threshold

	void CClimdexNormals::Pthreshold(const CWeatherYears& weather, CTPeriod period, CPPercentil& percentilA, CPPercentilM& percentilM)
	{
		static const double PERCENTIL[NB_P_PERCENTILS] = { 95, 99 };

		int firstYear = period.Begin().GetYear();


		//select non - MISSING data for each day covering all base years
		CStatisticEx statA;

		for (size_t m = 0; m < 12; m++)
		{
			CStatisticEx stat;

			for (size_t y = 0; y < period.GetNbYears(); y++)
			{
				int year = firstYear + int(y);
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					if (weather[year][m][d][H_PRCP].IsInit() && weather[year][m][d][H_PRCP][SUM] >= 1.0)//take only precipitation days
						stat += weather[year][m][d][H_PRCP][SUM];
				}
			}

			if (stat.IsInit())
			{
				for (size_t p = 0; p < NB_P_PERCENTILS; p++)
				{
					double n = PERCENTIL[p] / 100.0 * stat.size();
					size_t n1 = size_t(floor(n));
					size_t n2 = min(n1 + 1, stat.size() - 1);
					double f = n - n1;
					ASSERT(f >= 0 && f <= 1);
					percentilM[m][p] = (1 - f)*stat.get_sorted(n1) + f*stat.get_sorted(n2);
				}
			}

			statA += stat;
		}
	
	
		if (statA.IsInit())
		{
			for (size_t p = 0; p < NB_P_PERCENTILS; p++)
			{
				double n = PERCENTIL[p] / 100.0 * statA.size();
				size_t n1 = size_t(floor(n));
				size_t n2 = min(n1 + 1, statA.size() - 1);
				double f = n - n1;
				ASSERT(f >= 0 && f <= 1);
				percentilA[p] = (1 - f)*statA.get_sorted(n1) + f*statA.get_sorted(n2);
			}
		}

	}//	end subroutine threshold


	double CClimdexNormals::GetEventPercent(const CWeatherDay& day, TPecentilVar vv, TTPecentil p)const
	{
		double results = -999;

		CTRef TRef = day.GetTRef();
		size_t jd = GetJDay(TRef.GetMonth(), TRef.GetDay());

		
		TVarH v = VARIABLES[vv];
		if (day[v].IsInit())   
		{
			bool bInside = m_period.IsInside(day.GetTRef());

			double f = p == P10 ? -1 : 1;//revers signe for p10

			CStatistic stat;
			if (m_bUseBootstrap && bInside)
			{
				size_t i = size_t(day.GetTRef().GetYear() - m_period.Begin().GetYear());
				ASSERT(i < 30);
				for (size_t j = 0; j < m_period.GetNbYears(); j++)       // inside base years
				{
					if (j != i )
					{
						if (m_Tpercentil[i][j][jd][vv][p] > -999)
							stat += (f*day[v][MEAN] > f*m_Tpercentil[i][j][jd][vv][p]) ? 100 : 0;
					}
				}
			}
			else
			{
				if (m_Tpercentil[0][0][jd][vv][p] > -999)
					stat += (f*day[v][MEAN] > f*m_Tpercentil[0][0][jd][vv][p]) ? 100 : 0;
			}

			
			if (stat.IsInit())
				results = stat[MEAN];
		}

		return results;
	}
}