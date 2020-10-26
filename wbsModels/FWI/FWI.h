#pragma once

#include <wtypes.h>
#include <map>
#include <array>
#include "Basic/ERMsg/ERMsg.h"
//#include "Basic/WeatherStation.h"
#include "Basic/modelStat.h"
#include "ModelBase\InputParam.h"


namespace WBSF
{
	class CWeatherStation;
	class CModelStatVector;
	class CWeatherDay;
	class CWeatherYear;

	enum TFWIInitValue{ FWI_START_DATE, FWI_FFMC, FWI_DMC, FWI_DC, FWI_END_DATE, NB_FWI_INPUT };
	class CInitialValues : public std::map<std::string, std::array<double, NB_FWI_INPUT> >
	{
	public:
		ERMsg Load(const std::string& data);
	};

	class CFWIStat : public CModelStat
	{
	public:
		enum TDailyStat{ TMEAN_NOON, RELH_NOON, WNDS_NOON, WNDD_NOON, PRCP, FFMC, DMC, DC, ISI, BUI, FWI, DSR, NB_D_STAT };
		enum TMonthlyStat{ NUM_VALUES = NB_D_STAT, TMEAN_MIN, RELH_MIN, WNDS_MIN, WNDD_MIN, NB_DAY_CONS_NO_PRCP, FFMC_MIN, DMC_MIN, DC_MIN, ISI_MIN, BUI_MIN, FWI_MIN, DSR_MIN, TMEAN_MAX, RELH_MAX, WNDS_MAX, WNDD_MAX, NB_DAY_NO_PRCP, FFMC_MAX, DMC_MAX, DC_MAX, ISI_MAX, BUI_MAX, FWI_MAX, DSR_MAX, NB_M_STAT };
		enum TAnnualStat{ FIRST_FWI_DAY = NB_M_STAT, LAST_FWI_DAY, SNOW_MELT, SNOW_FALL, NB_A_STAT };

		static void Covert2D(const CModelStatVector& result, CModelStatVector& resultD);
		static void Covert2M(const CModelStatVector& result, CModelStatVector& resultM);
		static void Covert2A(const CModelStatVector& result, CModelStatVector& resultA);
		//static long GetNbDayWithoutPrcp(const CModelStatVector& result, const CTPeriod& p, bool bConsecutive);

	};

	typedef CModelStatVectorTemplate<CFWIStat::NB_D_STAT> CFWIDStatVector;
	typedef CModelStatVectorTemplate<CFWIStat::NB_M_STAT> CFWIMStatVector;
	typedef CModelStatVectorTemplate<CFWIStat::NB_A_STAT> CFWIAStatVector;


	class CFWI
	{
	public:

		enum TVanWagner { VAN_WAGNER_1977, VAN_WAGNER_1987 };
		enum TMethod{NOON_CALCULATION, ALL_HOURS_CALCULATION, NB_METHODS};
		static const int MISSING = -9999;


		CFWI();
		~CFWI();


		void Reset();

		ERMsg Execute(const CWeatherStation& weather, CModelStatVector& output);

		static double GetFFMC(double oldFFMC, double T, double Hr, double Ws, double prcp);
		static double GetHFFMC(double oldFFMC, double T, double Hr, double Ws, double prcp, TVanWagner VanWagnerType= VAN_WAGNER_1987);
		static double GetDMC(double oldDMC, size_t m, double T, double Hr, double prcp);
		static double GetDC(double oldDC, double lat, size_t m, double T, double prcp);
		static double GetHISI(double Fo, double Ws, TVanWagner VanWagnerType = VAN_WAGNER_1987, bool fbpMod = false);
		static double GetISI(double ffmc, double Ws, bool fbpMod = false);
		static double GetBUI(double dmc, double dc);
		static double GetFWI(double bui, double isi);
		static double GetDSR(double fwi);
		
		static double GetNoonToNoonPrcp(const CWeatherDay& day);
		static size_t GetNbDayLastRain(const CWeatherYear& weather, size_t firstDay);

		size_t GetInitialValue(const CWeatherStation& weather, size_t y, size_t lastDay, double& FFMC, double& DMC, double& DC);

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
		CInitialValues m_init_values;

		//
		double m_carryOverFraction;//Carry over fraction (a)
		double m_effectivenessOfWinterPrcp;//Effectiveness of winter (b)
		
		TVanWagner m_VanWagnerType;
		bool m_fbpMod;

	};

}