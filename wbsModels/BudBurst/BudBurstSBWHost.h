#pragma once


#include "Basic/ModelStat.h"
#include "Basic/WeatherStation.h"

namespace WBSF
{

	enum THBBOutput{ O_SUGAR, O_STARCH, O_MERISTEMS, O_BRANCH, O_NEEDLE, O_C, O_INHIBITOR, O_BUDBURST, NB_HBB_OUTPUTS};
	


	class CSBWHostBudBurst 
	{
	public:

		

		CSBWHostBudBurst();
		~CSBWHostBudBurst();


		size_t m_species;
		
		std::map<int, double> m_defioliation;
		ERMsg Execute(CWeatherStation& weather, CModelStatVector& output);

		
	};

}

