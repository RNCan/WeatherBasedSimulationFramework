#pragma once

#include "Basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/modelStat.h"

namespace WBSF
{

	class CPlantHardiness
	{
	public:

		CPlantHardiness()
		{}

		void Compute(const CWeatherStation& weather, CModelStatVector& result);

	};

}