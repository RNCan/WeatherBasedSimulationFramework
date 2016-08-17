#pragma once

#include <vector>
#include "Basic/WeatherStation.h"
#include "MPBDevRates.h"


namespace WBSF
{


	class CMPBDevelopmentDay
	{
	public:

		double operator[](size_t s)const{ _ASSERT(s < NB_STAGES); return m_data[s]; }
		double& operator[](size_t s){ _ASSERT(s < NB_STAGES); return m_data[s]; }

	protected:

		double m_data[NB_STAGES];
	};



	class CMPBDevelopmentVector : public std::vector<CMPBDevelopmentDay>
	{
	public:

		CMPBDevelopmentVector(const CRandomGenerator& RG);
		void Init(const CDailyWaveVector& T);
		size_t GetNextStageDay(size_t firstDay, size_t s, double threshold)const;
		CMPBDevelopmentTable MPB_RATES_TABLE;
	};

}