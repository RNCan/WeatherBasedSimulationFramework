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

		ERMsg msg;


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
						array<size_t, 12> value;
						switch (v)
						{
						case GSL:	GetGSL(weather[y], value);
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
		//Init class member
		CGrowingSeason GS(CGSInfo::TT_TMEAN, 6, 5, CGSInfo::TT_TMEAN, 6, 5);

		CTPeriod p = GS.GetGrowingSeason(weather);
		p.Transform(CTM(CTM::DAILY));

		return p.GetLength();
	}

	double CClimdexVariables::GetTXX(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6)? weather[H_TMAX2][HIGHEST] : -999; }
	double CClimdexVariables::GetTNX(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6)? weather[H_TMIN2][HIGHEST] : -999; }
	double CClimdexVariables::GetTXN(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6)? weather[H_TMAX2][LOWEST] : -999; }
	double CClimdexVariables::GetTNN(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6)? weather[H_TMIN2][LOWEST] : -999; }
	double CClimdexVariables::GetDTR(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_TRNG2][NB_VALUE]<6) ? weather[H_TRNG2][MEAN] : -999; }
	double CClimdexVariables::GetRX1DAY(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] == 0) ? weather[H_PRCP][HIGHEST] : -999; }
	double CClimdexVariables::GetPRCP(const CWeatherMonth& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] == 0) ? weather[H_PRCP][SUM] : -999; }

	double CClimdexVariables::GetTXX(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6) ? weather[H_TMAX2][HIGHEST] : -999; }
	double CClimdexVariables::GetTNX(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6) ? weather[H_TMIN2][HIGHEST] : -999; }
	double CClimdexVariables::GetTXN(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMAX2][NB_VALUE]<6) ? weather[H_TMAX2][LOWEST] : -999; }
	double CClimdexVariables::GetTNN(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TMIN2][NB_VALUE]<6) ? weather[H_TMIN2][LOWEST] : -999; }
	double CClimdexVariables::GetDTR(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_TRNG2][NB_VALUE]<6) ? weather[H_TRNG2][MEAN] : -999; }
	double CClimdexVariables::GetRX1DAY(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] == 0) ? weather[H_PRCP][HIGHEST] : -999; }
	double CClimdexVariables::GetPRCP(const CWeatherYear& weather)	{ return (weather.GetNbDays() - weather[H_PRCP][NB_VALUE] == 0) ? weather[H_PRCP][SUM] : -999; }


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

		CTPeriod period = weather.GetPrevious().GetEntireTPeriod(CTM::DAILY);
		CTRef TRef = period.End();

		size_t m = DECEMBER;
		for (size_t d = 0; d < 6; d++, TRef--)
		{
			double f = p == P10 ? -1 : 1;
			double threshold = N.GetThreshold(365 - d - 1, vv, p);
			
			bool bTest = f*weather[TRef][v][MEAN] > f*threshold;
			if (weather[TRef][v].IsInit() && bTest)
				cnt++;
			else
				d=6;//finish here
		}// day


		return cnt;

	}

	size_t CClimdexVariables::GetSDI(const CWeatherYear& weather, const CClimdexNormals& N, TPecentilVar vv, TTPecentil p, std::array<size_t, 12> &sdi)
	{
		TVarH v = CClimdexNormals::VARIABLES[vv];

		size_t cnt = GetPreviousCnt(weather, N, vv, p);
		size_t sdiSum = 0;

		for (size_t m = 0; m < 12; m++)
		{
			for (size_t d = 0, jd=0; d < GetNbDayPerMonth(m); d++, jd++)
			{
				double f = p == P10 ? -1 : 1;
				double threshold = N.GetThreshold(jd, vv, p);
				if (weather[m][d][v].IsInit() && f*weather[m][d][v][MEAN] > f*threshold)
				{
					cnt++;
					if (cnt >= 6)
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
		return 0;
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
		return 365 + GetPreviousCD(weather, bWet);
	}

	size_t CClimdexVariables::GetNextCD(const CWeatherYear& weatherIn, bool bWet)
	{
		int year = weatherIn.GetTRef().GetYear();
		const CWeatherYears* pParent = static_cast<const CWeatherYears*>(weatherIn.GetParent());
		ASSERT(pParent);

		if (!pParent->IsYearInit(year + 1))
			return 0;


		const CWeatherYear& weather = weatherIn.GetNext();

		size_t cnt = 0;
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
		return 365 + GetNextCD(weather, bWet);
	}

	size_t CClimdexVariables::GetCD(const CWeatherYear& weather, bool bWet, std::array<size_t, 12> &cd)
	{
		//find previous event
		if( !weather.GetVariables()[H_PRCP] )
			return NOT_INIT;
	
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
					{
						cdA = cnt;
					
						int tmp = int(cnt);
						//put this value for actual and all previous months
						for (size_t mm = m; mm < 12 && tmp >= 0; mm--)
						{
							if (cnt > cd[mm])
								cd[mm] = cnt;
							
							if (mm == m)
								tmp -= int(d+1);
							else 
								tmp -= int(GetNbDayPerMonth(mm));
						}
							
					}
						
				}
				else
				{
					//reset when missing
					cnt = 0;
				}
			}
		}// day


		cnt += GetNextCD(weather, bWet);
		if (cnt > cdA)
		{
			cdA = cnt;

			int tmp = int(cnt);
			//put this value for actual and all previous months
			for (size_t mm = DECEMBER; mm < 12 && tmp >= 0; mm--)
			{
				if (cnt > cd[mm])
					cd[mm] = cnt;
				
				tmp -= int(GetNbDayPerMonth(mm));
			}

		}

		return cdA;
	}


	double CClimdexVariables::GetRp(const CWeatherMonth& weather, const CClimdexNormals& N, TPPecentil p)
	{
		CStatistic Rp;
		
		size_t m = weather.GetTRef().GetMonth();
		
		for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
		{

			//if (TRef.GetMonth() == FEBRUARY && TRef.GetDay() == DAY_29)
				//TRef++;//skip February 29

			double threshold = N.GetThreshold(m, p);
			if (weather[d][H_PRCP].IsInit() && weather[d][H_PRCP][SUM] >= 0.1)//take only wet day
				Rp += (weather[d][H_PRCP][SUM] > threshold) ? 100 : 0;

			//if (weather[TRef][H_PRCP].IsInit() && weather[TRef][H_PRCP][SUM] > threshold)
			//			{
			//			weather[TRef][H_PRCP][SUM];
			//	}
		}// day
		

		return Rp[MEAN];
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
		case WSDI:  value = GetWSDI(weather, m_N, junk); break;
		case CSDI:  value = GetCSDI(weather, m_N, junk); break;
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
			if (v == TN10 || v == TX10 || v==TN90 || v==TX90 || v==SDII || v==R95P || v== R99P)
				s = MEAN;
			else if(v == RX5D) 
				s = HIGHEST;

			value = stat[s];
		}
		}

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
		ASSERT(period.GetNbYears() == 30);

		clear();


		int firstYear = m_period.Begin().GetYear();

		size_t nbYears = m_bUseBootstrap ? 30 : 1;
		//compute temperature percentil
		for (size_t i = 0; i < nbYears; i++)   // This part consumes most of the time, due to "threshold"...
		{
			for (size_t j = 0; j < nbYears; j++)
			{
				Tthreshold(weather, m_period, firstYear + int(i), firstYear + int(j), m_Tpercentil[i][j]);
			}
		}

		//compute precipitation percentil
		Pthreshold(weather, m_period, m_Ppercentil);
	}

	void CClimdexNormals::Tthreshold(const CWeatherYears& weather, CTPeriod period, int yRemove, int yDuplicate, CTPercentil& percentil)
	{
		static const double PERCENTIL[NB_T_PERCENTILS] = { 10, 50, 90 };


		static const double NoMissingThreshold = 0.85;
		static const int WINSIZE = 5;
		static const int SS = int(WINSIZE / 2);
		
		int Icritical = int(weather.size()*WINSIZE*NoMissingThreshold);
		
		for (size_t vv = 0; vv < NB_PERCENTILS_VAR; vv++)
		{
			TVarH v = VARIABLES[vv];

			//for (size_t jd = 0; jd < 365; jd++)
			for (size_t m = 0; m < 12; m++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					CStatisticEx stat;
					//select non - MISSING data for each day covering all base years
					for (size_t y = 0; y < period.size(); y++)
					{
						int year = period[y].GetYear();
						if (year == yRemove)
							year = yDuplicate;

						CTRef baseTRef(year,m,d);
						for (CTRef TRef = baseTRef - SS; TRef <= baseTRef + SS; TRef++)
						{
							if (TRef.GetMonth()==FEBRUARY && TRef.GetDay() == DAY_29)
								TRef++;

							if (weather[TRef][v].IsInit())
								stat += weather[TRef][v][MEAN];
						}
					}

					//que faire dans le cas ou il n'y a pas assez de valeur...
					if (stat[NB_VALUE] > 0)//Icritical
					{
						for (size_t p = 0; p < NB_T_PERCENTILS; p++)
						{
							size_t n1 = max(size_t(1), size_t(ceil(PERCENTIL[p] / 100 * stat.size()))) - size_t(1);
							size_t n2 = min(n1 + 1, stat.size() - 1);
							double f = PERCENTIL[p] / 100 * stat.size() - floor(PERCENTIL[p] / 100 * stat.size());
							ASSERT(f >= 0 && f <= 1);
							
							double perceltil = (1 - f)*stat.get_sorted(n1) + f*stat.get_sorted(n2);
							percentil[GetJDay(1999, m, d)][vv][p] = perceltil;
						}//for all persentils
					}//if critical
				}//for all days
			}//for all months
		}//for all variables
	}//	end subroutine threshold

	void CClimdexNormals::Pthreshold(const CWeatherYears& weather, CTPeriod period, CPPercentil& percentil)
	{
		static const double PERCENTIL[NB_P_PERCENTILS] = { 90, 99 };

		//select non - MISSING data for each day covering all base years

		for (size_t m = 0; m < 12; m++)
		{
			CStatisticEx stat;

			for (size_t y = 0; y < period.GetNbYears(); y++)
			{
				for (size_t d = 0; d < GetNbDayPerMonth(m); d++)
				{
					if (weather[y][m][d][H_PRCP].IsInit())
						stat += weather[y][m][d][H_PRCP][SUM];
				}
			}
			if (stat[NB_VALUE] > 0)
			{
				for (size_t p = 0; p < NB_P_PERCENTILS; p++)
				{
					size_t n1 = max(size_t(1), size_t(ceil(PERCENTIL[p] / 100 * stat.size()))) - size_t(1);
					size_t n2 = min(n1 + 1, stat.size() - 1);
					double f = PERCENTIL[p] / 100 * stat.size() - floor(PERCENTIL[p] / 100 * stat.size());
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
			if (m_bUseBootstrap)
			{
				size_t i = size_t(day.GetTRef().GetYear() - m_period.Begin().GetYear());
				for (size_t j = 0; j < m_period.GetNbYears(); j++)       // inside base years
				{
					if (j == i && !bInside ||
						j != i && bInside)
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