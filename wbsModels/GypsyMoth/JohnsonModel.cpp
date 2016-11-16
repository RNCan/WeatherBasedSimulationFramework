#include "JohnsonModel.h"
#include "Basic/WeatherStation.h"
#include "Basic/UtilMath.h"
#include <algorithm>

using namespace std;

namespace WBSF
{


	// the look up table of emergence proportions 
	const double  CJohnsonModel::EMERGE_TABLE[28][11] =
	{
		0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
		0.020, 0.020, 0.020, 0.020, 0.010, 0.010, 0.005, 0.010, 0.010, 0.010, 0.005,
		0.050, 0.050, 0.060, 0.070, 0.050, 0.030, 0.020, 0.040, 0.040, 0.040, 0.020,
		0.090, 0.080, 0.110, 0.170, 0.160, 0.100, 0.090, 0.160, 0.230, 0.300, 0.290,
		0.130, 0.130, 0.170, 0.340, 0.300, 0.260, 0.240, 0.410, 0.530, 0.700, 0.740,
		0.180, 0.200, 0.250, 0.450, 0.460, 0.510, 0.500, 0.580, 0.700, 0.860, 0.890,
		0.230, 0.280, 0.340, 0.540, 0.600, 0.650, 0.700, 0.710, 0.800, 0.940, 0.960,
		0.310, 0.380, 0.440, 0.630, 0.700, 0.760, 0.820, 0.810, 0.880, 0.980, 0.990,
		0.390, 0.480, 0.540, 0.710, 0.780, 0.830, 0.890, 0.900, 0.950, 1.000, 1.000,
		0.460, 0.570, 0.620, 0.760, 0.830, 0.880, 0.920, 0.940, 0.980, 1.000, 1.000,
		0.520, 0.650, 0.690, 0.800, 0.870, 0.920, 0.950, 0.970, 1.000, 1.000, 1.000,
		0.570, 0.720, 0.750, 0.840, 0.900, 0.950, 0.980, 0.990, 1.000, 1.000, 1.000,
		0.620, 0.770, 0.790, 0.870, 0.930, 0.970, 0.990, 1.000, 1.000, 1.000, 1.000,
		0.670, 0.820, 0.830, 0.890, 0.950, 0.980, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.710, 0.850, 0.860, 0.910, 0.970, 0.990, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.750, 0.880, 0.890, 0.925, 0.980, 0.995, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.790, 0.900, 0.910, 0.940, 0.990, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.820, 0.930, 0.930, 0.955, 0.995, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.860, 0.950, 0.950, 0.970, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.900, 0.970, 0.970, 0.985, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.930, 0.980, 0.990, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.945, 0.990, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.960, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.975, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		0.990, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
		1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000, 1.000
	};

	CJohnsonModel::CJohnsonModel(const CGMEggParam& param) : CEggModel(param)
	{}


	static double GetDD(const CWeatherDay& wDay, double threashold = 3.0)
	{
		double Tair = (wDay[HOURLY_DATA::H_TMIN2][MEAN] + wDay[HOURLY_DATA::H_TMAX2][MEAN])/2.0;
		return max(0.0, Tair - threashold);
	}

	ERMsg CJohnsonModel::ComputeHatch(const CWeatherStation& weather, const CTPeriod& p)
	{
		ASSERT(p.GetLength() == 730 || p.GetLength() == 731);

		ERMsg message;

		//resize output array
		m_eggState.Init(p.GetLength(), p.Begin());


		double day_deg_sum = 0;

		CTRef d(p.GetLastYear(), 0, 0);
		while ((day_deg_sum < 282.0) && p.IsInside(d))//(fd < p.GetNbDay()))
		{
			//day_deg_sum += dayDeg[fd];
			day_deg_sum += GetDD(weather.GetDay(d), 3.0);
			d++;
		}

		CTRef hatch_day = d - 1;

		//Reset day_deg_sum so that in the accumulation loop it starts where it should
		day_deg_sum -= GetDD(weather.GetDay(hatch_day), 3.0);
		day_deg_sum -= 282;//??

		//fd = weather.GetDayIndex(weather.GetNbYear()-2, 273, 0);
		//fd = p.Begin()+273;

		int num_under_5 = 0;
		// calculate the number of cold days for the look up table 
		for (CTRef i = p.Begin() + 273; i <= hatch_day; i++)
		{
			double Tair = (weather.GetDay(i)[HOURLY_DATA::H_TMIN2][MEAN] + weather.GetDay(i)[HOURLY_DATA::H_TMAX2][MEAN]) / 2.0;
			if (Tair <= 5.0)
				num_under_5++;
		}

		// add up the new year up to the first hatch day 
		//RSA: cette seccion a été enlever et mis dans la section précédente car le calcul du DD n'est plus fait comme
		//avant. Avant on utilisati un calcul de DD sur 2 jour et maintant on utilise un DD sur une seul journée.
		//for (CTRef i=firstDay; i<=hatch_day; i++) 
		//{
		//	
		//	if (weather.GetDay(i).GetDD(3.0)+3 <= 5.0) //vraiment étrange comme calcul???
		//		//if(weather.GetDay(i).GetTMean() <= 5.0)  ca ne devrai pas être cela à la place???
		//		num_under_5++;
		//}

		// set the column by how many days where under or equal to 5.0 C 
		short day_deg_5 = GetCol(num_under_5);

		// actually build the initialization for the simulation run 
		double prev_day_den = 0.0;

		//revoir le code pour tenir compte 
		//des simulations avec plusieurs années
		//_ASSERTE( param.GetOvipDate() >= 0 && param.GetOvipDate() < 366);
		//int ovipDate = weather.GetDayIndex(weather.GetNbYear()-2, param.GetOvipDate()%365, 0);
		//int ovipDate = m_param.m_ovipDate-p.Begin();
		//for (int i=ovipDate; i<hatch_day; i++)
		for (CTRef i = m_eggState.GetFirstTRef(); i < hatch_day; i++)
			m_eggState[i][DIAPAUSE] = MAXEGGS;

		for (CTRef i = hatch_day; i <= p.End(); i++)
		{
			//day_deg_sum += dayDeg[i];
			day_deg_sum += GetDD(weather.GetDay(i), 3.0);

			int j = (int)(day_deg_sum / 23.0);

			double remainder = day_deg_sum - ((double)j * 23.0);

			double proportions = 0;
			if (remainder == 0)
				proportions = EMERGE_TABLE[j][day_deg_5];
			else
				proportions = EMERGE_TABLE[j][day_deg_5] + ((EMERGE_TABLE[j + 1][day_deg_5] - EMERGE_TABLE[j][day_deg_5])*remainder / 23.0);

			double density = proportions * 100;//density in percent
			_ASSERTE(density >= 0 && density <= 100);
			density -= prev_day_den;
			prev_day_den += density;

			m_eggState[i][DIAPAUSE] = (1 - proportions)*MAXEGGS;
			m_eggState[i][HATCH] = proportions*MAXEGGS;

			if (prev_day_den > 0.0)
			{
				//m_hatching[i] = __max(0.f,(float)density);
				m_eggState[i][HATCHING] = max(0.f, (float)density);
			}

			if (proportions >= 1.0)
			{
				for (CTRef j = i + 1; j <= m_eggState.GetLastTRef(); j++)
					m_eggState[j][HATCH] = MAXEGGS;

				break;
			}

		}

		return message;
	}


	short CJohnsonModel::GetCol(short num_under_5)
	{
		//short day_deg_5 = 0;

		//if (num_under_5 < 80)
		//	day_deg_5 = 0;
		//else if (num_under_5 < 90)
		//	day_deg_5 = 1;
		//else if (num_under_5 < 100)
		//	day_deg_5 = 2;
		//else if (num_under_5 < 110)
		//	day_deg_5 = 3;
		//else if (num_under_5 < 120)
		//	day_deg_5 = 4;
		//else if (num_under_5 < 125)
		//	day_deg_5 = 5;
		//else if (num_under_5 < 130)
		//	day_deg_5 = 6;
		//else if (num_under_5 < 140)
		//	day_deg_5 = 7;
		//else if (num_under_5 < 147)
		//	day_deg_5 = 8;
		//else if (num_under_5 < 160)
		//	day_deg_5 = 9;
		//else
		//	day_deg_5 = 10;

		const short DAY5[11] = { 0, 80, 90, 100, 110, 120, 125, 130, 140, 147, 160 };

		int col = 10;
		for (int i = 10; i >= 0; i--)
		{
			if (num_under_5 >= DAY5[i])
			{
				col = i;
				break;
			}
		}

		//	_ASSERTE( day_deg_5 == col);
		return col;
	}

}

