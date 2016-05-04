#pragma once

#include <wtypes.h>
#include "ERMsg/ERMsg.h"
#include "Basic/WeatherStation.h"
#include "Basic/modelStat.h"
#include "ModelBase\InputParam.h"


namespace WBSF
{

	class CFWIStat : public CModelStat
	{
	public:
		enum TDailyStat{ TMEAN_NOON, RELH_NOON, WNDS_NOON, PRCP, FFMC, DMC, DC, ISI, BUI, FWI, DSR, NB_D_STAT };
		enum TMonthlyStat{ NUM_VALUES = NB_D_STAT, TMEAN_MIN, RELH_MIN, WNDS_MIN, NB_DAY_CONS_NO_PRCP, FFMC_MIN, DMC_MIN, DC_MIN, ISI_MIN, BUI_MIN, FWI_MIN, DSR_MIN, TMEAN_MAX, RELH_MAX, WNDS_MAX, NB_DAY_NO_PRCP, FFMC_MAX, DMC_MAX, DC_MAX, ISI_MAX, BUI_MAX, FWI_MAX, DSR_MAX, NB_M_STAT };
		enum TAnnualStat{ FIRST_FWI_DAY = NB_M_STAT, LAST_FWI_DAY, SNOW_MELT, SNOW_FALL, NB_A_STAT };

		static void CovertD2M(const CModelStatVector& resultD, CModelStatVector& resultM);
		static void CovertD2A(const CModelStatVector& resultD, CModelStatVector& resultA);
		static long GetNbDayWithoutPrcp(const CModelStatVector& result, const CTPeriod& p, bool bConsecutive);

	};

	typedef CModelStatVectorTemplate<CFWIStat::NB_D_STAT> CFWIDStatVector;
	typedef CModelStatVectorTemplate<CFWIStat::NB_M_STAT> CFWIMStatVector;
	typedef CModelStatVectorTemplate<CFWIStat::NB_A_STAT> CFWIAStatVector;


	class CFWI
	{
	public:

		enum TMethod{NOON_CALCULATION, ALL_HOURS_CALCULATION, NB_METHODS};
		static const int MISSING = -9999;


		CFWI();
		~CFWI();


		void Reset();

		void Execute(const CWeatherStation& weather, CModelStatVector& output);
		//const CFWIDStatVector& GetResult()const{ return m_dailyResult; }

		static double GetFFMC(double ffmc_old, const CHourlyData& day);
		static double GetFFMC(double oldFFMC, double T, double Hr, double Ws, double prcp);

		static double GetDMC(double dmc_old, const CHourlyData& day);
		static double GetDMC(double oldDMC, size_t m, double T, double Hr, double prcp);

		static double GetDC(double dc_old, const CHourlyData& day);
		static double GetDC(double oldDC, size_t m, double T, double prcp);

		static double GetISI(double ffmc, const CHourlyData& day);
		static double GetISI(double ffmc, double Ws);

		static double GetBUI(double dmc, double dc);
		static double GetFWI(double bui, double isi);
		static double GetDSR(double fwi);


		short GetInitialValue(const CWeatherStation& weather, size_t y, size_t lastDay, double& FFMC, double& DMC, double& DC);

		CTRef GetFirstDay(const CWeatherYear& weather);
		CTRef GetLastDay(const CWeatherYear& weather);

		TMethod m_method;
		bool m_bAutoSelect;

		//auto-select 
		size_t m_nbDaysStart;
		size_t m_TtypeStart;
		double m_thresholdStart;//Noon temperature threshold (C°) for starting date: 12 C° by default

		//day of the end
		size_t m_nbDaysEnd;
		size_t m_TtypeEnd;
		double m_thresholdEnd;

		//manual 
		CMonthDay m_firstDay;
		CMonthDay m_lastDay;
		double m_FFMC;
		double m_DMC;
		double m_DC;

		//
		double m_carryOverFraction;//Carry over fraction (a)
		double m_effectivenessOfWinterPrcp;//Effectiveness of winter (b)

	private:

		static size_t GetNbDayLastRain(const CWeatherYear& weather, size_t firstDay);

	};

}