//This code was created from article :
//Will climate change drive 21st century burn rates in Canadian boreal forest outside of its natural variability:
//collating global climate model experiments with sedimentary charcoal data
//Yves Bergeron, Dominic Cyr, Martin P. Girardin and Christopher Carcaillet
//International Journal of Wildland Fire 2010, 19, 1–13

#include "SummerMoisture.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	enum TOutput{ O_PRCP, O_TMAX, O_DMC, O_SM, NB_OUTPUTS};

	ERMsg CSummerMoisture::Execute(const CWeatherStation& weather, CModelStatVector& output)
	{
		ERMsg msg;
		
		output.Init(weather.GetEntireTPeriod().as(CTM::MONTHLY), NB_OUTPUTS);
		

		//for all years
		double mdc_old = 15;
		double P = 0;
		for (size_t y = 0; y < weather.GetNbYears() && msg; y++)
		{
			if (!m_bOverwinter)
				mdc_old = 15;

			int year = weather[y].GetTRef().GetYear();

			//for all months
			for (size_t m = 0; m < weather[y].GetNbMonth() && msg; m++)
			{
				double I = 0;

				if (m >= MAY && m <= OCTOBER)
				{
					if (m_bOverwinter)
					{
						if (y > 0 && m == MAY)
						{
							double Qf = 800 * exp(-mdc_old / 400);
							double Qs = m_a * Qf + m_b * 3.94*P;
							mdc_old = max(0.0, 400 * log(800 / Qs));
						}
					}


					I = ComputeIndice(year, m, mdc_old, weather[y][m][H_PRCP][SUM], weather[y][m][H_TMAX][MEAN], false);
				}
				else
				{
					if (m == 10)
						P = 0;

					P += weather[y][m][H_PRCP][SUM];
				}


				output[y * 12 + m][O_PRCP] = weather[y][m][H_PRCP][SUM];
				output[y * 12 + m][O_TMAX] = weather[y][m][H_TMAX][MEAN];
				output[y * 12 + m][O_DMC] = mdc_old;
				output[y * 12 + m][O_SM] = I;
				
			}

		}

		return msg;
	}

	//year	: The year
	//m		: Month index [0..11]
	//MDCo	: Monthly drought code (MDC) at the beginning of the month (this value will be change to the MDC of the beginning of the next month)
	//Rm	: Total rainfall of the month (mm)
	//Tm	: Monthly mean of daily maximum temperature (°C)
	double CSummerMoisture::ComputeIndice(int year, size_t m, double& MDCo, double Rm, double Tm, bool bSouthHemis)
	{
		ASSERT(Rm >= 0);
		ASSERT(Tm >= -50 && Tm <= 50);
		ASSERT(MDCo >= 0);

		static const double Lf_n[12] = { -1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5.0, 2.4, 0.4, -1.6, -1.6 };
		static const double Lf_s[12] = { 6.4, 5.0, 2.4, 0.4, -1.6, -1.6, -1.6, -1.6, -1.6, 0.9, 3.8, 5.8 };
		const double * Lf = bSouthHemis ? Lf_s : Lf_n;
		//Take value of maximum temperature over 0 °C
		Tm = max(0.0, Tm);

		//N is the number of days in the month
		size_t N = GetNbDayPerMonth(year, m);

		//Em is the potential evaporation for the month [equation 2]
		double Em = max(0.0, N*(0.36*Tm + Lf[m]));
		ASSERT(Em >= 0);

		// add half of drying before the rain influence [equation 3]
		double DChalf = MDCo + 0.25*Em;
		ASSERT(DChalf >= 0);

		double RMeff = 0.83*Rm;
		ASSERT(RMeff >= 0);

		//rain effect on the drought code [equation 4]
		double Qmr = (3.937*RMeff) / (800 * exp(-MDCo / 400));
		ASSERT(Qmr >= 0);

		//Drying taking place over the middle of the month (DCmr) [equation 5]
		double DCmr = (Rm <= 2.8) ? DChalf : max(0.0, DChalf - 400 * log(1 + Qmr));
		ASSERT(DCmr >= 0);

		//Estimated of drought code (MDC) at the end of the month [equation 6]
		double MDCm = DCmr + 0.25*Em;
		ASSERT(MDCm >= 0);


		//Finally, MDC0 and MDCm are averaged to find a mean drought value for the month [equation 7]
		double MDC = (MDCo + MDCm) / 2;
		ASSERT(MDC >= 0);

		//replace the the MDC of the beginnig of the month by the MDC at the end of the month
		MDCo = MDCm;

		return MDC;

	}
}

