//**********************************************************************
// 02/10/2019	1.0.0	Rémi Saint-Amant	Creation
//**********************************************************************

#include <iostream>
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "ModelBase/EntryPoint.h"
#include "SoilTemperature.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CSoilTemperatureModel::CreateObject);


	enum TDailyOutput{ O_SOIL_TEMPERATURE_MIN, O_SOIL_TEMPERATURE_MEAN, O_SOIL_TEMPERATURE_MAX, NB_OUTPUTS };

	//Contructor
	CSoilTemperatureModel::CSoilTemperatureModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2019)";
	}

	CSoilTemperatureModel::~CSoilTemperatureModel()
	{}


	//This method is call to load your parameter in your variable
	ERMsg CSoilTemperatureModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;
		return msg;
	}

	
	ERMsg CSoilTemperatureModel::OnExecuteAnnaul()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_OUTPUTS, -9999);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double daylightSoilTemperature = GetDaylightVaporPressureDeficit(m_weather[y]); //[kPa]
			double SoilTemperature = m_weather[y][H_SoilTemperature][MEAN]; //[kPa]

			m_output[y][O_SOIL_TEMPERATURE_MIN] = daylightSoilTemperature*10;//kPa --> hPa
			m_output[y][O_SOIL_TEMPERATURE_MEAN] = SoilTemperature*10;//kPa --> hPa
			m_output[y][O_SOIL_TEMPERATURE_MAX] = SoilTemperature * 10;//kPa --> hPa
		}

		return msg;
	}

	ERMsg CSoilTemperatureModel::OnExecuteMonthly()
	{
		ERMsg msg;


		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		m_output.Init(p, NB_OUTPUTS, -9999);

		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m<12; m++)
			{
				double daylightSoilTemperature = GetDaylightVaporPressureDeficit(m_weather[y][m]); //[kPa]
				double SoilTemperature = GetSoilTemperature(m_weather[y][m]); //[kPa]

				m_output[y * 12 + m][O_DAYLIGHT_SoilTemperature] = daylightSoilTemperature*10;//kPa --> hPa
				m_output[y * 12 + m][O_MEAN_SoilTemperature] = SoilTemperature*10;//kPa --> hPa
			}
		}


		return msg;
	}


	ERMsg CSoilTemperatureModel::OnExecuteDaily()
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

					double daylightSoilTemperature = GetDaylightVaporPressureDeficit(wDay); //[kPa]
					double SoilTemperature = GetSoilTemperature(wDay);//[kPa]

					CTRef ref = m_weather[y][m][d].GetTRef();

					m_output[ref][O_DAYLIGHT_SoilTemperature] = daylightSoilTemperature*10;//kPa --> hPa
					m_output[ref][O_MEAN_SoilTemperature] = SoilTemperature*10;//kPa --> hPa
				}
			}
		}


		return msg;
	}

	ERMsg CSoilTemperatureModel::OnExecuteHourly()
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
						double SoilTemperature = max(0.0f, wHour[H_ES] - wHour[H_EA] )*10;//kPa --> hPa

						CTRef ref = m_weather[y][m][d][h].GetTRef();
						size_t sunrise = Round(sun.GetSunrise(ref));
						size_t sunset = min(23ll, Round(sun.GetSunset(ref)));

						double daylightSoilTemperature = (h >= sunrise && h <= sunset) ? SoilTemperature : -9999;

						m_output[ref][O_DAYLIGHT_SoilTemperature] = daylightSoilTemperature;//hPa
						m_output[ref][O_MEAN_SoilTemperature] = SoilTemperature;//hPa
					}
				}
			}
		}


		return msg;
	}

	//Saturation vapor pressure at daylight temperature [kPa]
	double GetDaylightVaporPressureDeficit(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetDaylightVaporPressureDeficit(weather[m]);

		return stat[MEAN];
	}

	//Saturation vapor pressure at daylight temperature [kPa]
	double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetDaylightVaporPressureDeficit(weather[d]);

		return stat[MEAN];
	}

	//Saturation vapor pressure at daylight temperature [kPa]
	double GetDaylightVaporPressureDeficit(const CWeatherDay& weather)
	{
		CStatistic SoilTemperature;
		if (weather.IsHourly())
		{
			CSun sun(weather.GetLocation().m_lat, weather.GetLocation().m_lon);
			size_t sunrise = Round(sun.GetSunrise(weather.GetTRef()));
			size_t sunset = min(23ll, Round(sun.GetSunset(weather.GetTRef())));

			for (size_t h = sunrise; h <= sunset; h++)
				SoilTemperature += max(0.0f, weather[h][H_ES] - weather[h][H_EA]);
				
		}
		else
		{
			double daylightT = weather.GetTdaylight();
			double daylightEs = eᵒ(daylightT);//kPa // *1000; //by RSA 11-09-2018 kPa instead of Pa
			SoilTemperature += max(0.0, daylightEs - weather[H_EA][MEAN]);

			//double Dmax = eᵒ(weather[H_TMAX]) - eᵒ(weather[H_TMIN]);
			//SoilTemperature += 2.0 / 3.0*Dmax;

			//SoilTemperature += max(0.0, weather[H_ES][MEAN] - weather[H_EA][MEAN]);
		}

		return SoilTemperature[MEAN];
	}

	//return vapor pressure deficit [kPa]
	static double GetSoilTemperature(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetSoilTemperature(weather[m]);

		return stat[MEAN];
	}
	//return vapor pressure deficit [kPa]
	static double GetSoilTemperature(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetSoilTemperature(weather[d]);

		return stat[MEAN];
	}

	/*double ʃ(double Tᴰ, double Tᴸ, double Tᴴ )
	{
		ASSERT(Tᴰ <= Tᴸ);
		ASSERT(Tᴸ <= Tᴴ);

		double sum=0;
		for (size_t i = 0; i <= 100; i++)
		{
			double T = Tᴸ + i*(Tᴴ - Tᴸ) / 100.0;
			sum += eᵒ(T) - eᵒ(Tᴰ);
		}
		
		return sum;
	}*/

	double ʃ(double Tᴰ, double T)
	{
		ASSERT(Tᴰ <= T);
		return eᵒ(T) - eᵒ(Tᴰ);
	}


	// Function to evalute the value of integral 
	double trapezoidal(double Tᴰ, double Tᴸ, double Tᴴ, int n = 100)
	{
		// Grid spacing 
		double h = (Tᴴ - Tᴸ) / n;

		// Computing sum of first and last terms 
		// in above formula 
		double s = ʃ(Tᴰ, Tᴸ) + ʃ(Tᴰ, Tᴴ);

		// Adding middle terms in above formula 
		for (int i = 1; i < n; i++)
			s += 2 * ʃ(Tᴰ, Tᴸ + i * h);

		// h/2 indicates (b-a)/2n. Multiplying h/2 
		// with s. 
		return (h / 2)*s;
	}
	//return vapor pressure deficit [kPa]
	static double GetSoilTemperature(const CWeatherDay& weather)
	{
		CStatistic stat;
		if (weather.IsHourly())
		{
			for (size_t h = 0; h < 24; h++)
				stat += max(0.0f, weather[h][H_ES] - weather[h][H_EA]);
		}
		else
		{
			stat += max(0.0, weather[H_ES][MEAN] - weather[H_EA][MEAN]);


			//test from Castellvi(1996)
		/*	
			double Tᴰ = eᵒ(weather[H_TDEW][MEAN]);
			double Tn = eᵒ(weather[H_TMIN][MEAN]);
			double Tx = eᵒ(weather[H_TMAX][MEAN]);
			double Ta = (Tn+Tx)/2;
			if (Tᴰ < Tn && Tn < Tx)
			{
				double a = trapezoidal(Tᴰ, Tn, Ta);
				double b = trapezoidal(Tᴰ, Tn, Tx);
				ASSERT(a/b > 0 && a/b<=1);

				while (abs(0.5 - a/b)> 0.001)
				{
					Ta += (0.5-a/b)*(Tx-Tn);
					ASSERT(Ta >= Tn && Ta <= Tx);
					a = trapezoidal(Tᴰ, Tn, Ta);
					ASSERT(a / b >= 0 && a / b <= 1);
				}
			}

			double h = std::max(1.0, std::min(100.0, weather[H_RELH][MEAN]));
			stat += eᵒ(Ta)*(1- h /100.0);
*/
			
		}

		return stat[MEAN];
	}
	
}