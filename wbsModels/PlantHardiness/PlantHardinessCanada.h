#pragma once

#include "Basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/modelStat.h"

namespace WBSF
{

	class CPlantHardinessCanada
	{
	public:

		CPlantHardinessCanada()
		{}

		void Compute(const CWeatherStation& weather, CModelStatVector& result);



		static size_t GetColdestMonth(const CWeatherStation& weather);
		static size_t GetWarmerMonth(const CWeatherStation& weather);
		static double GetMeanFrosFreePeriod(const CWeatherStation& weather);
		static double GetMeanMaximumSnowDepth(const CWeatherStation& weather);
		static double GetJuneNovemberRain(const CWeatherStation& weather);
		static double GetJanuaryRainfall(const CWeatherStation& weather);
		static double GetSuitabilityIndex(const CWeatherStation& weather);

	};

}