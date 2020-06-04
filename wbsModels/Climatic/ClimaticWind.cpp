//**********************************************************************
// 31/01/2020   3.2.0	Rémi Saint-Amant    Creation from CLimatic model. Version sychronize with climatic model.
//**********************************************************************

#include <iostream>
#include "ModelBase/EntryPoint.h"
//#include "Climatic.h"
#include "ClimaticWind.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CClimaticWindModel::CreateObject);

	enum TAnnualStatWind { ANNUAL_MEAN_WNDS, ANNUAL_MEAN_WNDS0, ANNUAL_MEAN_WNDS35= ANNUAL_MEAN_WNDS0+35, NB_ANNUAL_STATS_WIND };
	enum TMonthlyStatWind { MONTHLY_MEAN_WNDS, MONTHLY_MEAN_WNDS0, MONTHLY_MEAN_WNDS35 = ANNUAL_MEAN_WNDS0 + 35, NB_MONTHLY_STATS_WIND };
	
	//Contructor
	CClimaticWindModel::CClimaticWindModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "3.2.0 (2020)";
	}

	CClimaticWindModel::~CClimaticWindModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CClimaticWindModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//size_t c = 0;
		//m_prcp_thres = parameters[c++].GetReal();

		return msg;
	}


	ERMsg CClimaticWindModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_ANNUAL_STATS_WIND, 0);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double wnds = m_weather[y][H_WNDS][MEAN];
			array<double, 36> wndd = GetWindD(m_weather[y]);
			m_output[y][ANNUAL_MEAN_WNDS] = Round(wnds, 1);

			for (size_t i = 0; i < 36; i++)
				m_output[y][ANNUAL_MEAN_WNDS0 + i] = Round(wndd[i], 1);
		}

		return msg;
	}

	ERMsg CClimaticWindModel::OnExecuteMonthly()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_MONTHLY_STATS_WIND , 0);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m < 12; m++)
			{
					double wnds = m_weather[y][m].GetStat(H_WNDS)[MEAN];
					array<double, 36> wndd = GetWindD(m_weather[y][m]);

					m_output[y * 12 + m][MONTHLY_MEAN_WNDS] = Round(wnds, 1);

					for (size_t i = 0; i < 36; i++)
						m_output[y * 12 + m][MONTHLY_MEAN_WNDS0 + i] = Round(wndd[i], 1);
				
			}
		}


		return msg;
	}

	 
	array<double, 36> CClimaticWindModel::GetWindD(const CWeatherYear& weather)
	{
		array<double, 36> wndd = { 0 };

		CStatistic stat;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			if (weather.IsHourly())
			{
				for (size_t h = 0; h < 24; h++)
				{
					double ws = weather.GetDay(d).at(h).at(H_WNDS);
					double wd = weather.GetDay(d).at(h).at(H_WNDD);
					size_t i = ((size_t)((wd + 5.0) / 10.0)) % 36;
					ASSERT(i < 36);
					wndd[i] += ws;
					stat += ws;
				}
			}
			else
			{
				double ws = weather.GetDay(d)[H_WNDS][MEAN];
				double wd = weather.GetDay(d)[H_WNDD][MEAN];
				size_t i = ((size_t)((wd + 5.0) / 10.0)) % 36;
				ASSERT(i < 36);
				wndd[i] += ws;
				stat += ws;
			}
		}

		//compute in percent;
		if (stat[SUM] > 0)
		{
			for (size_t i = 0; i < 36; i++)
				wndd[i] = wndd[i] * 100 / stat[SUM];
		}


		return wndd;
	}

	array<double, 36> CClimaticWindModel::GetWindD(const CWeatherMonth& weather)
	{
		array<double, 36> wndd = { 0 };

		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
		{
			if (weather.IsHourly())
			{
				for (size_t h = 0; h < 24; h++)
				{
					double ws = weather[d].at(h).at(H_WNDS);
					double wd = weather[d].at(h).at(H_WNDD);
					size_t i = ((size_t)((wd + 5.0) / 10.0)) % 36;
					ASSERT(i < 36);
					wndd[i] += ws;
					stat += ws;


				}

			}
			else
			{
				double ws = weather[d][H_WNDS][MEAN];
				double wd = weather[d][H_WNDD][MEAN];
				size_t i = ((size_t)((wd + 5.0) / 10.0)) % 36;
				ASSERT(i < 36);
				wndd[i] += ws;
				stat += ws;
			}


		}

		//compute in percent;
		if (stat[SUM] > 0)
		{
			for (size_t i = 0; i < 36; i++)
				wndd[i] = wndd[i] * 100 / stat[SUM];
		}

		return wndd;
	}


}