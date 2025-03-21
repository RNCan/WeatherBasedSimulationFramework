//Modifications
// 26/06/2018	R�mi Saint-Amant	Compile with VS2017
// 27/01/2010 	R�mi Saint-Amant	Incorporate in BioSIMModelBase
//**********************************************************************
#include "PlantHardinessCanada.h"
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


	size_t CPlantHardinessCanada::GetColdestMonth(const CWeatherStation& weather)
	{
		size_t index = -1;
		double T = 999;
		for (size_t m = 0; m < 12; m++)
		{
			CStatistic stat = GetNormalStat(weather, m, H_TMIN);
			if (stat[MEAN] < T)
			{
				T = stat[MEAN];
				index = m;
			}
		}

		_ASSERT(index < 12);
		return index;
	}

	size_t CPlantHardinessCanada::GetWarmerMonth(const CWeatherStation& weather)
	{
		size_t index = -1;
		double T = -999;
		for (size_t m = 0; m < 12; m++)
		{
			CStatistic stat = GetNormalStat(weather, m, H_TMAX);

			if (stat[MEAN] > T)
			{
				T = stat[MEAN];
				index = m;
			}
		}

		_ASSERT(index < 12);
		return index;
	}

	double CPlantHardinessCanada::GetMeanFrosFreePeriod(const CWeatherStation& weather)
	{
		//CGrowingSeason gs(CGSInfo::TT_TMIN, 1, 0, CGSInfo::TT_TMIN, 1, 0);
		CFrostFreePeriod FF;

		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
		{
			CTPeriod p = FF.GetPeriod(weather[y]);
			stat += p.GetNbRef();
		}

		return stat[MEAN];
	}

	double CPlantHardinessCanada::GetMeanMaximumSnowDepth(const CWeatherStation& weather)
	{
		CStatistic stat;
		for (size_t y = 0; y < weather.size(); y++)
			stat += weather[y].GetStat(H_SNDH)[HIGHEST];

		return stat[MEAN];
	}

	double CPlantHardinessCanada::GetJuneNovemberRain(const CWeatherStation& weather)
	{
		CTPeriod JuneNovember(weather.GetFirstYear(), JUNE, FIRST_DAY, weather.GetLastYear(), NOVEMBER, LAST_DAY, CTPeriod::YEAR_BY_YEAR);
		double prcp = weather.GetStat(H_PRCP, JuneNovember)[SUM];
		double snow = weather.GetStat(H_SNOW, JuneNovember)[SUM];

		return (prcp - snow) / weather.size();
	}
	//it's the rainfall and not snowfall
	double CPlantHardinessCanada::GetJanuaryRainfall(const CWeatherStation& weather)
	{
		double prcp = GetNormalStat(weather, JANUARY, H_PRCP)[MEAN];
		double snow = GetNormalStat(weather, JANUARY, H_SNOW)[MEAN];

		return prcp - snow;
	}

	//X1 = temp�rature minimale quotidienne moyenne (�C) du mois le plus froid
	//X2 = nombre annuel moyen de jours sans gel (au-dessus de 0�C)
	//X3 = pr�cipitations (P) de juin � novembre inclusivement, transform�es selon P/(P+a) o� a=25.4 si P est exprim� en mm et a=1 if P est exprim� en pouces
	//X4 = temp�rature maximale quotidienne moyenne (�C) du mois le plus chaud
	//X5 = facteur hivernal donn� par (0�C - X1)Pjan o� Pjan repr�sente pr�cipitations de janvier (mm)
	//X6 = �paisseur maximale moyenne du manteau neigeux (N), transform�e selon N/(N+a) o� a=25.4 si N est exprim� en mm et a=1 if Nest exprim� en pouces
	//X7 = rafale maximale (km h-1) sur une p�riode de 30 ans

	//X1 = monthly mean of the daily minimum temperatures (�C) of the coldest month
	//X2 = mean frost free period above 0�C in days
	//X3 = amount of rainfall (R) from June to November, inclusive, in terms of R/(R+a) where a=25.4 if R is in millimeters and a=1 if R is in inches
	//X4 = monthly mean of the daily maximum temperatures (�C) of the warmest month
	//X5 = winter factor expressed in terms of (0�C - X1)Rjan where Rjan represents the rainfall in January expressed in mm
	//X6 = mean maximum snow depth in terms of S/(S+a) where a=25.4 if S is in millimeters and a=1 if S is in inches
	//X7 = maximum wind gust in (km/hr) in 30 years
	double CPlantHardinessCanada::GetSuitabilityIndex(const CWeatherStation& weather)
	{
		//cm = coldest month
		size_t cm = GetColdestMonth(weather);
		size_t wm = GetWarmerMonth(weather);
		
		double X1 = GetNormalStat(weather, cm, H_TMIN)[MEAN];
		double X2 = GetMeanFrosFreePeriod(weather);
		double P = GetJuneNovemberRain(weather);
		double X3 = P / (P + 25.4);
		double X4 = GetNormalStat(weather, wm, H_TMAX)[MEAN];
		double Pjan = GetJanuaryRainfall(weather);
		double X5 = (0 - X1)*Pjan;
		double N = GetMeanMaximumSnowDepth(weather) * 10;//cm --> mm
		double X6 = N / (N + 25.4);
		//Approximation of max wind gust. max gust is 2.5 time the maximum daily mean wind speed (from wikipedia: 2.27-2.75)
		//https://en.wikipedia.org/wiki/Wind_speed
		double X7 = std::min(180.0, weather.GetStat(H_WNDS)[HIGHEST]*2.5);
	
		//Y: estimated index of suitability 
		double Y = -67.62 + 1.734*X1 + 0.1868*X2 + 69.77*X3 + 1.256*X4 + 0.006119*X5 + 22.37*X6 - 0.01832*X7;

		return std::max(0.0, Y);
	}

	void CPlantHardinessCanada::Compute(const CWeatherStation& weather, CModelStatVector& result)
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