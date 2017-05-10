//**********************************************************************
// 25/04/2017	1.0.0	Rémi Saint-Amant    Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "TminTairTmax.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	 

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CTminTairTmax::CreateObject);
	
	static size_t GetNbDayWithPrcp(const CWeatherYear& weather);
	static size_t GetNbDayWithPrcp(const CWeatherMonth& weather);
	static size_t GetNbFrostDay(const CWeatherYear& weather);
	static size_t GetNbFrostDay(const CWeatherMonth& weather);

	enum TAnnualStat{ ANNUAL_LOWEST_TMIN, ANNUAL_MEAN_TMIN, ANNUAL_MEAN_TMEAN, ANNUAL_MEAN_TMAX, ANNUAL_HIGHEST_TMAX, NB_ANNUAL_STATS };
	enum TMonthlyStat{ MONTHLY_LOWEST_TMIN, MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMEAN, MONTHLY_MEAN_TMAX, MONTHLY_HIGHEST_TMAX, NB_MONTHLY_STATS };
	enum TDailyStat{ DAILY_TMIN, DAILY_TAIR, DAILY_TMAX, NB_DAILY_STATS };
	enum THourlyStat{ HOURLY_TMIN, HOURLY_TAIR, HOURLY_TMAX, NB_HOURLY_STATS };

	//Contructor
	CTminTairTmax::CTminTairTmax()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;

		VERSION = "1.0.0 (2017)";
	}

	CTminTairTmax::~CTminTairTmax()
	{}

	//This method is call to load your parameter in your variable
	ERMsg CTminTairTmax::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		return msg;
	}

	
	ERMsg CTminTairTmax::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_STATS, -9999);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			m_output[y][ANNUAL_LOWEST_TMIN] = m_weather[y][H_TMIN2][LOWEST];
			m_output[y][ANNUAL_MEAN_TMIN]	= m_weather[y][H_TMIN2][MEAN];
			m_output[y][ANNUAL_MEAN_TMEAN]	= m_weather[y][H_TAIR2][MEAN];
			m_output[y][ANNUAL_MEAN_TMAX]	= m_weather[y][H_TMAX2][MEAN];
			m_output[y][ANNUAL_HIGHEST_TMAX] = m_weather[y][H_TMAX2][HIGHEST];
		}

		return msg;
	}

	ERMsg CTminTairTmax::OnExecuteMonthly()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_MONTHLY_STATS, -9999);

		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m<12; m++)
			{
				m_output[y * 12 + m][MONTHLY_LOWEST_TMIN] = m_weather[y][m][H_TMIN2][LOWEST];
				m_output[y * 12 + m][MONTHLY_MEAN_TMIN] = m_weather[y][m][H_TMIN2][MEAN];
				m_output[y * 12 + m][MONTHLY_MEAN_TMEAN] = m_weather[y][m][H_TAIR2][MEAN];
				m_output[y * 12 + m][MONTHLY_MEAN_TMAX] = m_weather[y][m][H_TMAX2][MEAN];
				m_output[y * 12 + m][MONTHLY_HIGHEST_TMAX] = m_weather[y][m][H_TMAX2][HIGHEST];
			}
		}


		return msg;
	}


	
	ERMsg CTminTairTmax::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_DAILY_STATS, -999);

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					const CWeatherDay& wDay = m_weather[y][m][d]; 

					CTRef ref = wDay.GetTRef(); 
					m_output[ref][DAILY_TMIN] = Round(wDay[H_TMIN2][LOWEST],1);
					m_output[ref][DAILY_TAIR] = Round(wDay[H_TAIR2][MEAN], 1);
					m_output[ref][DAILY_TMAX] = Round(wDay[H_TMAX2][HIGHEST], 1);
				}
			}
		}

		return msg;
	}

	ERMsg CTminTairTmax::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables(H_TAIR2);

		CTPeriod p = m_weather.GetEntireTPeriod();
		m_output.Init(p, NB_DAILY_STATS, -999);

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						const CHourlyData& data = m_weather[y][m][d][h];

						CTRef ref = data.GetTRef(); 
						m_output[ref][HOURLY_TMIN] = Round( data[H_TAIR2], 1);
						m_output[ref][HOURLY_TAIR] = Round( data[H_TAIR2], 1);
						m_output[ref][HOURLY_TMAX] = Round( data[H_TAIR2], 1);
					}
				}
			}
		}

		return msg;
	}

//
//
//	//simulated annaling 
//	void CTminTairTmax::AddSAResult(const StringVector& header, const StringVector& data)
//	{
//
//
//
//		if (header.size() == 12)
//		{
//			std::vector<double> obs(4);
//
//			CTRef TRef(ToShort(data[2]), ToShort(data[3]) - 1, ToShort(data[4]) - 1, ToShort(data[5]));
//			for (int i = 0; i < 4; i++)
//				obs[i] = ToDouble(data[i + 6]);
//
//
//			ASSERT(obs.size() == 4);
//			m_SAResult.push_back(CSAResult(TRef, obs));
//		}
//
//		/*if( header.size()==26)
//		{
//		std::vector<double> obs(24);
//
//		for(int h=0; h<24; h++)
//		obs[h] = data[h+2].ToDouble();
//
//
//		ASSERT( obs.size() == 24 );
//		m_SAResult.push_back( CSAResult(CTRef(), obs ) );
//		}
//		else if( header.size()==13)
//		{
//		std::vector<double> obs(7);
//
//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1,data[5].ToShort());
//		for(int c=0; c<7; c++)
//		obs[c] = data[c+6].ToDouble();
//
//
//		ASSERT( obs.size() == 7 );
//		m_SAResult.push_back( CSAResult(TRef, obs ) );
//		}
//		else if( header.size()==12)
//		{
//		std::vector<double> obs(7);
//
//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1);
//		for(int c=0; c<7; c++)
//		obs[c] = data[c+5].ToDouble();
//
//
//		ASSERT( obs.size() == 7 );
//		m_SAResult.push_back( CSAResult(TRef, obs ) );
//		}
//		else if( header.size()==11)
//		{
//		std::vector<double> obs(7);
//
//		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1);
//		for(int c=0; c<7; c++)
//		obs[c] = data[c+4].ToDouble();
//
//
//		ASSERT( obs.size() == 7 );
//		m_SAResult.push_back( CSAResult(TRef, obs ) );
//		}*/
//	}
//
//	void CTminTairTmax::GetFValueHourly(CStatisticXY& stat)
//	{
//		if (m_SAResult.size() > 0)
//		{
////			CHourlyStatVector data;
//////			GetHourlyStat(data);
////
////			for (size_t d = 0; d < m_SAResult.size(); d++)
////			{
////				if (m_SAResult[d].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[d].m_ref))
////				{
////					static const int HOURLY_TYPE[6] = { HOURLY_T, HOURLY_TDEW, HOURLY_REL_HUM, HOURLY_WIND_SPEED, HOURLY_SRAD };
////					double obs = m_SAResult[d].m_obs[m_varType];
////					double sim = data[m_SAResult[d].m_ref][HOURLY_TYPE[m_varType]];
////
////
////					//double test = data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM];
////					//CFL::RH2Td(data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM], data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM]);
////
////					if (!_isnan(sim) && !_isnan(obs) &&
////						_finite(sim) && _finite(obs))
////						stat.Add(sim, obs);
////				}
////
////				HxGridTestConnection();
////
////			}
//
//			/*
//					if( m_SAResult[0].m_obs.size() == 24 )
//					{
//					//CTRef TRef = data.GetFirstTRef();
//					//CStatistic statH[24];
//					//for(int i=0; i<data.size(); i++, TRef++)
//					//{
//					//	double v = data[i][m_varType];
//					//	statH[TRef.GetHour()]+=v;
//					//	HxGridTestConnection();
//					//}
//
//					//for(int y=0; y<m_weather.GetNbYear(); y++)
//					//{
//					//	double DD=0;
//					//	for(int m=0; m<12; m++)
//					//	{
//					//		for(int d=0; d<m_weather[y][m].GetNbDay(); d++)
//					//		{
//					//			const CWeatherDay& wDay = m_weather[y][m][d];
//					//			for(int h=0; h<24; h++)
//					//			{
//					//
//					//				//switch(m_varType)
//					//				//{
//					//				////case T_MN:
//					//				//case TDEW: v= Min( wDay.GetT(h), GetVarH(wDay, h, var));break;
//					//				//case RELH: v= Max(0, Min(100, GetVarH(wDay, h, var)));break;
//					//				//case WNDS: v = Max(0, GetVarH(wDay, h, var));break;
//					//				//}
//
//					//				statH[h]+=v;
//					//				HxGridTestConnection();
//					//			}
//					//		}
//					//	}
//					//}
//
//
//					//ASSERT( m_SAResult.size() == 1 );
//					//ASSERT( m_SAResult[0].m_obs.size() == 24 );
//					//for(int h=0; h<24; h++)
//					//{
//					//	stat.Add(statH[h][MEAN], m_SAResult[0].m_obs[h]);
//					//}
//					}
//					else if( m_SAResult[0].m_obs.size() == 7 )
//					{
//
//
//					for(size_t i=0; i<m_SAResult.size(); i++)
//					{
//
//					if( m_SAResult[i].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[i].m_ref))
//					{
//					double obs =  m_SAResult[i].m_obs[m_varType];
//					double sim = data[m_SAResult[i].m_ref][m_varType];
//					stat.Add(sim,obs);
//					}
//
//					HxGridTestConnection();
//
//					}
//					}
//					*/
//		}
//	}
//
//	void CTminTairTmax::GetFValueDaily(CStatisticXY& stat)
//	{
//
//		if (m_SAResult.size() > 0)
//		{
//			//OnExecuteDaily();
//			//const CDailyStatVector& data = (const CDailyStatVector&)GetOutput();
//
//			//for (size_t i = 0; i < m_SAResult.size(); i++)
//			//{
//
//			//	if (m_SAResult[i].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[i].m_ref))
//			//	{
//
//			//		static const int DAILY_TYPE[6] = { DAILY_TMIN, DAILY_TMAX, DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, DAILY_MEAN_WNDS, DAILY_MEAN_VPD };
//			//		double obs = m_SAResult[i].m_obs[m_varType];
//			//		double sim = data[m_SAResult[i].m_ref][DAILY_TYPE[m_varType]];
//			//		stat.Add(sim, obs);
//			//	}
//
//			//	HxGridTestConnection();
//
//			//}
//		}
//	}
//
//
//	void CTminTairTmax::GetFValueMonthly(CStatisticXY& stat)
//	{
//
//		if (m_SAResult.size() > 0)
//		{
//
//			//OnExecuteMonthly();
//			//const CMonthlyStatVector& data = (const CMonthlyStatVector&)GetOutput();
//
//			//for (size_t i = 0; i < m_SAResult.size(); i++)
//			//{
//
//			//	if (m_SAResult[i].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[i].m_ref))
//			//	{
//
//
//			//		static const int MONTHLY_TYPE[6] = { MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMAX, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_MEAN_WNDS, MONTHLY_MEAN_VPD };
//			//		double obs = m_SAResult[i].m_obs[m_varType];
//			//		double sim = data[m_SAResult[i].m_ref][MONTHLY_TYPE[m_varType]];
//
//
//
//
//
//			//		stat.Add(sim, obs);
//			//	}
//
//			//	HxGridTestConnection();
//
//			//}
//		}
//	}
//
//
//	//NOTE: Begin and END are ZERO-BASED Julian dates
//	//Source:
//	//Boughner, C.C. 1964. Distribution of growing degree days in Canada. 
//	//Can. Met. Memoirs 17. Met. Br., Dept. of Transport. 40 p.
//	//CPeriod GetGrowingSeason(CWeatherYear& weather)
//	//{
//	//	int day = 200; //(Mid-July)
//	//	bool frost = false;
//	//	CPeriod p(m_year, 0, GetLastDay());
//
//
//	//	//Beginning of the growing season
//	//	//look for the first occurrence of 3 successive days with frost
//	//	do
//	//	{
//
//	//		frost = GetDay(day).GetTMin() < 0 &&
//	//			GetDay(day - 1).GetTMin() < 0 &&
//	//			GetDay(day - 2).GetTMin() < 0;
//
//	//		day--;
//
//	//	} while (!frost && day > 1);
//
//
//	//	if (day>1)
//	//	{
//	//		p.Begin().SetJulianDay(day + 2);
//	//	}
//
//
//	//	//End of growing season
//	//	day = 200;
//	//	do
//	//	{
//	//		//look for the first occurrence of 3 successive days with frost
//	//		frost = GetDay(day).GetTMin() < 0 &&
//	//			GetDay(day + 1).GetTMin() < 0 &&
//	//			GetDay(day + 2).GetTMin() < 0;
//	//		day++;
//	//	} while (!frost && day < GetLastDay() - 2);
//
//	//	if (day<GetLastDay() - 2)
//	//	{
//	//		p.End().SetJulianDay(day - 2);
//	//	}
//
//	//	if (p.End() < p.Begin())
//	//	{
//	//		p.End() = p.Begin() = CDate(m_year, 200);
//	//	}
//
//	//	return p;
//	//}
//
//
//	//FrostFree period
//	//CPeriod GetFrostFreePeriod(CWeatherYear& weather)
//	//{
//	//	CPeriod pTmp;
//	//	CPeriod p;
//	//	CWeatherYear::InitializePeriod(pTmp);//Init year
//	//	//	CWeatherYear::InitializePeriod(p);
//
//	//	//int FFPeriod=0;
//	//	//int maxFFPeriod=0;
//	//	bool notInit = true;
//
//	//	int nbDay = GetNbDay();
//	//	for (int jd = 0; jd<nbDay; jd++)
//	//	{
//	//		if (GetDay(jd).GetTMin()>0) //Frost-free period begin or continues
//	//		{
//	//			if (notInit)
//	//			{
//	//				pTmp.Begin().SetJulianDay(jd);
//	//				notInit = false;
//	//			}
//	//		}
//	//		else
//	//		{
//	//			if (!notInit)
//	//			{
//	//				pTmp.End().SetJulianDay(jd);
//	//				notInit = true;
//
//	//				//Frost-free period ends
//	//				if (pTmp.GetLength() > p.GetLength())
//	//					p = pTmp;
//	//			}
//	//		}
//
//	//		if (jd == GetLastDay() && !notInit)
//	//		{
//	//			pTmp.End().SetJulianDay(jd);
//	//			if (pTmp.GetLength() > p.GetLength())
//	//				p = pTmp;
//	//		}
//	//	}
//
//
//
//	//	return p;
//	//}
//
//	//return Water Deficit en mm of wather
//	//double GetWaterDeficit(CWeatherYear& weather)
//	//{
//	//	//est-ce qu'on devrait utiliser cette équation à la place???
//	//	//return max( 0, GetStat(PET, SUM) -	GetStat(PPT, SUM));
//
//	//	CThornthwaitePET PET(*this, 0, CThornthwaitePET::POTENTIEL_STANDARD);
//	//	double A = 0;
//	//	//calculer Et pour le mois et A
//	//	for (int m = 0; m<12; m++)
//	//	{
//	//		if (m_months[m].GetStat(TMEAN, MEAN)>0.)
//	//		{
//	//			//precipitation in mm
//	//			double A_tmp = (PET.Get(m) - m_months[m].GetStat(PPT, SUM));
//	//			if (A_tmp>0.)
//	//				A += A_tmp;
//	//		}
//	//	}
//
//	//	return(A);
//	//}
//
//	size_t GetNbDayWithPrcp(const CWeatherYear& weather)
//	{
//		CStatistic stat=0;
//		for (size_t m = 0; m < weather.size(); m++)
//			stat += GetNbDayWithPrcp(weather[m]);
//
//		return stat[SUM];
//	}
//
//	size_t GetNbDayWithPrcp(const CWeatherMonth& weather)
//	{
//		size_t stat=0;
//		for (size_t d = 0; d < weather.size(); d++)
//			stat += (weather[d][H_PRCP][SUM] >= 0.2 ? 1 : 0);
//
//		return stat;
//	}
//	/*
//	double GetNbDayWithPrcp(const CWeatherDay& weather)
//	{
//		return (weather[H_PRCP][SUM]>=0.2?1:0);
//	}
//	*/
//	size_t GetNbFrostDay(const CWeatherYear& weather)
//	{
//		size_t stat = 0;
//		for (size_t m = 0; m < weather.size(); m++)
//			stat += GetNbFrostDay(weather[m]);
//
//		return stat;
//	}
//
//	size_t GetNbFrostDay(const CWeatherMonth& weather)
//	{
//		size_t stat = 0;
//		for (size_t d = 0; d < weather.size(); d++)
//			stat += (weather[d][H_TMIN2][LOWEST] <= 0 ? 1 : 0);
//
//		return stat;
//	}
//
//	/*
//	double GetNbFrostDay(const CWeatherDay& weather)
//	{
//		return (weather[H_TMIN2][LOWEST] <= 0 ? 1 : 0);
//	}
//*/
	
}