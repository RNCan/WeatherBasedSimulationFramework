//Modifications
// 15/01/2020 	Rémi Saint-Amant	Creation
//**********************************************************************
#include "PlantHardinessUSA.h"
#include "Basic/WeatherDefine.h"

using namespace std;

//1990zones
//Updated
//zones
//Temperature range [°C]
// 0a -99.9 1o −53.9
// 0b −53.9 to −51.1 
// 1a -51.1 to -48.3 
// 1b -48.3 to -45.6 
// 2a -45.6 to -42.8 
// 2b -42.8 to -40.0 
// 3a -40.0 to -37.2 
// 3b -37.2 to -34.4 
// 4a -34.4 to -31.7 
// 4b -31.7 to -28.9 
// 5a -28.9 to -26.1 
// 5b -26.1 to -23.3 
// 6a -23.3 to -20.6 
// 6b -20.6 to -17.8 
// 7a -17.8 to -15.0
// 7b -15.0 to -12.2
// 8a -12.2 to -9.4 
// 8b  -9.4 to -6.7
// 9a  -6.7 to -3.9
// 9b  -3.9 to -1.1
//10a  -1.1 to  1.7 
//10b   1.7 to  4.4
//11a   4.4 to  7.2
//11b   7.2 to 10.0
//12a  10.0 to 12.8
//12b  12.8 to 15.6
//13a  15.6 to 18.3
//13b  18.3 to 21.1

double GetZoneIndex(double T)
{
	//zone are base on °F
	//°C -> °F
	double Tf = max(-70.0, (T * 9.0 / 5.0) + 32.0);

	double index = (Tf - -70);
	return index;
}

namespace WBSF
{

	using namespace HOURLY_DATA;

	
	double CPlantHardinessUSA::GetSuitabilityIndex(const CWeatherStation& weather)
	{
		//cm = coldest month
		int first_year = weather.GetFirstYear();

		CStatistic stat;
		for (size_t y = 0; y < weather.GetNbYears() - 1; y++)
		{
			CStatistic statY;
			CTPeriod p(first_year, JULY, FIRST_DAY, first_year + 1, JUNE, LAST_DAY);
			for (CTRef d = p.Begin(); d <= p.End(); d++)
				statY += weather.GetDay(d)[H_TMIN][MEAN];

			stat += statY[LOWEST];
		}

		ASSERT(stat[NB_VALUE] == weather.GetNbYears() - 1);
		double zone = GetZoneIndex(stat[MEAN]);

		return zone;
	}

	void CPlantHardinessUSA::Compute(const CWeatherStation& weather, CModelStatVector& result)
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