//**********************************************************************
// 11/09/2018	1.0.0	Rémi Saint-Amant    Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "Daily vs Hourly.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CDaily_vs_Hourly::CreateObject);

	//Contructor
	CDaily_vs_Hourly::CDaily_vs_Hourly()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;

		VERSION = "1.0.0 (2018)";
	}

	CDaily_vs_Hourly::~CDaily_vs_Hourly()
	{}

	//This method is call to load your parameter in your variable
	ERMsg CDaily_vs_Hourly::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		return msg;
	}

	ERMsg CDaily_vs_Hourly::OnExecuteDaily()
	{
		ERMsg msg;
		if (m_weather.IsHourly())
		{
			CWVariables vars = m_weather.GetVariables();
			ASSERT(vars.count() == m_info.m_outputVariables.size());

			CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
			m_output.Init(p, vars.count(), -999);

			for (size_t y = 0; y < m_weather.size(); y++)
			{
				for (size_t m = 0; m < m_weather[y].size(); m++)
				{
					for (size_t d = 0; d < m_weather[y][m].size(); d++)
					{
						const CWeatherDay& wDay = m_weather[y][m][d];


						for (size_t v = 0, vv = 0; v < vars.size(); v++)
						{
							if (vars.test(v))
							{
								CTRef ref = wDay.GetTRef();
								switch (v)
								{
								case H_TMIN: m_output[ref][vv] = Round(wDay[H_TMIN][LOWEST], 1); break;
								case H_TAIR: m_output[ref][vv] = Round(wDay[H_TAIR][MEAN], 1); break;
								case H_TMAX: m_output[ref][vv] = Round(wDay[H_TMAX][HIGHEST], 1); break;
								case H_PRCP:  m_output[ref][vv] = Round(wDay[H_PRCP][SUM], 1); break;
								case H_TDEW:  m_output[ref][vv] = Round(wDay[H_TDEW][MEAN], 1); break;
								case H_RELH:  m_output[ref][vv] = Round(wDay[H_RELH][MEAN], 1); break;
								case H_WNDS:  m_output[ref][vv] = Round(wDay[H_WNDS][MEAN], 1); break;
								case H_WNDD:  m_output[ref][vv] = Round(wDay[H_WNDD][MEAN], 1); break;
								case H_SRAD:  m_output[ref][vv] = Round(wDay[H_SRAD][MEAN], 1); break;
								default: ASSERT(false);
								}
								vv++;
							}
						}
					}
				}
			}
		}
		else
		{
			msg.ajoute("This model need hourly data as input");
		}

		return msg;
	}

	ERMsg CDaily_vs_Hourly::OnExecuteHourly()
	{
		ERMsg msg;
		if (m_weather.IsDaily())
		{
			CWVariables vars = m_weather.GetVariables();
			ASSERT(vars.count() == m_info.m_outputVariables.size());

			m_weather.ComputeHourlyVariables(); 


			CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::HOURLY));
			m_output.Init(p, vars.count(), -999);

			for (size_t y = 0; y < m_weather.size(); y++)
			{
				for (size_t m = 0; m < m_weather[y].size(); m++)
				{
					for (size_t d = 0; d < m_weather[y][m].size(); d++)
					{
						for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
						{
							const CHourlyData& wHour = m_weather[y][m][d][h];

							for (size_t v = 0, vv = 0; v < vars.size(); v++)
							{
								if (vars.test(v))
								{
									CTRef ref = wHour.GetTRef();
									switch (v)
									{
									case H_TMIN: m_output[ref][vv] = Round(wHour[H_TMIN], 1); break;
									case H_TAIR: m_output[ref][vv] = Round(wHour[H_TAIR], 1); break;
									case H_TMAX: m_output[ref][vv] = Round(wHour[H_TMAX], 1); break;
									case H_PRCP:  m_output[ref][vv] = Round(wHour[H_PRCP], 1); break;
									case H_TDEW:  m_output[ref][vv] = Round(wHour[H_TDEW], 1); break;
									case H_RELH:  m_output[ref][vv] = Round(wHour[H_RELH], 1); break;
									case H_WNDS:  m_output[ref][vv] = Round(wHour[H_WNDS], 1); break;
									case H_WNDD:  m_output[ref][vv] = Round(wHour[H_WNDD], 1); break;
									case H_SRAD: m_output[ref][vv] = Round(wHour[H_SRAD], 1); break;
									default: ASSERT(false);
									}
									vv++;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			msg.ajoute("This model need daily data as input");
		}

		return msg;
	}
}