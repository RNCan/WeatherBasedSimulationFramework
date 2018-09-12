//Modifications
// 26/06/2018	Rémi Saint-Amant	Compile with VS2017
// 27/01/2010 	Rémi Saint-Amant	Incorporate in BioSIMModelBase
//**********************************************************************
#include "PlantHardiness.h"
#include "Basic/WeatherDefine.h"
#include "Basic/UtilMath.h"
#include "Basic/GrowingSeason.h"


namespace WBSF
{

	using namespace HOURLY_DATA;



	CStatistic GetNormalStat(const CWeatherStation& weather, size_t m, TVarH v)
	{
		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
			stat += weather[y][m][v];

		return stat;
	}


	size_t GetColdestMonth(const CWeatherStation& weather)
	{
		size_t index = -1;
		double T = 999;
		for (size_t m = 0; m < 12; m++)
		{
			
			CStatistic stat = GetNormalStat(weather, m, H_TMIN2);

			if (stat[MEAN] < T)
			{
				T = stat[MEAN];
				index = m;
			}
		}

		_ASSERT(index < 12);
		return index;
	}

	size_t GetWarmerMonth(const CWeatherStation& weather)
	{
		size_t index = -1;
		double T = -999;
		for (size_t m = 0; m < 12; m++)
		{
			CStatistic stat = GetNormalStat(weather, m, H_TMAX2);

			if (stat[MEAN] > T)
			{
				T = stat[MEAN];
				index = m;
			}
		}

		_ASSERT(index < 12);
		return index;
	}

	double GetMeanFrosFreePeriod(const CWeatherStation& weather)
	{
		CGrowingSeason gs(CGSInfo::TT_TMIN, 1, 0, CGSInfo::TT_TMIN, 1, 0);

		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
		{
			CTPeriod p = gs.GetFrostFreePeriod(weather[y]);
			stat += p.GetNbRef();
		}

		return stat[MEAN];
	}

	double GetMeanMaximumSnowDepth(const CWeatherStation& weather)
	{
		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
			stat += weather[y].GetStat(H_SNDH)[HIGHEST];

		return stat[MEAN];
	}

	double GetJuneNovemberRain(const CWeatherStation& weather)
	{
		CTPeriod JuneNovember(weather.GetFirstYear(), JUNE, FIRST_DAY, weather.GetLastYear(), NOVEMBER, LAST_DAY, CTPeriod::YEAR_BY_YEAR);
		double prcp = weather.GetStat(H_PRCP, JuneNovember)[SUM];
		double snow = weather.GetStat(H_SNOW, JuneNovember)[SUM];

		return (prcp - snow) / weather.size();
	}
	//it's the rainfall and not snowfall
	double GetJanuaryRainfall(const CWeatherStation& weather)
	{
		double prcp = GetNormalStat(weather, JANUARY, H_PRCP)[MEAN];
		double snow = GetNormalStat(weather, JANUARY, H_SNOW)[MEAN];

		return prcp - snow;
	}

	double GetSuitabilityIndex(const CWeatherStation& weather)
	{
		//cm = coldest month
		size_t cm = GetColdestMonth(weather);
		size_t wm = GetWarmerMonth(weather);


		//Y = estimated index of suitability 
		//X1 = monthly mean of the daily minimum temperatures (°C) of the coldest month
		//X2 = mean frost free period above 0°C in days
		//X3 = amount of rainfall (R) from June to November, inclusive, in terms of R/(R+a) where a=25.4 if R is in millimeters and a=1 if R is in inches
		//X4 = monthly mean of the daily maximum temperatures (°C) of the warmest month
		//X5 = winter factor expressed in terms of (0°C - X1)Rjan where Rjan represents the rainfall in January expressed in mm
		//X6 = mean maximum snow depth in terms of S/(S+a) where a=25.4 if S is in millimeters and a=1 if S is in inches
		//X7 = maximum wind gust in (km/hr) in 30 years
		double X1 = GetNormalStat(weather, cm, H_TMIN2)[MEAN];
		double X2 = GetMeanFrosFreePeriod(weather);//(double)weather.GetFrostFreeDay()/weather.GetNbYear();
		double P = GetJuneNovemberRain(weather);
		double X3 = P / (P + 25.4);
		double X4 = GetNormalStat(weather, wm, H_TMAX2)[MEAN];
		double Pjan = GetJanuaryRainfall(weather);
		double X5 = (0 - X1)*Pjan;
		double N = GetMeanMaximumSnowDepth(weather) * 10;//cm --> mm
		double X6 = N / (N + 25.4);
		double X7 = std::min(180.0, weather.GetStat(H_WNDS)[HIGHEST]*2.5);//???max gust is 2.5 time the maximum daily mean wind speed (from wikipedia 2.27 -  2.75)

		//X1 = température minimale quotidienne moyenne (°C) du mois le plus froid
		//X2 = nombre annuel moyen de jours sans gel (au-dessus de 0°C)
		//X3 = précipitations (P) de juin à novembre inclusivement, transformées selon P/(P+a) où a=25.4 si P est exprimé en mm et a=1 if P est exprimé en pouces
		//X4 = température maximale quotidienne moyenne (°C) du mois le plus chaud
		//X5 = facteur hivernal donné par (0°C - X1)Pjan où Pjan représente précipitations de janvier (mm)
		//X6 = épaisseur maximale moyenne du manteau neigeux (N), transformée selon N/(N+a) où a=25.4 si N est exprimé en mm et a=1 if Nest exprimé en pouces
		//X7 = rafale maximale (km h-1) sur une période de 30 ans

		double Y = -67.62 + 1.734*X1 + 0.1868*X2 + 69.77*X3 + 1.256*X4 + 0.006119*X5 + 22.37*X6 - 0.01832*X7;
		//double Y2 = -83.04 + 1.867*X1 + 0.1569*X2 + 113.0*X3 + 1.297*X4;


		return std::max(0.0, Y);

	}

	void CPlantHardiness::Compute(const CWeatherStation& weather, CModelStatVector& result)
	{
		ASSERT(weather.size() > 20);


		result.Init(CTPeriod(CTRef(CTRef::ANNUAL, 0, 0, 0, 0), CTRef(CTRef::ANNUAL, 0, 0, 0, 0)), 2);
		double SI = GetSuitabilityIndex(weather);
		int CZ = int(SI / 10.0);
		int type = Round( (SI - CZ * 10)/10.0, 0); //zone a (0) or zone b (1)

		result[0][0] = Round(SI, 1);
		result[0][1] = CZ + type*0.5;

	}

}