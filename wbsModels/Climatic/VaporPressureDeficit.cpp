//**********************************************************************
// 20/09/2016	1.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	1.0.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "VaporPressureDeficit.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CVPDModel::CreateObject);


	//Saturation vapor pressure at daylight temperature
	static double GetDaylightVaporPressureDeficit(const CWeatherYear& weather);
	static double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather);
	static double GetDaylightVaporPressureDeficit(const CWeatherDay& weather);
	
	static double GetVPD(const CWeatherYear& weather);
	static double GetVPD(const CWeatherMonth& weather);
	static double GetVPD(const CWeatherDay& weather);


	enum TAnnualStat{ O_DAYLIGHT_VPD, O_MEAN_VPD, NB_OUTPUTS };

	//Contructor
	CVPDModel::CVPDModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.1.0 (2016)";
	}

	CVPDModel::~CVPDModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CVPDModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;
		return msg;
	}

	
	ERMsg CVPDModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_OUTPUTS, -9999);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double daylightVPD = GetDaylightVaporPressureDeficit(m_weather[y]); //[Pa]
			double VPD = m_weather[y][H_VPD2][MEAN]; //[Pa]

			m_output[y][O_DAYLIGHT_VPD] = daylightVPD;
			m_output[y][O_MEAN_VPD] = VPD;
		}

		return msg;
	}

	ERMsg CVPDModel::OnExecuteMonthly()
	{
		ERMsg msg;


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_OUTPUTS, -9999);

		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m<12; m++)
			{
				double daylightVPD = GetDaylightVaporPressureDeficit(m_weather[y][m]); //[Pa]
				double VPD = GetVPD(m_weather[y][m]); //[Pa]

				m_output[y * 12 + m][O_DAYLIGHT_VPD] = daylightVPD;
				m_output[y * 12 + m][O_MEAN_VPD] = VPD;
			}
		}


		return msg;
	}


	ERMsg CVPDModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, -9999);

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					const CWeatherDay& wDay = m_weather[y][m][d];

					double daylightVPD = GetDaylightVaporPressureDeficit(wDay); //Pa
					double VPD = GetVPD(wDay);//Pa

					CTRef ref = m_weather[y][m][d].GetTRef();

					m_output[ref][O_DAYLIGHT_VPD] = daylightVPD;
					m_output[ref][O_MEAN_VPD] = VPD;
				}
			}
		}


		return msg;
	}

	ERMsg CVPDModel::OnExecuteHourly()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::HOURLY);
		m_output.Init(p, NB_OUTPUTS, -9999);

		
		CSun sun(m_weather.m_lat, m_weather.m_lon);
		
		
		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					for (size_t h = 0; h < m_weather[y][m][d].size(); h++)
					{
						const CHourlyData& wHour = m_weather[y][m][d][h];

						//double daylightVPD = GetDaylightVaporPressureDeficit(wDay); //Pa
						double VPD = max(0.0f, wHour[H_ES2] - wHour[H_EA2] );

						CTRef ref = m_weather[y][m][d][h].GetTRef();
						size_t sunrise = Round(sun.GetSunrise(ref));
						size_t sunset = min(23ll, Round(sun.GetSunset(ref)));

						double daylightVPD = (h >= sunrise && h <= sunset) ? VPD : -9999;

						m_output[ref][O_DAYLIGHT_VPD] = daylightVPD;
						m_output[ref][O_MEAN_VPD] = VPD;
					}
				}
			}
		}


		return msg;
	}


	double GetDaylightVaporPressureDeficit(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetDaylightVaporPressureDeficit(weather[m]);

		return stat[MEAN];
	}

	double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetDaylightVaporPressureDeficit(weather[d]);

		return stat[MEAN];
	}

	//Saturation vapor pressure at daylight temperature
	double GetDaylightVaporPressureDeficit(const CWeatherDay& weather)
	{
		CStatistic VPD;
		if (weather.IsHourly())
		{
			CSun sun(weather.GetLocation().m_lat, weather.GetLocation().m_lon);
			size_t sunrise = Round(sun.GetSunrise(weather.GetTRef()));
			size_t sunset = min(23ll, Round(sun.GetSunset(weather.GetTRef())));

			for (size_t h = sunrise; h <= sunset; h++)
				VPD += max(0.0f, weather[h][H_ES2] - weather[h][H_EA2]);
				
		}
		else
		{
			double daylightT = weather.GetTdaylight();
			double daylightEs = eᵒ(daylightT) * 1000;
			VPD += max(0.0, daylightEs - weather[H_EA2][MEAN]);
		}

		return VPD[MEAN];
	}


	static double GetVPD(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetVPD(weather[m]);

		return stat[MEAN];
	}

	static double GetVPD(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetVPD(weather[d]);

		return stat[MEAN];
	}

	static double GetVPD(const CWeatherDay& weather)
	{
		CStatistic stat;
		if (weather.IsHourly())
		{
			for (size_t h = 0; h < 24; h++)
				stat += max(0.0f, weather[h][H_ES2] - weather[h][H_EA2]);
		}
		else
		{
			stat += max(0.0, weather[H_ES2][MEAN] - weather[H_EA2][MEAN]);
		}

		return stat[MEAN];
	}
	
}