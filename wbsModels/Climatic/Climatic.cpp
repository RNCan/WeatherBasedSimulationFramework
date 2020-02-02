//**********************************************************************
// 31/01/2020   3.2.0	Rémi Saint-Amant    Add m_prcp_thres and Ex models with wind direction clusters in new model ClimaticWind
// 16/03/2018	3.1.2	Rémi Saint-Amant    hourly and daily SRad in MJ/m² instread of W/m²
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


	enum TAnnualStat{ ANNUAL_LOWEST_TMIN, ANNUAL_MEAN_TMIN, ANNUAL_MEAN_TMEAN, ANNUAL_MEAN_TMAX, ANNUAL_HIGHEST_TMAX, ANNUAL_PPT, ANNUAL_MEAN_TDEW, ANNUAL_MEAN_REL_HUM, ANNUAL_SUN, ANNUAL_FROST_DAY, ANNUAL_FROSTFREE_DAY, ANNUAL_WET_DAY, ANNUAL_DRY_DAY, NB_ANNUAL_STATS };
	enum TMonthlyStat{ MONTHLY_LOWEST_TMIN, MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMEAN, MONTHLY_MEAN_TMAX, MONTHLY_HIGHEST_TMAX, MONTHLY_PPT, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_SUN, MONTHLY_FROST_DAY, MONTHLY_FROSTFREE_DAY, MONTHLY_WET_DAY, MONTHLY_DRY_DAY, NB_MONTHLY_STATS };
	enum TDailyStat{ DAILY_TMIN, DAILY_TAIR, DAILY_TMAX, DAILY_PRCP, DAILY_TDEW, DAILY_RELH, DAILY_SRAD, NB_DAILY_STATS };
	enum THourlyStat{ HOURLY_TMIN, HOURLY_TAIR, HOURLY_TMAX, HOURLY_PRCP, HOURLY_TDEW, HOURLY_RELH, HOURLY_SRAD, NB_HOURLY_OUTPUTS };

	enum TDailyStatEx { DAILY_TMIN_EX, DAILY_TAIR_EX, DAILY_TMAX_EX, DAILY_PRCP_EX, DAILY_TDEW_EX, DAILY_RELH_EX, DAILY_WNDS_EX, DAILY_WNDD_EX, DAILY_SRAD_EX, NB_DAILY_STATS_EX };
	enum THourlyStatEx { HOULRY_TMIN_EX, HOURLY_TAIR_EX, HOULRY_TMAX_EX, HOURLY_PRCP_EX, HOURLY_TDEW_EX_EX, HOURLY_RELH_EX_EX, HOURLY_WNDS_EX, HOURLY_WNDD_EX, HOURLY_SRAD_EX, NB_HOURLY_OUTPUTS_EX };

//	enum TDailyStatEx { NB_DAILY_STATS_EX };
	//enum THourlyStatEx { NB_HOURLY_OUTPUTS_EX };


	//Contructor
	CClimaticModel::CClimaticModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = -1;
		VERSION = "3.2.0 (2020)";

		m_bEx = false;
		m_prcp_thres = 0.2;
	}

	CClimaticModel::~CClimaticModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CClimaticModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		if (parameters.size() == 1)
		{
			size_t c = 0;
			m_prcp_thres = parameters[c++].GetReal();
		}

		m_bEx = m_info.m_modelName.find("Ex") != string::npos;
		return msg;
	}

	
	ERMsg CClimaticModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_STATS, -9999);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double annualMinimum = m_weather[y][H_TMIN][LOWEST];
			double annualMinMean = m_weather[y][H_TMIN][MEAN];
			double annualMean = m_weather[y][H_TAIR][MEAN];
			double annualMaxMean = m_weather[y][H_TMAX][MEAN];
			double annualMaximum = m_weather[y][H_TMAX][HIGHEST];
			double annualPpt = m_weather[y][H_PRCP][SUM];
			double Tdew = m_weather[y][H_TDEW][MEAN];
			double relHum = m_weather[y][H_RELH][MEAN];
			double annualSun = m_weather[y][H_SRMJ][SUM];
			size_t frostDay = GetNbFrostDay(m_weather[y]);
			size_t frostFreeDay = m_weather[y].GetNbDays() - frostDay;
			size_t nbWetDay = GetNbDayWithPrcp(m_weather[y], m_prcp_thres);
			size_t nbDryDay = m_weather[y].GetNbDays() - nbWetDay;

			m_output[y][ANNUAL_LOWEST_TMIN] = Round(annualMinimum, 1);
			m_output[y][ANNUAL_MEAN_TMIN] = Round(annualMinMean, 1);
			m_output[y][ANNUAL_MEAN_TMEAN] = Round(annualMean, 1);
			m_output[y][ANNUAL_MEAN_TMAX] = Round(annualMaxMean, 1);
			m_output[y][ANNUAL_HIGHEST_TMAX] = Round(annualMaximum, 1);
			m_output[y][ANNUAL_PPT] = Round(annualPpt, 1);
			m_output[y][ANNUAL_MEAN_TDEW] = Round(Tdew, 1);
			m_output[y][ANNUAL_MEAN_REL_HUM] = Round(relHum, 1);
			m_output[y][ANNUAL_SUN] = Round(annualSun, 1);
			
			m_output[y][ANNUAL_FROST_DAY] = frostDay;
			m_output[y][ANNUAL_FROSTFREE_DAY] = frostFreeDay;
			m_output[y][ANNUAL_WET_DAY] = nbWetDay;
			m_output[y][ANNUAL_DRY_DAY] = nbDryDay;
			
			
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

				double monthlyMinimum = m_weather[y][m][H_TMIN][LOWEST];
				double monthlyMinMean = m_weather[y][m][H_TMIN][MEAN];
				double monthlyMean = m_weather[y][m][H_TAIR][MEAN];
				double monthlyMaxMean = m_weather[y][m][H_TMAX][MEAN];
				double monthlyMaximum = m_weather[y][m][H_TMAX][HIGHEST];
				double Tdew = m_weather[y][m].GetStat(H_TDEW)[MEAN];
				double relHum = m_weather[y][m].GetStat(H_RELH)[MEAN];
				double monthlyPpt = m_weather[y][m][H_PRCP][SUM];
				double monthlySun = m_weather[y][m][H_SRMJ][SUM];
				size_t frostDay = GetNbFrostDay(m_weather[y][m]);
				size_t frostFreeDay = m_weather[y][m].GetNbDays() - frostDay;
				size_t nbWetDay = GetNbDayWithPrcp(m_weather[y][m], m_prcp_thres);
				size_t nbDryDay = m_weather[y][m].GetNbDays() - nbWetDay;


				m_output[y * 12 + m][MONTHLY_LOWEST_TMIN] = Round(monthlyMinimum,1);
				m_output[y * 12 + m][MONTHLY_MEAN_TMIN] = Round(monthlyMinMean, 1);
				m_output[y * 12 + m][MONTHLY_MEAN_TMEAN] = Round(monthlyMean, 1);
				m_output[y * 12 + m][MONTHLY_MEAN_TMAX] = Round(monthlyMaxMean, 1);
				m_output[y * 12 + m][MONTHLY_HIGHEST_TMAX] = Round(monthlyMaximum, 1);
				m_output[y * 12 + m][MONTHLY_PPT] = Round(monthlyPpt, 1);
				m_output[y * 12 + m][MONTHLY_MEAN_TDEW] = Round(Tdew, 1);
				m_output[y * 12 + m][MONTHLY_MEAN_REL_HUM] = Round(relHum, 1);
				m_output[y * 12 + m][MONTHLY_SUN] = Round(monthlySun, 1);


				m_output[y * 12 + m][MONTHLY_FROST_DAY] = frostDay;
				m_output[y * 12 + m][MONTHLY_FROSTFREE_DAY] = frostFreeDay;
				m_output[y * 12 + m][MONTHLY_WET_DAY] = nbWetDay;
				m_output[y * 12 + m][MONTHLY_DRY_DAY] = nbDryDay;
				
			}
		}


		return msg;
	}


	ERMsg CClimaticModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, m_bEx ? NB_DAILY_STATS_EX : NB_DAILY_STATS, -9999);

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					const CWeatherDay& wDay = m_weather[y][m][d];
					CTRef ref = wDay.GetTRef();

					

					if (m_bEx)
					{
						m_output[ref][DAILY_TMIN_EX] = Round(wDay[H_TMIN][LOWEST], 1);
						m_output[ref][DAILY_TAIR_EX] = Round(wDay[H_TAIR][MEAN], 1);
						m_output[ref][DAILY_TMAX_EX] = Round(wDay[H_TMAX][HIGHEST], 1);
						m_output[ref][DAILY_PRCP_EX] = Round(wDay[H_PRCP][SUM], 1);
						m_output[ref][DAILY_TDEW_EX] = Round(wDay[H_TDEW][MEAN], 1);
						m_output[ref][DAILY_RELH_EX] = Round(wDay[H_RELH][MEAN], 1);
						m_output[ref][DAILY_WNDS_EX] = Round(wDay[H_WNDS][MEAN], 1);
						m_output[ref][DAILY_WNDD_EX] = Round(wDay[H_WNDD][MEAN], 1);
						m_output[ref][DAILY_SRAD_EX] = Round(wDay[H_SRAD][SUM], 3);
					}
					else
					{
						m_output[ref][DAILY_TMIN] = Round(wDay[H_TMIN][LOWEST], 1);
						m_output[ref][DAILY_TAIR] = Round(wDay[H_TAIR][MEAN], 1);
						m_output[ref][DAILY_TMAX] = Round(wDay[H_TMAX][HIGHEST], 1);
						m_output[ref][DAILY_PRCP] = Round(wDay[H_PRCP][SUM], 1);
						m_output[ref][DAILY_TDEW] = Round(wDay[H_TDEW][MEAN], 1);
						m_output[ref][DAILY_RELH] = Round(wDay[H_RELH][MEAN], 1);
						m_output[ref][DAILY_SRAD] = Round(wDay[H_SRAD][MEAN], 3);

					}
				}
			}
		}

		return msg;
	}

	ERMsg CClimaticModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
		{
			m_weather.ComputeHourlyVariables();
		}

		CTPeriod p = m_weather.GetEntireTPeriod();
		m_output.Init(p, m_bEx ? NB_HOURLY_OUTPUTS_EX : NB_HOURLY_OUTPUTS, -999);
		


		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						CTRef TRef(p.GetFirstYear()+int(y),m,d,h);
						const CHourlyData& wh = m_weather.GetHour(TRef);

						if (m_bEx)
						{
							for (size_t v = 0; v < NB_HOURLY_OUTPUTS_EX; v++)
								m_output[TRef][v] = wh[v];
								//m_output[TRef][v] = m_weather[y][m][d][h][v];
						}
						else
						{
							m_output[TRef][HOURLY_TMIN] = Round(wh[H_TMIN], 1);
							m_output[TRef][HOURLY_TAIR] = Round(wh[H_TAIR], 1);
							m_output[TRef][HOURLY_TMAX] = Round(wh[H_TMAX], 1);
							m_output[TRef][HOURLY_PRCP] = Round(wh[H_PRCP], 1);
							m_output[TRef][HOURLY_TDEW] = Round(wh[H_TDEW], 1);
							m_output[TRef][HOURLY_RELH] = Round(wh[H_RELH], 1);
							m_output[TRef][HOURLY_SRAD] = Round(wh[H_SRAD], 3);
						}
					}
				}
			}
		}



		return msg;
	}


	size_t CClimaticModel::GetNbDayWithPrcp(const CWeatherYear& weather, double prcp_thres)
	{
		CStatistic stat=0;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetNbDayWithPrcp(weather[m], prcp_thres);

		return stat[SUM];
	}

	size_t CClimaticModel::GetNbDayWithPrcp(const CWeatherMonth& weather, double prcp_thres)
	{
		size_t stat=0;
		for (size_t d = 0; d < weather.size(); d++)
			stat += (weather[d][H_PRCP][SUM] >= prcp_thres ? 1 : 0);

		return stat;
	}
	
	size_t CClimaticModel::GetNbFrostDay(const CWeatherYear& weather)
	{
		size_t stat = 0;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetNbFrostDay(weather[m]);

		return stat;
	}

	size_t CClimaticModel::GetNbFrostDay(const CWeatherMonth& weather)
	{
		size_t stat = 0;
		for (size_t d = 0; d < weather.size(); d++)
			stat += (weather[d][H_TMIN][LOWEST] <= 0 ? 1 : 0);

		return stat;
	}
}