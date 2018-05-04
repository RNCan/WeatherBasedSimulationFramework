#pragma once

#include "Basic/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/ModelStat.h"
#include "Basic/WeatherDefine.h"

namespace WBSF
{


	class CBlueStainVariables
	{
	public:

		
		enum TVariable{ V_TMIN_EXT, V_TMEAN, V_TMAX_EXT, V_PRCP, V_WARMQ_TMEAN, V_COLDQ_TMEAN, V_WETQ_TMEAN, V_DRYQ_TMEAN, V_WARMQ_PRCP, V_COLDQ_PRCP, V_WETQ_PRCP, V_DRYQ_PRCP, V_AI, V_WARMQ_AI, V_COLDQ_AI, V_WETQ_AI, V_DRYQ_AI, V_WARMM_TMEAN, V_COLDM_TMEAN, V_WETM_PRCP, V_DRYM_PRCP, V_SUMMER_DD5, NB_VARIABLES };
		enum TExtrem{ WETTEST, DRIEST, WARMEST, COLDEST, NB_EXTREM};
		typedef std::array < CTPeriod, NB_EXTREM> ExtremPeriods;
		typedef std::array< size_t, NB_EXTREM> ExtremMonths;
			

		static const char* VAR_NAME[NB_VARIABLES];


		CTPeriod CBlueStainVariables::GetExtremQuarter(const CWeatherYear& weather, TExtrem e, bool bLoop = false);
		static size_t GetExtremMonth(const CWeatherStation& weather, TExtrem e);
		static size_t GetExtremMonth(const CWeatherYear& weather, TExtrem e);



		static CStatistic GetNormalStat(const CWeatherStation& weather, size_t m, TExtrem e);
		static void GetSummerDD5(CWeatherStation& weather, CModelStatVector& ouptut);
		static void GetWaterDeficit(const CWeatherStation& weather, CModelStatVector& ouptut);
		static double GetAridity(const CModelStatVector& AR, CTPeriod p = CTPeriod(), bool bLimitToZero = false);
		
		void Execute(CWeatherStation& weather, CModelStatVector& output);
		
	};

}