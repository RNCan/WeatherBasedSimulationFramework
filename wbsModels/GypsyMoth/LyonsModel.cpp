#include "LyonsModel.h"
#include "Basic/WeatherStation.h"

namespace WBSF
{

	const double  CLyonsModel::a = -0.1079;
	const double  CLyonsModel::b = 0.0145;
	const double  CLyonsModel::c = -0.0002;
	const double  CLyonsModel::p0 = 0.7519;
	const double  CLyonsModel::p1 = 2.9442;
	const double  CLyonsModel::p2 = 0.2704;

	CLyonsModel::CLyonsModel(const CGMEggParam& param) : CEggModel(param)
	{}

	ERMsg CLyonsModel::ComputeHatch(const CWeatherStation& weather, const CTPeriod& p)
	{
		ASSERT(p.GetLength() == 730 || p.GetLength() == 731);

		ERMsg message;

		//resize output array
		m_eggState.Init(p.GetLength(), p.Begin());



		//compute the beginning date : must be in the last year
		double  x = 0.0;
		//int		day	= weather.GetDayIndex(weather.GetNbYear()-1, 89, 0);
		//int		day	= CJDayRef(p.GetLastYear(), 89) - weather.GetFirstTRef();
		CTRef day = CJDayRef(p.GetLastYear(), 89);
		ASSERT(weather.GetEntireTPeriod(CTM::DAILY).IsInside(day));

		for (CTRef i = m_eggState.GetFirstTRef(); i < day; i++)
			m_eggState[i][DIAPAUSE] = MAXEGGS;


		//while ((x <= 0.7519) && (day < weather.GetNbDay()))
		while ((x <= 0.7519) && p.IsInside(day))
		{
			m_eggState[day][DIAPAUSE] = MAXEGGS;

			double sum = 0.0;
			CDailyWaveVector t;
			weather.GetDay(day).GetAllenWave(t, 12, 1);
			_ASSERTE(t.size() == 24);

			for (int j = 0; j < 24; j++)
				sum += a + b*t[j] + c*(pow(t[j], 2.0));
			sum /= 24.0;
			if (sum < 0.0)
				sum = 0.0;
			x += sum;
			day++;
		}


		//if (day < weather.GetNbDay()) // emergence occured 
		if (p.IsInside(day))
		{
			day--;
			//hatch.first_hatch = day;
			double f = 1.0 - exp(-pow((x - p0) / p2, p1));
			//m_hatching[day] = (float)(f*100);
			m_eggState[day][HATCHING] = (f * 100);
			double f_previous = f;

			//while ((f < .9995) && (day < weather.GetNbDay()))
			while ((f < .9995) && p.IsInside(day))
			{
				day++;
				double sum = 0.0;
				CDailyWaveVector t;
				weather.GetDay(day).GetAllenWave(t, 12, 1);
				_ASSERTE(t.size() == 24);

				for (int j = 0; j < 24; j++)
					sum += a + b*t[j] + c*(pow(t[j], 2.0));

				sum /= 24.0;
				if (sum < 0.0)
					sum = 0.0;
				x += sum;
				f = 1.0 - exp(-pow((x - p0) / p2, p1));
				double f_today = f * 100;
				//m_hatching[day] = (float)(f_today - f_previous);
				m_eggState[day][HATCHING] = f_today - f_previous;
				f_previous = f_today;

				m_eggState[day][DIAPAUSE] = (1 - f)*MAXEGGS;
				m_eggState[day][HATCH] = f*MAXEGGS;
			}

			//m_hatching[day+1] = 0;

			for (CTRef i = day + 1; i <= m_eggState.GetLastTRef(); i++)
				m_eggState[i][HATCH] = MAXEGGS;
		}

		return message;
	}


}