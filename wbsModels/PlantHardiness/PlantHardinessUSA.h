#pragma once

#include "Basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/modelStat.h"

namespace WBSF
{

	class CPlantHardinessUSA
	{
	public:

		CPlantHardinessUSA()
		{}

		void Compute(const CWeatherStation& weather, CModelStatVector& result);


		static double GetSuitabilityIndex(const CWeatherStation& weather);
	};

}