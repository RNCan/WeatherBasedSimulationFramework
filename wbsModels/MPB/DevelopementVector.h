#pragma once

#include <vector>
#include "Basic/WeatherStation.h"
#include "MPBDevRates.h"


namespace WBSF
{


	class CMPBDevelopmentDay
	{
	public:

		double operator[](int s)const{ _ASSERT(s >= 0 && s < NB_STAGES); return m_data[s]; }
		double& operator[](int s){ _ASSERT(s >= 0 && s < NB_STAGES); return m_data[s]; }

	protected:

		double m_data[NB_STAGES];
	};



	class CMPBDevelopmentVector : public std::vector<CMPBDevelopmentDay>
	{
	public:

		CMPBDevelopmentVector(const CRandomGenerator& RG);
		void Init(const CDailyWaveVector& T);
		int GetNextStageDay(int firstDay, int s, double threshold)const;
		CMPBDevelopmentTable MPB_RATES_TABLE;
	};

}