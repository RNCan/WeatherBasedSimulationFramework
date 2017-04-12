//**********************************************************************
// 11/04/2017	3.1.1	Rémi Saint-Amant    Recompile
// 20/09/2016	3.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	3.0.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 27/11/2014			Rémi Saint-Amant	Compiled 64 bits with new framework
// 05/04/2013			Rémi Saint-Amant	Remove DD and ET from this model
// 26/02/2013			Rémi Saint-Amant	Add hourly model
// 29/06/2010			Rémi Saint-Amant	Compatible with HxGrid. Remove extrem
// 30/10/2009			Rémi Saint-Amant	Change CPeriod by CTPeriod
// 03/03/2009			Rémi Saint-Amant	Integrate with new BioSIMModelBase (hxGrid)
// 19/11/2008			Rémi Saint-Amant	Update with VS9 and new BioSIMModelBase 
// 27/05/2008			Rémi Saint-Amant	Used of wind speed in the computation of ASC2000 PET
// 01/12/2002			Rémi Saint-Amant	2 variables was added: Degree-day and % of snow
// 15/07/2002			Rémi Saint-Amant	Creation
//**********************************************************************

#include <iostream>
#include "Basic/Evapotranspiration.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"
#include "Climatic.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CClimaticModel::CreateObject);
	
	static size_t GetNbDayWithPrcp(const CWeatherYear& weather);
	static size_t GetNbDayWithPrcp(const CWeatherMonth& weather);
	static size_t GetNbFrostDay(const CWeatherYear& weather);
	static size_t GetNbFrostDay(const CWeatherMonth& weather);
	


	enum TAnnualStat{ ANNUAL_LOWEST_TMIN, ANNUAL_MEAN_TMIN, ANNUAL_MEAN_TMEAN, ANNUAL_MEAN_TMAX, ANNUAL_HIGHEST_TMAX, ANNUAL_PPT, ANNUAL_MEAN_TDEW, ANNUAL_MEAN_REL_HUM, ANNUAL_MEAN_WNDS, ANNUAL_FROST_DAY, ANNUAL_FROSTFREE_DAY, ANNUAL_WET_DAY, ANNUAL_DRY_DAY, ANNUAL_SUN, NB_ANNUAL_STATS };
	enum TMonthlyStat{ MONTHLY_LOWEST_TMIN, MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMEAN, MONTHLY_MEAN_TMAX, MONTHLY_HIGHEST_TMAX, MONTHLY_PPT, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_MEAN_WNDS, MONTHLY_FROST_DAY, MONTHLY_FROSTFREE_DAY, MONTHLY_WET_DAY, MONTHLY_DRY_DAY, MONTHLY_SUN, NB_MONTHLY_STATS };
	enum TDailyStat{ DAILY_TMIN, DAILY_TMEAN, DAILY_TMAX, DAILY_PPT, NB_DAILY_STATS };//DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, DAILY_MEAN_WNDS, DAILY_SUN,
	enum THourlyStat{ HOULRY_TMIN, HOURLY_TAIR, HOULRY_TMAX, HOURLY_PRCP, HOURLY_TDEW, HOURLY_RELH, HOURLY_WNDS, HOURLY_WNDD, HOURLY_SRAD, HOURLY_PRES, NB_HOURLY_OUTPUTS };



	//Contructor
	CClimaticModel::CClimaticModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = -1;


		VERSION = "3.1.1 (2017)";

		m_varType = 0;
		m_a[0] = -0.9417;
		m_b[0] = -0.3227;
		m_a[1] = 0.207;
		m_b[1] = 0.205;

		m_x0 = 1;
		m_x1 = 1;
		m_x2 = 1;
		m_x3 = 1;

		m_bInit = false;
	}

	CClimaticModel::~CClimaticModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CClimaticModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;



		if (parameters.size() == 9)
		{
			int c = 0;
			m_varType = parameters[c++].GetInt();
			for (int i = 0; i < 2; i++)
			{
				m_a[i] = parameters[c++].GetReal();
				m_b[i] = parameters[c++].GetReal();
			}

			m_x0 = parameters[c++].GetReal();
			m_x1 = parameters[c++].GetReal();
			m_x2 = parameters[c++].GetReal();
			m_x3 = parameters[c++].GetReal();
		}

		return msg;
	}

	
	ERMsg CClimaticModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_STATS, -9999);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double annualMinimum = m_weather[y][H_TMIN2][LOWEST];
			double annualMinMean = m_weather[y][H_TMIN2][MEAN];
			double annualMean = m_weather[y][H_TAIR2][MEAN];
			double annualMaxMean = m_weather[y][H_TMAX2][MEAN];
			double annualMaximum = m_weather[y][H_TMAX2][HIGHEST];
			double annualPpt = m_weather[y][H_PRCP][SUM];
			double annualSun = m_weather[y][H_SRMJ][SUM];
			double annualSnow = m_weather[y][H_SNOW][SUM];
			double Tdew = m_weather[y][H_TDEW][MEAN];
			double relHum = m_weather[y][H_RELH][MEAN];
			double wnds = m_weather[y][H_WNDS][MEAN];

			size_t frostDay = GetNbFrostDay(m_weather[y]);
			size_t frostFreeDay = m_weather[y].GetNbDays() - frostDay;
			size_t nbWetDay = GetNbDayWithPrcp(m_weather[y]);
			size_t nbDryDay = m_weather[y].GetNbDays() - nbWetDay;

			m_output[y][ANNUAL_LOWEST_TMIN] = annualMinimum;
			m_output[y][ANNUAL_MEAN_TMIN] = annualMinMean;
			m_output[y][ANNUAL_MEAN_TMEAN] = annualMean;
			m_output[y][ANNUAL_MEAN_TMAX] = annualMaxMean;
			m_output[y][ANNUAL_HIGHEST_TMAX] = annualMaximum;
			m_output[y][ANNUAL_PPT] = annualPpt;
			m_output[y][ANNUAL_MEAN_TDEW] = Tdew;
			m_output[y][ANNUAL_MEAN_REL_HUM] = relHum;
			m_output[y][ANNUAL_MEAN_WNDS] = wnds;
			m_output[y][ANNUAL_FROST_DAY] = frostDay;
			m_output[y][ANNUAL_FROSTFREE_DAY] = frostFreeDay;
			m_output[y][ANNUAL_WET_DAY] = nbWetDay;
			m_output[y][ANNUAL_DRY_DAY] = nbDryDay;
			
			m_output[y][ANNUAL_SUN] = annualSun;
		}

	

		return msg;
	}

	ERMsg CClimaticModel::OnExecuteMonthly()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_MONTHLY_STATS, -9999);

		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m<12; m++)
			{

				double monthlyMinimum = m_weather[y][m][H_TMIN2][LOWEST];
				double monthlyMinMean = m_weather[y][m][H_TMIN2][MEAN];
				double monthlyMean = m_weather[y][m][H_TAIR2][MEAN];
				double monthlyMaxMean = m_weather[y][m][H_TMAX2][MEAN];
				double monthlyMaximum = m_weather[y][m][H_TMAX2][HIGHEST];
				double monthlyPpt = m_weather[y][m][H_PRCP][SUM];
				double monthlySun = m_weather[y][m][H_SRMJ][SUM];
				double Tdew = m_weather[y][m].GetStat(H_TDEW)[MEAN];
				double relHum = m_weather[y][m].GetStat(H_RELH)[MEAN];
				double wnds = m_weather[y][m].GetStat(H_WNDS)[MEAN];
				size_t frostDay = GetNbFrostDay(m_weather[y][m]);
				size_t frostFreeDay = m_weather[y][m].GetNbDays() - frostDay;
				size_t nbWetDay = GetNbDayWithPrcp(m_weather[y][m]);
				size_t nbDryDay = m_weather[y][m].GetNbDays() - nbWetDay;


				m_output[y * 12 + m][MONTHLY_LOWEST_TMIN] = monthlyMinimum;
				m_output[y * 12 + m][MONTHLY_MEAN_TMIN] = monthlyMinMean;
				m_output[y * 12 + m][MONTHLY_MEAN_TMEAN] = monthlyMean;
				m_output[y * 12 + m][MONTHLY_MEAN_TMAX] = monthlyMaxMean;
				m_output[y * 12 + m][MONTHLY_HIGHEST_TMAX] = monthlyMaximum;
				m_output[y * 12 + m][MONTHLY_PPT] = monthlyPpt;
				m_output[y * 12 + m][MONTHLY_MEAN_TDEW] = Tdew;
				m_output[y * 12 + m][MONTHLY_MEAN_REL_HUM] = relHum;
				m_output[y * 12 + m][MONTHLY_MEAN_WNDS] = wnds;
				m_output[y * 12 + m][MONTHLY_FROST_DAY] = frostDay;
				m_output[y * 12 + m][MONTHLY_FROSTFREE_DAY] = frostFreeDay;
				m_output[y * 12 + m][MONTHLY_WET_DAY] = nbWetDay;
				m_output[y * 12 + m][MONTHLY_DRY_DAY] = nbDryDay;
				m_output[y * 12 + m][MONTHLY_SUN] = monthlySun;
			}
		}


		return msg;
	}


	ERMsg CClimaticModel::OnExecuteDaily()
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

					//double Tmin = ;
					//double Tmean = wDay[H_TAIR2][MEAN];
					//double Tmax = wDay[H_TMAX2][MEAN];
					//double ppt = wDay[H_PRCP][SUM] >= 0.1 ? wDay[H_PRCP][SUM] : 0;

				/*	double Tdew = wDay[H_TDEW][MEAN];
					double relHum = wDay[H_RELH][MEAN];
					double wnds = wDay[H_WNDS][MEAN];
					double srad = wDay[H_SRAD2][MEAN];
*/
					CTRef ref = wDay.GetTRef();//m_weather[y][m][d].GetTRef();

					m_output[ref][DAILY_TMIN] = wDay[H_TMIN2][LOWEST];
					m_output[ref][DAILY_TMEAN] = wDay[H_TAIR2][MEAN];
					m_output[ref][DAILY_TMAX] = wDay[H_TMAX2][HIGHEST];
					m_output[ref][DAILY_PPT] = wDay[H_PRCP][SUM];
					/*m_output[ref][DAILY_MEAN_TDEW] = Tdew;
					m_output[ref][DAILY_MEAN_REL_HUM] = relHum;
					m_output[ref][DAILY_MEAN_WNDS] = wnds;
					m_output[ref][DAILY_SUN] = srad;*/
				}
			}
		}

		return msg;
	}

	ERMsg CClimaticModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CTPeriod p = m_weather.GetEntireTPeriod();
		m_output.Init(p, NB_HOURLY_OUTPUTS, -999);

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						CTRef TRef(p.GetFirstYear()+int(y),m,d,h);
						for (size_t v = 0; v < NB_HOURLY_OUTPUTS; v++)
							m_output[TRef][v] = m_weather[y][m][d][h][v];
					}
				}
			}
		}



		return msg;
	}

	int CClimaticModel::GetFrostDay(int year, const double& th)
	{
		int nbDays = 0;
		/* for(int day=0; day<m_weather[year].GetNbDay(); day++)
		 {
		 const CDay& d = m_weather[year].GetDay(day);

		 if(d.GetTMin()<th)
		 nbDays++;
		 }*/

		return nbDays;
	}





	//simulated annaling 
	void CClimaticModel::AddSAResult(const StringVector& header, const StringVector& data)
	{



		if (header.size() == 12)
		{
			std::vector<double> obs(4);

			CTRef TRef(ToShort(data[2]), ToShort(data[3]) - 1, ToShort(data[4]) - 1, ToShort(data[5]));
			for (int i = 0; i < 4; i++)
				obs[i] = ToDouble(data[i + 6]);


			ASSERT(obs.size() == 4);
			m_SAResult.push_back(CSAResult(TRef, obs));
		}

		/*if( header.size()==26)
		{
		std::vector<double> obs(24);

		for(int h=0; h<24; h++)
		obs[h] = data[h+2].ToDouble();


		ASSERT( obs.size() == 24 );
		m_SAResult.push_back( CSAResult(CTRef(), obs ) );
		}
		else if( header.size()==13)
		{
		std::vector<double> obs(7);

		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1,data[5].ToShort());
		for(int c=0; c<7; c++)
		obs[c] = data[c+6].ToDouble();


		ASSERT( obs.size() == 7 );
		m_SAResult.push_back( CSAResult(TRef, obs ) );
		}
		else if( header.size()==12)
		{
		std::vector<double> obs(7);

		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1);
		for(int c=0; c<7; c++)
		obs[c] = data[c+5].ToDouble();


		ASSERT( obs.size() == 7 );
		m_SAResult.push_back( CSAResult(TRef, obs ) );
		}
		else if( header.size()==11)
		{
		std::vector<double> obs(7);

		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1);
		for(int c=0; c<7; c++)
		obs[c] = data[c+4].ToDouble();


		ASSERT( obs.size() == 7 );
		m_SAResult.push_back( CSAResult(TRef, obs ) );
		}*/
	}

	void CClimaticModel::GetFValueHourly(CStatisticXY& stat)
	{
		if (m_SAResult.size() > 0)
		{
//			CHourlyStatVector data;
////			GetHourlyStat(data);
//
//			for (size_t d = 0; d < m_SAResult.size(); d++)
//			{
//				if (m_SAResult[d].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[d].m_ref))
//				{
//					static const int HOURLY_TYPE[6] = { HOURLY_T, HOURLY_TDEW, HOURLY_REL_HUM, HOURLY_WIND_SPEED, HOURLY_SRAD };
//					double obs = m_SAResult[d].m_obs[m_varType];
//					double sim = data[m_SAResult[d].m_ref][HOURLY_TYPE[m_varType]];
//
//
//					//double test = data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM];
//					//CFL::RH2Td(data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM], data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM]);
//
//					if (!_isnan(sim) && !_isnan(obs) &&
//						_finite(sim) && _finite(obs))
//						stat.Add(sim, obs);
//				}
//
//				HxGridTestConnection();
//
//			}

			/*
					if( m_SAResult[0].m_obs.size() == 24 )
					{
					//CTRef TRef = data.GetFirstTRef();
					//CStatistic statH[24];
					//for(int i=0; i<data.size(); i++, TRef++)
					//{
					//	double v = data[i][m_varType];
					//	statH[TRef.GetHour()]+=v;
					//	HxGridTestConnection();
					//}

					//for(int y=0; y<m_weather.GetNbYear(); y++)
					//{
					//	double DD=0;
					//	for(int m=0; m<12; m++)
					//	{
					//		for(int d=0; d<m_weather[y][m].GetNbDay(); d++)
					//		{
					//			const CWeatherDay& wDay = m_weather[y][m][d];
					//			for(int h=0; h<24; h++)
					//			{
					//
					//				//switch(m_varType)
					//				//{
					//				////case T_MN:
					//				//case TDEW: v= Min( wDay.GetT(h), GetVarH(wDay, h, var));break;
					//				//case RELH: v= Max(0, Min(100, GetVarH(wDay, h, var)));break;
					//				//case WNDS: v = Max(0, GetVarH(wDay, h, var));break;
					//				//}

					//				statH[h]+=v;
					//				HxGridTestConnection();
					//			}
					//		}
					//	}
					//}


					//ASSERT( m_SAResult.size() == 1 );
					//ASSERT( m_SAResult[0].m_obs.size() == 24 );
					//for(int h=0; h<24; h++)
					//{
					//	stat.Add(statH[h][MEAN], m_SAResult[0].m_obs[h]);
					//}
					}
					else if( m_SAResult[0].m_obs.size() == 7 )
					{


					for(size_t i=0; i<m_SAResult.size(); i++)
					{

					if( m_SAResult[i].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[i].m_ref))
					{
					double obs =  m_SAResult[i].m_obs[m_varType];
					double sim = data[m_SAResult[i].m_ref][m_varType];
					stat.Add(sim,obs);
					}

					HxGridTestConnection();

					}
					}
					*/
		}
	}

	void CClimaticModel::GetFValueDaily(CStatisticXY& stat)
	{

		if (m_SAResult.size() > 0)
		{
			//OnExecuteDaily();
			//const CDailyStatVector& data = (const CDailyStatVector&)GetOutput();

			//for (size_t i = 0; i < m_SAResult.size(); i++)
			//{

			//	if (m_SAResult[i].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[i].m_ref))
			//	{

			//		static const int DAILY_TYPE[6] = { DAILY_TMIN, DAILY_TMAX, DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, DAILY_MEAN_WNDS, DAILY_MEAN_VPD };
			//		double obs = m_SAResult[i].m_obs[m_varType];
			//		double sim = data[m_SAResult[i].m_ref][DAILY_TYPE[m_varType]];
			//		stat.Add(sim, obs);
			//	}

			//	HxGridTestConnection();

			//}
		}
	}


	void CClimaticModel::GetFValueMonthly(CStatisticXY& stat)
	{

		if (m_SAResult.size() > 0)
		{

			//OnExecuteMonthly();
			//const CMonthlyStatVector& data = (const CMonthlyStatVector&)GetOutput();

			//for (size_t i = 0; i < m_SAResult.size(); i++)
			//{

			//	if (m_SAResult[i].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[i].m_ref))
			//	{


			//		static const int MONTHLY_TYPE[6] = { MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMAX, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_MEAN_WNDS, MONTHLY_MEAN_VPD };
			//		double obs = m_SAResult[i].m_obs[m_varType];
			//		double sim = data[m_SAResult[i].m_ref][MONTHLY_TYPE[m_varType]];





			//		stat.Add(sim, obs);
			//	}

			//	HxGridTestConnection();

			//}
		}
	}


	//NOTE: Begin and END are ZERO-BASED Julian dates
	//Source:
	//Boughner, C.C. 1964. Distribution of growing degree days in Canada. 
	//Can. Met. Memoirs 17. Met. Br., Dept. of Transport. 40 p.
	//CPeriod GetGrowingSeason(CWeatherYear& weather)
	//{
	//	int day = 200; //(Mid-July)
	//	bool frost = false;
	//	CPeriod p(m_year, 0, GetLastDay());


	//	//Beginning of the growing season
	//	//look for the first occurrence of 3 successive days with frost
	//	do
	//	{

	//		frost = GetDay(day).GetTMin() < 0 &&
	//			GetDay(day - 1).GetTMin() < 0 &&
	//			GetDay(day - 2).GetTMin() < 0;

	//		day--;

	//	} while (!frost && day > 1);


	//	if (day>1)
	//	{
	//		p.Begin().SetJulianDay(day + 2);
	//	}


	//	//End of growing season
	//	day = 200;
	//	do
	//	{
	//		//look for the first occurrence of 3 successive days with frost
	//		frost = GetDay(day).GetTMin() < 0 &&
	//			GetDay(day + 1).GetTMin() < 0 &&
	//			GetDay(day + 2).GetTMin() < 0;
	//		day++;
	//	} while (!frost && day < GetLastDay() - 2);

	//	if (day<GetLastDay() - 2)
	//	{
	//		p.End().SetJulianDay(day - 2);
	//	}

	//	if (p.End() < p.Begin())
	//	{
	//		p.End() = p.Begin() = CDate(m_year, 200);
	//	}

	//	return p;
	//}


	//FrostFree period
	//CPeriod GetFrostFreePeriod(CWeatherYear& weather)
	//{
	//	CPeriod pTmp;
	//	CPeriod p;
	//	CWeatherYear::InitializePeriod(pTmp);//Init year
	//	//	CWeatherYear::InitializePeriod(p);

	//	//int FFPeriod=0;
	//	//int maxFFPeriod=0;
	//	bool notInit = true;

	//	int nbDay = GetNbDay();
	//	for (int jd = 0; jd<nbDay; jd++)
	//	{
	//		if (GetDay(jd).GetTMin()>0) //Frost-free period begin or continues
	//		{
	//			if (notInit)
	//			{
	//				pTmp.Begin().SetJulianDay(jd);
	//				notInit = false;
	//			}
	//		}
	//		else
	//		{
	//			if (!notInit)
	//			{
	//				pTmp.End().SetJulianDay(jd);
	//				notInit = true;

	//				//Frost-free period ends
	//				if (pTmp.GetLength() > p.GetLength())
	//					p = pTmp;
	//			}
	//		}

	//		if (jd == GetLastDay() && !notInit)
	//		{
	//			pTmp.End().SetJulianDay(jd);
	//			if (pTmp.GetLength() > p.GetLength())
	//				p = pTmp;
	//		}
	//	}



	//	return p;
	//}

	//return Water Deficit en mm of wather
	//double GetWaterDeficit(CWeatherYear& weather)
	//{
	//	//est-ce qu'on devrait utiliser cette équation à la place???
	//	//return max( 0, GetStat(PET, SUM) -	GetStat(PPT, SUM));

	//	CThornthwaitePET PET(*this, 0, CThornthwaitePET::POTENTIEL_STANDARD);
	//	double A = 0;
	//	//calculer Et pour le mois et A
	//	for (int m = 0; m<12; m++)
	//	{
	//		if (m_months[m].GetStat(TMEAN, MEAN)>0.)
	//		{
	//			//precipitation in mm
	//			double A_tmp = (PET.Get(m) - m_months[m].GetStat(PPT, SUM));
	//			if (A_tmp>0.)
	//				A += A_tmp;
	//		}
	//	}

	//	return(A);
	//}

	size_t GetNbDayWithPrcp(const CWeatherYear& weather)
	{
		CStatistic stat=0;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetNbDayWithPrcp(weather[m]);

		return stat[SUM];
	}

	size_t GetNbDayWithPrcp(const CWeatherMonth& weather)
	{
		size_t stat=0;
		for (size_t d = 0; d < weather.size(); d++)
			stat += (weather[d][H_PRCP][SUM] >= 0.2 ? 1 : 0);

		return stat;
	}
	/*
	double GetNbDayWithPrcp(const CWeatherDay& weather)
	{
		return (weather[H_PRCP][SUM]>=0.2?1:0);
	}
	*/
	size_t GetNbFrostDay(const CWeatherYear& weather)
	{
		size_t stat = 0;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetNbFrostDay(weather[m]);

		return stat;
	}

	size_t GetNbFrostDay(const CWeatherMonth& weather)
	{
		size_t stat = 0;
		for (size_t d = 0; d < weather.size(); d++)
			stat += (weather[d][H_TMIN2][LOWEST] <= 0 ? 1 : 0);

		return stat;
	}

	/*
	double GetNbFrostDay(const CWeatherDay& weather)
	{
		return (weather[H_TMIN2][LOWEST] <= 0 ? 1 : 0);
	}
*/
	
}