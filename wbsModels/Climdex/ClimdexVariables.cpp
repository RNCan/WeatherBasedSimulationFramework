//**********************************************************************
// 08/05/2017 	1.0.0   Rémi Saint-Amant	translate from Climdex FORTRAN code
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
							array<size_t, 12> value = { 0 };
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


	size_t CClimdexVariables::GetNumber(const CWeatherMonth& weather, size_t index)
	{
		CStatistic stat;
		size_t value = NOT_INIT;
		size_t m = weather.GetTRef().GetMonth();
		size_t nbDays = GetNbDayPerMonth(m);

		for (size_t d = 0; d < nbDays; d++)
		{
			switch (index)
			{
			case FD: value = weather[d][H_TMIN2].IsInit() ? (weather[d][H_TMIN2][MEAN] < 0) ? 1 : 0 : NOT_INIT; break;
			case SU: value = weather[d][H_TMAX2].IsInit() ? (weather[d][H_TMAX2][MEAN] > 25) ? 1 : 0 : NOT_INIT; break;
			case ID:value = weather[d][H_TMAX2].IsInit() ? (weather[d][H_TMAX2][MEAN] < 0) ? 1 : 0 : NOT_INIT; break;
			case TR:value = weather[d][H_TMIN2].IsInit() ? (weather[d][H_TMIN2][MEAN] > 20) ? 1 : 0 : NOT_INIT; break;
			default: ASSERT(false);
			}

			if (value != NOT_INIT)
				stat += value;
		}

		return stat[SUM];
	}
	// subroutines used by fclimdex_station, to calculate index: GSL.
	//
	// definitions:
	// GSL: Growing Season Length: Annual (1st Jan to 31st Dec in Northern Hemisphere (NH), 
	//  1st July to 30th June in Southern Hemisphere (SH)) count between first span of at least 6 days 
	//  with daily mean temperature TG>5 degree and first span after July 1st (Jan 1st in SH) of 6 days with TG<5 degree. 
	//
	// input

	size_t CClimdexVariables::GetGSL(const CWeatherYear& weather, std::array<size_t, 12> &gsl)
	{
		
		int year = weather.GetTRef().GetYear();

		//Init class member
		CGrowingSeason GS(CGSInfo::TT_TMEAN, 6, 5, CGSInfo::TT_TMEAN, 6, 5);

		CTPeriod period = GS.GetGrowingSeason(weather);
		ASSERT(period.GetTM() == CTM::DAILY);
		
		
		gsl.fill(0);
		for (size_t m = 0; m < 12; m++)
		{
			CTPeriod p(CTRef(year, m, FIRST_DAY), CTRef(year, m, LAST_DAY));
			CTPeriod i = period.Intersect(p);

			gsl[m] = i.GetLength();
			if (m == FEBRUARY)
				gsl[m] = min<size_t>(28, gsl[m]);
		}
		 

		return period.GetLength();
	}
	
	
	double CClimdexVariables::GetTXX(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6)? weather[H_TMAX2][HIGHEST] : -999; }
	double CClimdexVariables::GetTNX(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6)? weather[H_TMIN2][HIGHEST] : -999; }
	double CClimdexVariables::GetTXN(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6)? weather[H_TMAX2][LOWEST] : -999; }
	double CClimdexVariables::GetTNN(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6)? weather[H_TMIN2][LOWEST] : -999; }
	double CClimdexVariables::GetDTR(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TRNG2][NB_VALUE]<6) ? weather[H_TRNG2][MEAN] : -999; }
	double CClimdexVariables::GetRX1DAY(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= 3) ? weather[H_PRCP][HIGHEST] : -999; }
	double CClimdexVariables::GetPRCP(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= 3) ? weather[H_PRCP][SUM] : -999; }

	double CClimdexVariables::GetTXX(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6) ? weather[H_TMAX2][HIGHEST] : -999; }
	double CClimdexVariables::GetTNX(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6) ? weather[H_TMIN2][HIGHEST] : -999; }
	double CClimdexVariables::GetTXN(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6) ? weather[H_TMAX2][LOWEST] : -999; }
	double CClimdexVariables::GetTNN(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6) ? weather[H_TMIN2][LOWEST] : -999; }
	double CClimdexVariables::GetDTR(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TRNG2][NB_VALUE]<6) ? weather[H_TRNG2][MEAN] : -999; }
	double CClimdexVariables::GetRX1DAY(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= 15) ? weather[H_PRCP][HIGHEST] : -999; }
	double CClimdexVariables::GetPRCP(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] <= 15) ? weather[H_PRCP][SUM] : -999; }


	double CClimdexVariables::GetTP(const CWeatherMonth& weather, const CClimdexNormals& N, TPecentilVar v, TTPecentil p)
	{
		CStatistic stat;

		size_t m = weather.GetTRef().GetMonth();
		size_t nbDays = GetNbDayPerMonth(m);
		
		for (size_t d = 0; d < nbDays; d++)
		{
			double result = N.GetEventPercent(weather[d], v, p);

			if (result > -999)
				stat += result;
		}// for d

		
		// NOTE that the index allows at most 10 MISSING days in a month ////////////////////////////////////////////////////////
		double value = -999;
		if (nbDays - stat[NB_VALUE] < 10)   
		{
			value = stat[MEAN];
		}

		return value;
	}


	size_t CClimdexVariables::GetPreviousCnt(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar vv, TTPecentil p)
	{
		TVarH v = CClimdexNormals::VARIABLES[vv];
		int year = weather.GetTRef().GetYear();

		const CWeatherYears* pParent = static_cast<const CWeatherYears*>(weather.GetParent());
		ASSERT(pParent);

		if (!pParent->IsYearInit(year - 1))
			return 0;


		size_t cnt = 0;

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

	size_t CClimdexVariables::GetSDI(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar vv, TTPecentil p, std::array<size_t, 12> &sdi)
	{
		sdi.fill(0);

		TVarH v = CClimdexNormals::VARIABLES[vv];

		size_t cnt = GetPreviousCnt(weather, N, vv, p);
		size_t sdiSum = 0;

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
						sdi[m]+=6, sdiSum+=6;
					else if (cnt > 6)
						sdi[m]++, sdiSum++;
				}
				else
				{
					cnt = 0;
				}

			}// day
		}//month


		return sdiSum;
	}

	double CClimdexVariables::GetRX5DAY(const CWeatherMonth& weather)
	{
		double Rx5 = -999;
		size_t cnt = 0;
		
		if (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] > 3)//more than 3 missing
		{
			const CWeatherYear* pParent = (const CWeatherYear*)weather.GetParent();
			CTPeriod period = weather.GetEntireTPeriod(CTM::DAILY);

			if (period.Begin() > pParent->GetEntireTPeriod(CTM::DAILY).Begin())
				period.Begin() -= 4;

			for (CTRef TRef = period.Begin(); TRef <= period.End(); TRef++)
			{
				CStatistic stat;
				for (size_t dd = 0; dd < 5; dd++)
				{
					if (weather[TRef + dd][H_PRCP].IsInit())//&& weather[TRef][H_PRCP][SUM] >= 0.1
					{
						//cnt++;
						stat += weather[TRef][H_PRCP][SUM];
					}
				}

				if (stat.IsInit() && stat[SUM] > Rx5)
				{
					Rx5 = stat[SUM];
				}
			}//loop for day

			Rx5 = -999;
		}

		return Rx5;
	}
	
	double CClimdexVariables::GetRX5DAY_old(const CWeatherYear& weather)
	{


		//const CWeatherYear* pParent = (const CWeatherYear*)weather.GetParent();
		//CTPeriod period = weather.GetEntireTPeriod(CTM::DAILY);

		//if (period.Begin() > weather.GetEntireTPeriod(CTM::DAILY).Begin())
		//period.Begin() -= 4;

		int year = weather.GetTRef().GetYear();
		const CWeatherYears* pParent = static_cast<const CWeatherYears*>(weather.GetParent());
		ASSERT(pParent);

		
		double Rx5 = -999;
		size_t cnt = 0;
		CStatistic stat;

		/*if (pParent->IsYearInit(year - 1))
		{
			for (size_t d = 0; d < 4; d++)
			{
				const CWeatherDay& wDay = weather[DECEMBER][DAY_31 - 1];
				if (wDay[H_PRCP].IsInit() )
				{
					cnt++;
					stat += wDay[H_PRCP][SUM];
				}
				else
				{
					cnt = 0;
					stat.Reset();
				}
			}
		}*/

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++, cnt++)
			{
				//const CWeatherDay& wDay = weather[m][d];
				// get Rx5day
				if (cnt >= 5)   // corrected by Imke, original was 'cnt .gt. 5'  -- 2012.7.9
				{
					double r5prcp = 0;
					for (size_t dd = cnt-4; dd <= cnt; dd++)
					{
						const CWeatherDay& wDay = weather.GetDay(weather[m][d].GetTRef() - dd);
						if (wDay[H_PRCP].IsInit())
							r5prcp += wDay[H_PRCP][SUM];
					}

					stat += r5prcp;
				}
			}  // loop for day
		}     // loop for month

		// check if annual PRCP is MISSING
		// if yes, set annual index=MISSING
		if (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] > 15)
		{
			Rx5 = stat[HIGHEST];
		}
		else
		{
			Rx5 = -999;
		}


		return Rx5;
	}

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

	size_t CClimdexVariables::GetPreviousCD(const CWeatherYear& weatherIn, bool bWet)
	{
		int year = weatherIn.GetTRef().GetYear();
		const CWeatherYears* pParent = static_cast<const CWeatherYears*>(weatherIn.GetParent());
		ASSERT(pParent);

		if (!pParent->IsYearInit(year - 1))
			return 0;

		
		const CWeatherYear& weather = weatherIn.GetPrevious();

		size_t cnt=0;
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				const CWeatherDay& wDay = weather[DECEMBER - m][GetNbDayPerMonth(m) - d - 1];
				bool bTest = bWet ? wDay[H_PRCP][SUM] >= 0.1 : wDay[H_PRCP][SUM] < 0.1;

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
		return 365 + GetPreviousCD(weather, bWet);
	}

	size_t CClimdexVariables::GetNextCD(const CWeatherYear& weatherIn, bool bWet, size_t cnt)
	{
		int year = weatherIn.GetTRef().GetYear();
		const CWeatherYears* pParent = static_cast<const CWeatherYears*>(weatherIn.GetParent());
		ASSERT(pParent);

		if (!pParent->IsYearInit(year + 1))
			return 0;

		const CWeatherYear& weather = weatherIn.GetNext();
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				bool bTest = bWet ? weather[m][d][H_PRCP][SUM] >= 0.1 : weather[m][d][H_PRCP][SUM] < 0.1;

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

	size_t CClimdexVariables::GetCD(const CWeatherYear& weather, bool bWet, std::array<size_t, 12> &cd)
	{
		cd.fill(0);
	
		//get annual index
		size_t cnt = GetPreviousCD(weather, bWet);
		size_t cdA = cnt;
	
		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d=0; d < GetNbDayPerMonth(m); d++)
			{
				bool bTest = bWet ? weather[m][d][H_PRCP][SUM] >= 0.1 : weather[m][d][H_PRCP][SUM] < 0.1;
				if (weather[m][d][H_PRCP].IsInit() && bTest)
				{
					cnt++;
					if (cnt > cdA)
						cdA = cnt;

					if (cnt > cd[m])
						cd[m] = cnt;

					//int tmp = int(cnt);
					////put this value for actual and all previous months
					//for (size_t mm = m; mm < 12 && tmp > 0; mm--)
					//{
					//	if (cnt > cd[mm])
					//		cd[mm] = cnt;

					//	if (mm == m)
					//		tmp -= int(d + 1);
					//	else
					//		tmp -= int(GetNbDayPerMonth(mm));
					//}
						
				}
				else
				{
					//reset when missing
					cnt = 0;
				}
			}
		}// day


		cnt = GetNextCD(weather, bWet, cnt);
		if (cnt > cdA)
			cdA = cnt;

		//put this value for actual and all previous months
		if (cnt > cd[DECEMBER])
			cd[DECEMBER] = cnt;

		return cdA;
	}


	double CClimdexVariables::GetRp(const CWeatherMonth& weather, const CClimdexNormals& N, TPPecentil p)
	{
		CStatistic Rp;
		
		size_t m = weather.GetTRef().GetMonth();
		
		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
		{
			double threshold = N.GetPThreshold(m, p);
			if (weather[d][H_PRCP].IsInit() && weather[d][H_PRCP][SUM] >= 0.1)//take only wet day
				if (weather[d][H_PRCP][SUM] > threshold)
					Rp += weather[d][H_PRCP][SUM];
				//Rp += (weather[d][H_PRCP][SUM] > threshold) ? 100 : 0;

		}// day
		

		return Rp.IsInit()?Rp[SUM]:-999;
	}
	 
	double CClimdexVariables::Get(size_t v, const CWeatherYear& weather)
	{
		double value=-999;
		
		std::array<size_t, 12> junk; 
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
		case CDD: value = GetCDD(weather, junk); break;
		case CWD: value = GetCWD(weather, junk); break;

		default:
		{
			CStatistic stat;

			for (size_t m = 0; m < weather.size(); m++)
			{
				double val = Get(v, weather[m]);
				if (val > -999)
					stat += val;
			}

			size_t s = SUM;
			if (v == TN10 || v == TX10 || v==TN90 || v==TX90 || v==SDII )
				s = MEAN;
			else if(v == RX5D) 
				s = HIGHEST;

			value = stat[s];
		}
		}

		if (v == RX5D && weather.GetNbDays() - weather[H_PRCP][NB_VALUE] > 15)//more than 15 missing
			value = -999;


		return value;
	}

	double CClimdexVariables::Get(size_t v, const CWeatherMonth& weather)
	{
		//CStatistic stat;
		double value = -999;
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

		return value;
	}



	//void Execute(CWeatherStation& weather, CModelStatVector& output)
	//{
	//	output.Init(weather.GetEntireTPeriod(CTM(CTM::ANNUAL)), CClimdexVariables::NB_VARIABLES, -999);

	//	CModelStatVector DD5;
	//	CModelStatVector WD;

	//	GetSummerDD5(weather, DD5);
	//	GetWaterDeficit(weather, WD);

	//
	//	for (size_t y = 0; y < weather.size(); y++)
	//	{
	//		array < CTPeriod, NB_EXTREM> Q;	//quarter extrem
	//		array < size_t, NB_EXTREM> M;		//monthly extrem

	//		for (size_t e = 0; e < NB_EXTREM; e++)
	//			Q[e] = GetExtremQuarter(weather[y], TExtrem(e), false);

	//		for (size_t e = 0; e < NB_EXTREM; e++)
	//			M[e] = GetExtremMonth(weather[y], TExtrem(e));




	//		int year = weather.GetFirstYear() + int(y);
	//		for (size_t v = 0; v < CClimdexVariables::NB_VARIABLES; v++)
	//		{
	//			switch (v)
	//			{
	//			case V_TMIN_EXT:	output[y][v] = weather[y][H_TMIN2][MEAN]; break;
	//			case V_TMEAN:		output[y][v] = weather[y][H_TNTX][MEAN]; break;
	//			case V_TMAX_EXT:	output[y][v] = weather[y][H_TMAX2][MEAN]; break;
	//			case V_PRCP:		output[y][v] = weather[y](H_PRCP)[SUM]; break;
	//			case V_WARMQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[WARMEST])[MEAN]; break;
	//			case V_COLDQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[COLDEST])[MEAN]; break;
	//			case V_WETQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[WETTEST])[MEAN]; break;
	//			case V_DRYQ_TMEAN:	output[y][v] = weather[y](H_TNTX, Q[DRIEST])[MEAN]; break;
	//			case V_WARMQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[WARMEST])[SUM]; break;
	//			case V_COLDQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[COLDEST])[SUM]; break;
	//			case V_WETQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[WETTEST])[SUM]; break;
	//			case V_DRYQ_PRCP:	output[y][v] = weather[y](H_PRCP, Q[DRIEST])[SUM]; break;
	//			case V_AI:			output[y][v] = GetAridity(WD, CTPeriod(CTRef(year, FIRST_MONTH), CTRef(year, LAST_MONTH)), false); break;
	//			case V_WARMQ_AI:	output[y][v] = GetAridity(WD, Q[WARMEST], false); break;
	//			case V_COLDQ_AI:	output[y][v] = GetAridity(WD, Q[COLDEST], false); break;
	//			case V_WETQ_AI:		output[y][v] = GetAridity(WD, Q[WETTEST], false); break;
	//			case V_DRYQ_AI:		output[y][v] = GetAridity(WD, Q[DRIEST], false); break;
	//			case V_WARMM_TMEAN:	output[y][v] = weather[CTRef(year, M[WARMEST])][H_TNTX][MEAN]; break;
	//			case V_COLDM_TMEAN:	output[y][v] = weather[CTRef(year, M[COLDEST])][H_TNTX][MEAN]; break;
	//			case V_WETM_PRCP:	output[y][v] = weather[CTRef(year, M[WETTEST])][H_PRCP][SUM]; break;
	//			case V_DRYM_PRCP:	output[y][v] = weather[CTRef(year, M[DRIEST])][H_PRCP][SUM]; break;
	//			case V_SUMMER_DD5:	output[y][v] = DD5[y][0]; break;
	//			default: ASSERT(false);
	//			}
	//		}
	//	}
	//}

	
	//---------------------------------------------------------------------- -
	//Calculate threshold

	
	const TVarH CClimdexNormals::VARIABLES[NB_PERCENTILS_VAR] = { H_TMIN2, H_TMAX2 };

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


		for (size_t i = 0; i < m_Ppercentil.size(); i++)
			m_Ppercentil[i].fill(-999);

	}

	void CClimdexNormals::Compute(const CWeatherYears& weather)
	{
		ASSERT(m_period.GetNbYears() == 30);

		clear();

		//compute temperature percentil
		Tthreshold(weather, m_period, m_bUseBootstrap, m_Tpercentil);

		//compute precipitation percentil
		Pthreshold(weather, m_period, m_Ppercentil);
	}

	void CClimdexNormals::Tthreshold(const CWeatherYears& weather, CTPeriod period, bool bUseBootstrap, std::vector<std::vector<CTPercentil>>& Tpercentil)
	{
		ASSERT(period.GetNbYears() == 30);

		static const double PERCENTIL[NB_T_PERCENTILS] = { 10, 50, 90 };
		static const double NoMissingThreshold = 0.85;
		static const int WINSIZE = 5;
		static const int SS = int(WINSIZE / 2);
		
		int Icritical = int(weather.size()*WINSIZE*NoMissingThreshold);
		int firstYear = period.Begin().GetYear();
		size_t nbYears = bUseBootstrap ? 30 : 1;

		//extract values for all days
		array< array<array<array<float, NB_PERCENTILS_VAR>, 5>, 30>, 365> buffer;

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
			{
				size_t jd = WBSF::GetJDay(1999, m, d);
				for (size_t y = 0; y < 30; y++)
				{
					int year = firstYear + int(y);
					CTRef TRef = CTRef(year, m, d) - SS;
				
					for (size_t dd = 0; dd < 5; dd++, TRef++)
					{
						if (TRef.GetMonth()==FEBRUARY && TRef.GetDay() == DAY_29)
							TRef++;
					
						for (size_t vv = 0; vv < NB_PERCENTILS_VAR; vv++)
						{
							TVarH v = VARIABLES[vv];
							buffer[jd][y][dd][vv] = (weather[TRef][v].IsInit())?weather[TRef][v][MEAN]:-999;
						}//for all variables
					}//take consecutive 5 days
				}//for all years
			}//for all days
		}//for all months


		

		Tpercentil.resize(nbYears);
		//compute temperature percentil
		for (size_t i = 0; i < nbYears; i++)   // This part consumes most of the time, due to "threshold"...
		{
			Tpercentil[i].resize(nbYears);

//			int year = firstYear + int(i);
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


						//que faire dans le cas ou il n'y a pas assez de valeur...
						for (size_t vv = 0; vv < NB_PERCENTILS_VAR; vv++)
						{
							for (size_t p = 0; p < NB_T_PERCENTILS; p++)
								Tpercentil[i][j][jd][vv][p] = -999;

							if (stat[vv][NB_VALUE] > 0)//Icritical
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
							}//if critical<
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

	void CClimdexNormals::Pthreshold(const CWeatherYears& weather, CTPeriod period, CPPercentil& percentil)
	{
		static const double PERCENTIL[NB_P_PERCENTILS] = { 95, 99 };

		int firstYear = period.Begin().GetYear();


		//select non - MISSING data for each day covering all base years

		for (size_t m = 0; m < 12; m++)
		{
			CStatisticEx stat;

			for (size_t y = 0; y < period.GetNbYears(); y++)
			{
				int year = firstYear + int(y);
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					if (weather[year][m][d][H_PRCP].IsInit() && weather[year][m][d][H_PRCP][SUM] >= 0.1)//take only precipitation days
						stat += weather[year][m][d][H_PRCP][SUM];
				}
			}

			if (stat[NB_VALUE] > 0)
			{
				for (size_t p = 0; p < NB_P_PERCENTILS; p++)
				{
					double n = PERCENTIL[p] / 100.0 * stat.size();
					size_t n1 = size_t(floor(n));
					size_t n2 = min(n1 + 1, stat.size() - 1);
					double f = n - n1;
					ASSERT(f >= 0 && f <= 1);
					percentil[m][p] = (1 - f)*stat.get_sorted(n1) + f*stat.get_sorted(n2);
				}
			}
		}
	
	
	}//	end subroutine threshold


	double CClimdexNormals::GetEventPercent(const CWeatherDay& day, TPecentilVar vv, TTPecentil p)const
	{
		double results = -999;

		CTRef TRef = day.GetTRef();
		CTRef TRefNoLeap(1999, TRef.GetMonth(), TRef.GetDay() );
		size_t jd = TRefNoLeap.GetJDay();

		
		TVarH v = VARIABLES[vv];

		if (day[v].IsInit())       // for TX
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
						if (m_Tpercentil[i][j][jd][vv][p] != -999)
							stat += (f*day[v][MEAN] > f*m_Tpercentil[i][j][jd][vv][p]) ? 100 : 0;
					}
				}
			}
			else
			{
				if (m_Tpercentil[0][0][jd][vv][p] > -999)
					stat += (f*day[v][MEAN] > f*m_Tpercentil[0][0][jd][vv][p]) ? 100 : 0;
			}

			

			results = stat[MEAN];
		}

		return results;
	}
}