//**********************************************************************
// 21/01/2016	3.0.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 27/11/2014			Rémi Saint-Amant	Compiled 64 bits with new framework
// 05/04/2013			Rémi Saint-Amant	Remove DD and ET from this model
// 26/02/2013			Rémi Saint-Amant	Add hourly model
// 29/06/2010			Rémi Saint-Amant	Compatible with HxGrid. Remove extrem
// 30/10/2009			Rémi Saint-Amant	Change CPeriod by CTPeriod
// 03/03/2009			Rémi Saint-Amant	Integrate with new BioSIMModelBase (hxGrid)
// 19/11/2008			Rémi Saint-Amant	Update with VS9 and new BioSIMModelBase 
// 27/05/2008			Rémi Saint-Amant	Used of wind speed in the computation of ASC2000 PET
// 01/12/2002			Rémi Saint-Amant	2 variables was added: Degree-day and % of snow
// 15/07/2002			Rémi Saint-Amant	Creation
//**********************************************************************

#include <iostream>
#include "Basic/Evapotranspiration.h"
#include "Basic/UtilTime.h"
#include "Basic/UtilMath.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"
#include "ClimaticFull.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred = CModelFactory::RegisterModel(CClimaticModel::CreateObject);


	//Saturation vapor pressure at daylight temperature
	static double GetDaylightVaporPressureDeficit(const CWeatherYear& weather);
	static double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather);
	static double GetDaylightVaporPressureDeficit(const CWeatherDay& weather);

	static double GetNbDayWithPrcp(const CWeatherYear& weather);
	static double GetNbDayWithPrcp(const CWeatherMonth& weather);
	static double GetNbDayWithPrcp(const CWeatherDay& weather);

	static double GetNbFrostDay(const CWeatherYear& weather);
	static double GetNbFrostDay(const CWeatherMonth& weather);
	static double GetNbFrostDay(const CWeatherDay& weather);


	//Contructor
	CClimaticModel::CClimaticModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = -1;


		VERSION = "3.0.0 (2016)";

		m_varType = 0;
		m_a[0] = -0.9417;
		m_b[0] = -0.3227;
		m_a[1] = 0.207;
		m_b[1] = 0.205;

		m_x0 = 1;
		m_x1 = 1;
		m_x2 = 1;
		m_x3 = 1;

		m_bInit = false;
	}

	CClimaticModel::~CClimaticModel()
	{}
	
	//ANNUAL_NB_DAY_WITH_PRCP, ANNUAL_NB_DAY_WITHOUT_PRCP, 
	enum TAnnualStat{ ANNUAL_LOWEST_TMIN, ANNUAL_MEAN_TMIN, ANNUAL_MEAN_TMEAN, ANNUAL_MEAN_TMAX, ANNUAL_HIGHEST_TMAX, ANNUAL_PPT, ANNUAL_MEAN_TDEW, ANNUAL_MEAN_REL_HUM, ANNUAL_MEAN_WNDS, ANNUAL_FROST_DAY, ANNUAL_DAYLIGHT_VPD, ANNUAL_MEAN_VPD, ANNUAL_SUN, NB_ANNUAL_STATS };
	enum TMonthlyStat{ MONTHLY_LOWEST_TMIN, MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMEAN, MONTHLY_MEAN_TMAX, MONTHLY_HIGHEST_TMAX, MONTHLY_PPT, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_MEAN_WNDS, MONTHLY_FROST_DAY, MONTHLY_DAYLIGHT_VPD, MONTHLY_MEAN_VPD, MONTHLY_SUN,  NB_MONTHLY_STATS };
	enum TDailyStat{ DAILY_TMIN, DAILY_TMEAN, DAILY_TMAX, DAILY_PPT, DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, DAILY_MEAN_WNDS, DAILY_DAYLIGHT_VPD, DAILY_MEAN_VPD, DAILY_SUN, NB_DAILY_STATS };
	enum THourlyStat{ HOURLY_T, HOURLY_PRCP, HOURLY_TDEW, HOURLY_REL_HUM, HOURLY_WIND_SPEED, HOURLY_VPD, HOURLY_SRAD, NB_HOURLY_STATS };


	typedef CModelStatVectorTemplate<NB_ANNUAL_STATS> CAnnualStatVector;
	typedef CModelStatVectorTemplate<NB_MONTHLY_STATS> CMonthlyStatVector;
	typedef CModelStatVectorTemplate<NB_DAILY_STATS> CDailyStatVector;
	typedef CModelStatVectorTemplate<NB_HOURLY_STATS> CHourlyStatVector;


	ERMsg CClimaticModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//CGrowingSeason GS;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		CAnnualStatVector stat(p);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double daylightVPD = GetDaylightVaporPressureDeficit(m_weather[y]); //[Pa]
			double VPD = m_weather[y].GetStat(H_VPD)[MEAN]; //[Pa]

			double annualMinimum = m_weather[y].GetStat(H_TMIN)[LOWEST];
			double annualMinMean = m_weather[y].GetStat(H_TMIN)[MEAN];
			double annualMean = m_weather[y].GetStat(H_TAIR)[MEAN];
			double annualMaxMean = m_weather[y].GetStat(H_TMAX)[MEAN];
			double annualMaximum = m_weather[y].GetStat(H_TMAX)[HIGHEST];
			double annualPpt = m_weather[y].GetStat(H_PRCP)[SUM];
			double annualSun = m_weather[y].GetStat(H_SRAD)[SUM];
			double annualSnow = m_weather[y].GetStat(H_SNOW)[SUM];
			double Tdew = m_weather[y].GetStat(H_TDEW)[MEAN];
			double relHum = m_weather[y].GetStat(H_RELH)[MEAN];
			double wnds = m_weather[y].GetStat(H_WNDS)[MEAN];

			double frostDay = GetNbFrostDay(m_weather[y]);
			//ASSERT(m_weather[y].GetFrostDay(0) == GetFrostDay(y, 0));

			//CTPeriod growingSeason = GS.GetGrowingSeason(m_weather[y]);
			//CTPeriod frostFreePeriod = GS.GetFrostFreePeriod(m_weather[y]);
			size_t NBDayWithPrcp = GetNbDayWithPrcp(m_weather[y]);// .GetNbDayWith(PRCP, ">1"));
			size_t NBDayWithoutPrcp = m_weather[y].size() - NBDayWithPrcp;

			stat[y][ANNUAL_LOWEST_TMIN] = annualMinimum;
			stat[y][ANNUAL_MEAN_TMIN] = annualMinMean;
			stat[y][ANNUAL_MEAN_TMEAN] = annualMean;
			stat[y][ANNUAL_MEAN_TMAX] = annualMaxMean;
			stat[y][ANNUAL_HIGHEST_TMAX] = annualMaximum;
			stat[y][ANNUAL_PPT] = annualPpt;
			//stat[y][ANNUAL_SNOW] = annualSnow;
			stat[y][ANNUAL_MEAN_TDEW] = Tdew;
			stat[y][ANNUAL_MEAN_REL_HUM] = relHum;
			stat[y][ANNUAL_MEAN_WNDS] = wnds;
			stat[y][ANNUAL_FROST_DAY] = frostDay;
			stat[y][ANNUAL_DAYLIGHT_VPD] = daylightVPD;
			stat[y][ANNUAL_MEAN_VPD] = VPD;
			stat[y][ANNUAL_SUN] = annualSun;
			//stat[y][ANNUAL_NB_DAY_WITH_PRCP] = NBDayWithPrcp;
			//stat[y][ANNUAL_NB_DAY_WITHOUT_PRCP] = NBDayWithoutPrcp;
		}

		SetOutput(stat);

		return msg;
	}

	ERMsg CClimaticModel::OnExecuteMonthly()
	{
		ERMsg msg;

	//	CGrowingSeason GS;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::MONTHLY));
		CAnnualStatVector stat(p);

		//CMonthlyStatVector stat(m_weather.GetNbYear() * 12, CTRef(m_weather[0].GetYear(), CFL::FIRST_MONTH));

		for (size_t y = 0; y<m_weather.GetNbYears(); y++)
		{
			for (size_t m = 0; m<12; m++)
			{
				double daylightVPD = GetDaylightVaporPressureDeficit(m_weather[y][m]); //[Pa]
				double VPD = m_weather[y][m].GetStat(H_VPD)[MEAN]; //[Pa]

				double monthlyMinimum = m_weather[y][m].GetStat(H_TMIN)[LOWEST];
				double monthlyMinMean = m_weather[y][m].GetStat(H_TMIN)[MEAN];
				double monthlyMean = m_weather[y][m].GetStat(H_TAIR)[MEAN];
				double monthlyMaxMean = m_weather[y][m].GetStat(H_TMAX)[MEAN];
				double monthlyMaximum = m_weather[y][m].GetStat(H_TMAX)[HIGHEST];
				double monthlyPpt = m_weather[y][m].GetStat(H_PRCP)[SUM];
				double monthlySun = m_weather[y][m].GetStat(H_SRAD)[SUM];
				//double monthlySnow = m_weather[y][m].GetStat(STAT_SNOW, SUM);
				double Tdew = m_weather[y][m].GetStat(H_TDEW)[MEAN];
				double relHum = m_weather[y][m].GetStat(H_RELH)[MEAN];
				double wnds = m_weather[y][m].GetStat(H_WNDS)[MEAN];
				double frostDay = GetNbFrostDay(m_weather[y][m]);
				//_ASSERTE(frostDay == m_weather[y][m].GetNbDayWith(TMIN, "<0"));
				//int NBDayWithPrcp = m_weather[y][m].GetNbDayWith(PRCP, ">1");
				//int NBDayWithoutPrcp = m_weather[y][m].GetNbDayWith(PRCP, "<=1", true);


				stat[y * 12 + m][MONTHLY_LOWEST_TMIN] = monthlyMinimum;
				stat[y * 12 + m][MONTHLY_MEAN_TMIN] = monthlyMinMean;
				stat[y * 12 + m][MONTHLY_MEAN_TMEAN] = monthlyMean;
				stat[y * 12 + m][MONTHLY_MEAN_TMAX] = monthlyMaxMean;
				stat[y * 12 + m][MONTHLY_HIGHEST_TMAX] = monthlyMaximum;
				stat[y * 12 + m][MONTHLY_PPT] = monthlyPpt;
				//stat[y * 12 + m][MONTHLY_SNOW] = monthlySnow;
				stat[y * 12 + m][MONTHLY_MEAN_TDEW] = Tdew;
				stat[y * 12 + m][MONTHLY_MEAN_REL_HUM] = relHum;
				stat[y * 12 + m][MONTHLY_MEAN_WNDS] = wnds;
				stat[y * 12 + m][MONTHLY_FROST_DAY] = frostDay;
				stat[y * 12 + m][MONTHLY_DAYLIGHT_VPD] = daylightVPD;
				stat[y * 12 + m][MONTHLY_MEAN_VPD] = VPD;
				stat[y * 12 + m][MONTHLY_SUN] = monthlySun;
				//stat[y * 12 + m][MONTHLY_NB_DAY_WITH_PRCP] = NBDayWithPrcp;
				//stat[y * 12 + m][MONTHLY_NB_DAY_WITHOUT_PRCP] = NBDayWithoutPrcp;
			}
		}

		SetOutput(stat);


		return msg;
	}


	ERMsg CClimaticModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CAnnualStatVector stat(p);

		for (size_t y = 0; y<m_weather.size(); y++)
		{
			for (size_t m = 0; m<m_weather[y].size(); m++)
			{
				for (size_t d = 0; d<m_weather[y][m].size(); d++)
				{
					const CWeatherDay& wDay = m_weather[y][m][d];

					double daylightVPD = GetDaylightVaporPressureDeficit(wDay); //Pa
					double VPD = wDay[H_VPD][MEAN];//Pa

					double Tmin = wDay[H_TMIN][MEAN];
					double Tmean = wDay[H_TAIR][MEAN];
					double Tmax = wDay[H_TMAX][MEAN];
					double ppt = wDay[H_PRCP][SUM] >= 0.1 ? wDay[H_PRCP][SUM] : 0;
					//double snow = wDay[SNOW];
					//double snowpack = wDay[SNDH];
					double Tdew = wDay[H_TDEW][MEAN];
					double relHum = wDay[H_RELH][MEAN];
					double wnds = wDay[H_WNDS][MEAN];
					double srad = wDay[H_SRAD][MEAN];

					CTRef ref = m_weather[y][m][d].GetTRef();

					stat[ref][DAILY_TMIN] = Tmin;
					stat[ref][DAILY_TMEAN] = Tmean;
					stat[ref][DAILY_TMAX] = Tmax;
					stat[ref][DAILY_PPT] = ppt;
					//stat[ref][DAILY_SNOW] = snow;
					//stat[ref][DAILY_SNOWPACK] = snowpack;
					stat[ref][DAILY_MEAN_TDEW] = Tdew;
					stat[ref][DAILY_MEAN_REL_HUM] = relHum;
					stat[ref][DAILY_MEAN_WNDS] = wnds;
					stat[ref][DAILY_DAYLIGHT_VPD] = daylightVPD;
					stat[ref][DAILY_MEAN_VPD] = VPD;
					stat[ref][DAILY_SUN] = srad;
				}
			}
		}

		SetOutput(stat);

		return msg;
	}


	ERMsg CClimaticModel::OnExecuteHourly()
	{
		ERMsg msg;

		/*CHourlyStatVector stat;
		GetHourlyStat(stat);
		SetOutput(stat);
		*/
		return msg;
	}


	void CClimaticModel::GetHourlyStat(CModelStatVector& statIn)
	{
		//CHourlyStatVector& stat = (CHourlyStatVector&)statIn;

		//CSun sun(m_info.m_loc.m_lat, m_info.m_loc.m_lon);
		//CWeather weather(m_weather);
		//weather.AjusteMeanForHourlyRequest(m_info.m_loc);

		//stat.Init(weather.GetNbDay() * 24, weather.GetFirstTRef().Transform(CTRef::HOURLY));
		//for (int y = 0; y<weather.GetNbYear(); y++)
		//{
		//	//double DD=0;
		//	for (int m = 0; m<12; m++)
		//	{
		//		for (int d = 0; d<weather[y][m].GetNbDay(); d++)
		//		{

		//			const CWeatherDay& wDay = weather[y][m][d];

		//			CDay hours;
		//			wDay.GetHourlyVar(m_x0, m_x1, m_x2, m_x3, hours, m_info.m_loc);

		//			for (int h = 0; h<24; h++)
		//			{
		//				CTRef ref(weather[y].GetYear(), m, d, h);
		//				stat[ref][HOURLY_T] = hours[h][H_TEMP];
		//				stat[ref][HOURLY_PRCP] = hours[h][H_PRCP];
		//				stat[ref][HOURLY_TDEW] = hours[h][H_TDEW];
		//				stat[ref][HOURLY_REL_HUM] = hours[h][H_RELH];
		//				stat[ref][HOURLY_WIND_SPEED] = hours[h][H_WNDS];
		//				stat[ref][HOURLY_VPD] = hours[h](H_VPD);
		//				stat[ref][HOURLY_SRAD] = hours[h][H_SRAD];

		//				HxGridTestConnection();
		//			}
		//		}
		//	}
		//}
	}

	//ERMsg CClimaticModel::OnExecuteAnnual()
	//{
	//	ERMsg msg;
	//
	//	CAnnualStatVector stat(m_weather.GetNbYear(), CTRef(m_weather[0].GetYear()));
	//
	//	for(int y=0; y<m_weather.GetNbYear(); y++)
	//    {
	//		double daylightVPD = m_weather[y].GetStat( STAT_DAYLIGHT_VPD, MEAN); //kPa
	//		double VPD = m_weather[y].GetStat( STAT_VPD, MEAN); //kPa
	//		
	//		double annualMinimum = m_weather[y].GetStat( STAT_TMIN, LOWEST); 
	//		double annualMinMean = m_weather[y].GetStat( STAT_TMIN, MEAN);
	//		double annualMean = m_weather[y].GetStat( STAT_T_MN, MEAN);
	//		double annualMaxMean = m_weather[y].GetStat( STAT_TMAX, MEAN);
	//		double annualMaximum = m_weather[y].GetStat( STAT_TMAX, HIGHEST);
	//		double annualPpt = m_weather[y].GetStat( STAT_PRCP, SUM);
	//		double annualSun = m_weather[y].GetStat( STAT_SRAD, SUM);
	//		double annualSnow = m_weather[y].GetStat( STAT_SNOW, SUM);
	//		double Tdew = m_weather[y].GetStat( STAT_TDEW, MEAN);
	//		double relHum = m_weather[y].GetStat( STAT_RELH, MEAN);
	//		double wnds = m_weather[y].GetStat( STAT_WNDS, MEAN);
	//
	//		int frostDay = m_weather[y].GetFrostDay(0);
	//		ASSERT( m_weather[y].GetFrostDay(0) == GetFrostDay(y, 0));
	//
	//		CTPeriod growingSeason = m_weather[y].GetGrowingSeason();
	//		CTPeriod frostFreePeriod = m_weather[y].GetFrostFreePeriod();
	//		int NBDayWithPrcp = m_weather[y].GetNbDayWith(PRCP, ">1");
	//		int NBDayWithoutPrcp = m_weather[y].GetNbDayWith(PRCP, "<=1", true);
	//	
	//		stat[y][ANNUAL_LOWEST_TMIN] = annualMinimum;
	//		stat[y][ANNUAL_MEAN_TMIN] = annualMinMean; 
	//		stat[y][ANNUAL_MEAN_TMEAN] = annualMean;
	//		stat[y][ANNUAL_MEAN_TMAX] = annualMaxMean;
	//		stat[y][ANNUAL_HIGHEST_TMAX] = annualMaximum;
	//		stat[y][ANNUAL_PPT] = annualPpt;
	//		stat[y][ANNUAL_SNOW] = annualSnow;
	//		stat[y][ANNUAL_MEAN_TDEW] = Tdew;
	//		stat[y][ANNUAL_MEAN_REL_HUM] = relHum;
	//		stat[y][ANNUAL_MEAN_WNDS] = wnds;
	//		stat[y][ANNUAL_FROST_DAY] = frostDay;
	//		stat[y][ANNUAL_FROST_FREE_PERIOD] = frostFreePeriod.GetLength();
	//		stat[y][ANNUAL_GROWING_SEASON] = growingSeason.GetLength();
	//		stat[y][ANNUAL_DAYLIGHT_VPD] = daylightVPD;
	//		stat[y][ANNUAL_MEAN_VPD] = VPD;
	//		stat[y][ANNUAL_SUN] = annualSun;
	//		stat[y][ANNUAL_NB_DAY_WITH_PRCP] = NBDayWithPrcp;
	//		stat[y][ANNUAL_NB_DAY_WITHOUT_PRCP] = NBDayWithoutPrcp;
	//	}
	//	
	//	SetOutput(stat);
	//
	//    return msg;
	//}
	//
	//ERMsg CClimaticModel::OnExecuteMonthly()
	//{
	//	ERMsg msg;
	//	
	//	CMonthlyStatVector stat(m_weather.GetNbYear()*12, CTRef(m_weather[0].GetYear(), CFL::FIRST_MONTH));
	//
	//	for(int y=0; y<m_weather.GetNbYear(); y++)
	//    {		
	//		for(int m=0; m<12; m++)
	//		{
	//			double daylightVPD = m_weather[y][m].GetStat( STAT_DAYLIGHT_VPD, MEAN); //kPa
	//			double VPD = m_weather[y][m].GetStat( STAT_VPD, MEAN); //kPa
	//			
	//			double monthlyMinimum = m_weather[y][m].GetStat( STAT_TMIN, LOWEST); 
	//			double monthlyMinMean = m_weather[y][m].GetStat( STAT_TMIN, MEAN);
	//			double monthlyMean = m_weather[y][m].GetStat( STAT_T_MN, MEAN);
	//			double monthlyMaxMean = m_weather[y][m].GetStat( STAT_TMAX, MEAN);
	//			double monthlyMaximum = m_weather[y][m].GetStat( STAT_TMAX, HIGHEST);
	//			double monthlyPpt = m_weather[y][m].GetStat( STAT_PRCP, SUM);
	//			double monthlySun = m_weather[y][m].GetStat( STAT_SRAD, SUM);
	//			double monthlySnow = m_weather[y][m].GetStat( STAT_SNOW, SUM);
	//			double Tdew = m_weather[y][m].GetStat( STAT_TDEW, MEAN);
	//			double relHum = m_weather[y][m].GetStat( STAT_RELH, MEAN);
	//			double wnds = m_weather[y][m].GetStat( STAT_WNDS, MEAN);
	//			int frostDay = m_weather[y][m].GetFrostDay(0);
	//			_ASSERTE( frostDay==m_weather[y][m].GetNbDayWith(TMIN, "<0") );
	//			int NBDayWithPrcp = m_weather[y][m].GetNbDayWith(PRCP, ">1");
	//			int NBDayWithoutPrcp = m_weather[y][m].GetNbDayWith(PRCP, "<=1", true);
	//
	//
	//			stat[y*12+m][MONTHLY_LOWEST_TMIN] = monthlyMinimum;
	//			stat[y*12+m][MONTHLY_MEAN_TMIN] = monthlyMinMean; 
	//			stat[y*12+m][MONTHLY_MEAN_TMEAN] = monthlyMean;
	//			stat[y*12+m][MONTHLY_MEAN_TMAX] = monthlyMaxMean;
	//			stat[y*12+m][MONTHLY_HIGHEST_TMAX] = monthlyMaximum;
	//			stat[y*12+m][MONTHLY_PPT] = monthlyPpt;
	//			stat[y*12+m][MONTHLY_SNOW] = monthlySnow;
	//			stat[y*12+m][MONTHLY_MEAN_TDEW] = Tdew;
	//			stat[y*12+m][MONTHLY_MEAN_REL_HUM] = relHum;
	//			stat[y*12+m][MONTHLY_MEAN_WNDS] = wnds;
	//			stat[y*12+m][MONTHLY_FROST_DAY] = frostDay;
	//			stat[y*12+m][MONTHLY_DAYLIGHT_VPD] = daylightVPD;
	//			stat[y*12+m][MONTHLY_MEAN_VPD] = VPD;
	//			stat[y*12+m][MONTHLY_SUN] = monthlySun;
	//			stat[y*12+m][MONTHLY_NB_DAY_WITH_PRCP] = NBDayWithPrcp;
	//			stat[y*12+m][MONTHLY_NB_DAY_WITHOUT_PRCP] = NBDayWithoutPrcp;
	//		}
	//	}
	//
	//	SetOutput(stat);
	//
	//
	//	return msg;
	//}
	//
	//
	//ERMsg CClimaticModel::OnExecuteDaily()
	//{
	//	ERMsg msg;
	//	
	//	CDailyStatVector stat(m_weather.GetNbDay(), m_weather.GetFirstTRef() );
	//	
	//	for(int y=0; y<m_weather.GetNbYear(); y++)
	//    {
	//		for(int m=0; m<12; m++)
	//		{
	//			for(int d=0; d<m_weather[y][m].GetNbDay(); d++)
	//			{
	//				const CWeatherDay& wDay = m_weather[y][m][d];
	//
	//				double daylightVPD = wDay.GetDaylightVaporPressureDeficit(); //kPa
	//				double VPD = wDay.GetVaporPressureDeficit();//kPa
	//
	//				//CStatistic VPD;
	//				//for(int h=0; h<24; h++)
	//				//{
	//				//	double t = wDay.GetT(h);
	//				//	double Td = Min(t, wDay.GetTd(h)); 
	//				//	VPD += Max(0, CFL::GetEs(t)-CFL::GetEs(Td));
	//				//}
	//
	//				double Tmin = wDay.GetTMin(); 
	//				double Tmean = wDay.GetTMean();
	//				double Tmax = wDay.GetTMax();
	//				double ppt = wDay.GetPpt()>=0.1?wDay.GetPpt():0;
	//				double snow = wDay[SNOW];
	//				double snowpack = wDay[SNDH];
	//				double Tdew = wDay[TDEW];
	//				double relHum = wDay[RELH];
	//				double wnds = wDay[WNDS];
	//				double srad = wDay.GetRad();
	//
	//				CTRef ref(m_weather[y].GetYear(), m, d);
	//
	//				stat[ref][DAILY_TMIN] = Tmin; 
	//				stat[ref][DAILY_TMEAN] = Tmean;
	//				stat[ref][DAILY_TMAX] = Tmax;
	//				stat[ref][DAILY_PPT] = ppt;
	//				stat[ref][DAILY_SNOW] = snow;
	//				stat[ref][DAILY_SNOWPACK] = snowpack;
	//				stat[ref][DAILY_MEAN_TDEW] = Tdew;
	//				stat[ref][DAILY_MEAN_REL_HUM] = relHum;
	//				stat[ref][DAILY_MEAN_WNDS] = wnds;
	//				stat[ref][DAILY_DAYLIGHT_VPD] = daylightVPD;
	//				stat[ref][DAILY_MEAN_VPD] = VPD;
	//				stat[ref][DAILY_SUN] = srad;
	//			}
	//		}
	//	}
	//
	//	SetOutput(stat);
	//
	//	return msg;
	//}
	//
	//
	//ERMsg CClimaticModel::OnExecuteHourly()
	//{
	//	ERMsg msg;
	//
	//	CHourlyStatVector stat;
	//	GetHourlyStat(stat);
	//	SetOutput(stat);
	//
	//	return msg;
	//}
	//
	//
	//void CClimaticModel::GetHourlyStat(CModelStatVector& statIn)
	//{
	//	CHourlyStatVector& stat = (CHourlyStatVector&) statIn; 
	//		
	//	CSun sun(m_info.m_loc.GetLat(), m_info.m_loc.GetLon());
	//	CWeather weather(m_weather);
	//	weather.AjusteMeanForHourlyRequest(m_info.m_loc);
	//
	//	stat.Init(weather.GetNbDay()*24, weather.GetFirstTRef().Transform(CTRef::HOURLY) );
	//	for(int y=0; y<weather.GetNbYear(); y++)
	//    {
	//		//double DD=0;
	//		for(int m=0; m<12; m++)
	//		{
	//			for(int d=0; d<weather[y][m].GetNbDay(); d++)
	//			{
	//				
	//				const CWeatherDay& wDay = weather[y][m][d];
	//				
	//				CDay hours;
	//				wDay.GetHourlyVar(m_x0,m_x1,m_x2,m_x3,hours,m_info.m_loc);
	//				
	//				for(int h=0; h<24; h++)
	//				{
	//					CTRef ref(weather[y].GetYear(), m, d, h);
	//					stat[ref][HOURLY_T] = hours[h][H_TEMP];
	//					stat[ref][HOURLY_PRCP] = hours[h][H_PRCP];
	//					stat[ref][HOURLY_TDEW] = hours[h][H_TDEW];
	//					stat[ref][HOURLY_REL_HUM] = hours[h][H_RELH];
	//					stat[ref][HOURLY_WIND_SPEED] = hours[h][H_WNDS];
	//					stat[ref][HOURLY_VPD] = hours[h](H_VPD);
	//					stat[ref][HOURLY_SRAD] = hours[h][H_SRAD];
	//					
	//					HxGridTestConnection();
	//				}
	//			}
	//		}
	//	}
	//}


	int CClimaticModel::GetFrostDay(int year, const double& th)
	{
		int nbDays = 0;
		/* for(int day=0; day<m_weather[year].GetNbDay(); day++)
		 {
		 const CDay& d = m_weather[year].GetDay(day);

		 if(d.GetTMin()<th)
		 nbDays++;
		 }*/

		return nbDays;
	}


	//This method is call to load your parameter in your variable
	ERMsg CClimaticModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;



		if (parameters.size() == 9)
		{
			int c = 0;
			m_varType = parameters[c++].GetInt();
			for (int i = 0; i < 2; i++)
			{
				m_a[i] = parameters[c++].GetReal();
				m_b[i] = parameters[c++].GetReal();
			}

			m_x0 = parameters[c++].GetReal();
			m_x1 = parameters[c++].GetReal();
			m_x2 = parameters[c++].GetReal();
			m_x3 = parameters[c++].GetReal();
		}

		return msg;
	}




	//simulated annaling 
	void CClimaticModel::AddSAResult(const StringVector& header, const StringVector& data)
	{



		if (header.size() == 12)
		{
			std::vector<double> obs(4);

			CTRef TRef(ToShort(data[2]), ToShort(data[3]) - 1, ToShort(data[4]) - 1, ToShort(data[5]));
			for (int i = 0; i < 4; i++)
				obs[i] = ToDouble(data[i + 6]);


			ASSERT(obs.size() == 4);
			m_SAResult.push_back(CSAResult(TRef, obs));
		}

		/*if( header.size()==26)
		{
		std::vector<double> obs(24);

		for(int h=0; h<24; h++)
		obs[h] = data[h+2].ToDouble();


		ASSERT( obs.size() == 24 );
		m_SAResult.push_back( CSAResult(CTRef(), obs ) );
		}
		else if( header.size()==13)
		{
		std::vector<double> obs(7);

		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1,data[5].ToShort());
		for(int c=0; c<7; c++)
		obs[c] = data[c+6].ToDouble();


		ASSERT( obs.size() == 7 );
		m_SAResult.push_back( CSAResult(TRef, obs ) );
		}
		else if( header.size()==12)
		{
		std::vector<double> obs(7);

		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1,data[4].ToShort()-1);
		for(int c=0; c<7; c++)
		obs[c] = data[c+5].ToDouble();


		ASSERT( obs.size() == 7 );
		m_SAResult.push_back( CSAResult(TRef, obs ) );
		}
		else if( header.size()==11)
		{
		std::vector<double> obs(7);

		CTRef TRef(data[2].ToShort(),data[3].ToShort()-1);
		for(int c=0; c<7; c++)
		obs[c] = data[c+4].ToDouble();


		ASSERT( obs.size() == 7 );
		m_SAResult.push_back( CSAResult(TRef, obs ) );
		}*/
	}

	void CClimaticModel::GetFValueHourly(CStatisticXY& stat)
	{
		if (m_SAResult.size() > 0)
		{
			CHourlyStatVector data;
			GetHourlyStat(data);

			for (size_t d = 0; d < m_SAResult.size(); d++)
			{
				if (m_SAResult[d].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[d].m_ref))
				{
					static const int HOURLY_TYPE[6] = { HOURLY_T, HOURLY_TDEW, HOURLY_REL_HUM, HOURLY_WIND_SPEED, HOURLY_VPD, HOURLY_VPD };
					double obs = m_SAResult[d].m_obs[m_varType];
					double sim = data[m_SAResult[d].m_ref][HOURLY_TYPE[m_varType]];


					//double test = data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM];
					//CFL::RH2Td(data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM], data[m_SAResult[i].m_ref][MONTHLY_MEAN_REL_HUM]);

					if (!_isnan(sim) && !_isnan(obs) &&
						_finite(sim) && _finite(obs))
						stat.Add(sim, obs);
				}

				HxGridTestConnection();

			}

			/*
					if( m_SAResult[0].m_obs.size() == 24 )
					{
					//CTRef TRef = data.GetFirstTRef();
					//CStatistic statH[24];
					//for(int i=0; i<data.size(); i++, TRef++)
					//{
					//	double v = data[i][m_varType];
					//	statH[TRef.GetHour()]+=v;
					//	HxGridTestConnection();
					//}

					//for(int y=0; y<m_weather.GetNbYear(); y++)
					//{
					//	double DD=0;
					//	for(int m=0; m<12; m++)
					//	{
					//		for(int d=0; d<m_weather[y][m].GetNbDay(); d++)
					//		{
					//			const CWeatherDay& wDay = m_weather[y][m][d];
					//			for(int h=0; h<24; h++)
					//			{
					//
					//				//switch(m_varType)
					//				//{
					//				////case T_MN:
					//				//case TDEW: v= Min( wDay.GetT(h), GetVarH(wDay, h, var));break;
					//				//case RELH: v= Max(0, Min(100, GetVarH(wDay, h, var)));break;
					//				//case WNDS: v = Max(0, GetVarH(wDay, h, var));break;
					//				//}

					//				statH[h]+=v;
					//				HxGridTestConnection();
					//			}
					//		}
					//	}
					//}


					//ASSERT( m_SAResult.size() == 1 );
					//ASSERT( m_SAResult[0].m_obs.size() == 24 );
					//for(int h=0; h<24; h++)
					//{
					//	stat.Add(statH[h][MEAN], m_SAResult[0].m_obs[h]);
					//}
					}
					else if( m_SAResult[0].m_obs.size() == 7 )
					{


					for(size_t i=0; i<m_SAResult.size(); i++)
					{

					if( m_SAResult[i].m_obs[m_varType] >-999 && data.IsInside( m_SAResult[i].m_ref))
					{
					double obs =  m_SAResult[i].m_obs[m_varType];
					double sim = data[m_SAResult[i].m_ref][m_varType];
					stat.Add(sim,obs);
					}

					HxGridTestConnection();

					}
					}
					*/
		}
	}

	void CClimaticModel::GetFValueDaily(CStatisticXY& stat)
	{

		if (m_SAResult.size() > 0)
		{
			OnExecuteDaily();
			const CDailyStatVector& data = (const CDailyStatVector&)GetOutput();

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{

				if (m_SAResult[i].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[i].m_ref))
				{

					static const int DAILY_TYPE[6] = { DAILY_TMIN, DAILY_TMAX, DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, DAILY_MEAN_WNDS, DAILY_MEAN_VPD };
					double obs = m_SAResult[i].m_obs[m_varType];
					double sim = data[m_SAResult[i].m_ref][DAILY_TYPE[m_varType]];
					stat.Add(sim, obs);
				}

				HxGridTestConnection();

			}
		}
	}


	void CClimaticModel::GetFValueMonthly(CStatisticXY& stat)
	{

		if (m_SAResult.size() > 0)
		{

			OnExecuteMonthly();
			const CMonthlyStatVector& data = (const CMonthlyStatVector&)GetOutput();

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{

				if (m_SAResult[i].m_obs[m_varType] > -999 && data.IsInside(m_SAResult[i].m_ref))
				{


					static const int MONTHLY_TYPE[6] = { MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMAX, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, MONTHLY_MEAN_WNDS, MONTHLY_MEAN_VPD };
					double obs = m_SAResult[i].m_obs[m_varType];
					double sim = data[m_SAResult[i].m_ref][MONTHLY_TYPE[m_varType]];





					stat.Add(sim, obs);
				}

				HxGridTestConnection();

			}
		}
	}


	//NOTE: Begin and END are ZERO-BASED Julian dates
	//Source:
	//Boughner, C.C. 1964. Distribution of growing degree days in Canada. 
	//Can. Met. Memoirs 17. Met. Br., Dept. of Transport. 40 p.
	//CPeriod GetGrowingSeason(CWeatherYear& weather)
	//{
	//	int day = 200; //(Mid-July)
	//	bool frost = false;
	//	CPeriod p(m_year, 0, GetLastDay());


	//	//Beginning of the growing season
	//	//look for the first occurrence of 3 successive days with frost
	//	do
	//	{

	//		frost = GetDay(day).GetTMin() < 0 &&
	//			GetDay(day - 1).GetTMin() < 0 &&
	//			GetDay(day - 2).GetTMin() < 0;

	//		day--;

	//	} while (!frost && day > 1);


	//	if (day>1)
	//	{
	//		p.Begin().SetJulianDay(day + 2);
	//	}


	//	//End of growing season
	//	day = 200;
	//	do
	//	{
	//		//look for the first occurrence of 3 successive days with frost
	//		frost = GetDay(day).GetTMin() < 0 &&
	//			GetDay(day + 1).GetTMin() < 0 &&
	//			GetDay(day + 2).GetTMin() < 0;
	//		day++;
	//	} while (!frost && day < GetLastDay() - 2);

	//	if (day<GetLastDay() - 2)
	//	{
	//		p.End().SetJulianDay(day - 2);
	//	}

	//	if (p.End() < p.Begin())
	//	{
	//		p.End() = p.Begin() = CDate(m_year, 200);
	//	}

	//	return p;
	//}


	//FrostFree period
	//CPeriod GetFrostFreePeriod(CWeatherYear& weather)
	//{
	//	CPeriod pTmp;
	//	CPeriod p;
	//	CWeatherYear::InitializePeriod(pTmp);//Init year
	//	//	CWeatherYear::InitializePeriod(p);

	//	//int FFPeriod=0;
	//	//int maxFFPeriod=0;
	//	bool notInit = true;

	//	int nbDay = GetNbDay();
	//	for (int jd = 0; jd<nbDay; jd++)
	//	{
	//		if (GetDay(jd).GetTMin()>0) //Frost-free period begin or continues
	//		{
	//			if (notInit)
	//			{
	//				pTmp.Begin().SetJulianDay(jd);
	//				notInit = false;
	//			}
	//		}
	//		else
	//		{
	//			if (!notInit)
	//			{
	//				pTmp.End().SetJulianDay(jd);
	//				notInit = true;

	//				//Frost-free period ends
	//				if (pTmp.GetLength() > p.GetLength())
	//					p = pTmp;
	//			}
	//		}

	//		if (jd == GetLastDay() && !notInit)
	//		{
	//			pTmp.End().SetJulianDay(jd);
	//			if (pTmp.GetLength() > p.GetLength())
	//				p = pTmp;
	//		}
	//	}



	//	return p;
	//}

	//return Water Deficit en mm of wather
	//double GetWaterDeficit(CWeatherYear& weather)
	//{
	//	//est-ce qu'on devrait utiliser cette équation à la place???
	//	//return max( 0, GetStat(PET, SUM) -	GetStat(PPT, SUM));

	//	CThornthwaitePET PET(*this, 0, CThornthwaitePET::POTENTIEL_STANDARD);
	//	double A = 0;
	//	//calculer Et pour le mois et A
	//	for (int m = 0; m<12; m++)
	//	{
	//		if (m_months[m].GetStat(TMEAN, MEAN)>0.)
	//		{
	//			//precipitation in mm
	//			double A_tmp = (PET.Get(m) - m_months[m].GetStat(PPT, SUM));
	//			if (A_tmp>0.)
	//				A += A_tmp;
	//		}
	//	}

	//	return(A);
	//}


	double GetDaylightVaporPressureDeficit(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetDaylightVaporPressureDeficit(weather[m]);

		return stat[MEAN];
	}

	double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetDaylightVaporPressureDeficit(weather[d]);

		return stat[MEAN];
	}

	//Saturation vapor pressure at daylight temperature
	double GetDaylightVaporPressureDeficit(const CWeatherDay& weather)
	{
		double daylightT = weather.GetTdaylight();
		double daylightEs = e°(daylightT);

		return max(0.0, daylightEs - weather[H_EA][MEAN]);
	}

	double GetNbDayWithPrcp(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetNbDayWithPrcp(weather[m]);

		return stat[SUM];
	}

	double GetNbDayWithPrcp(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetNbDayWithPrcp(weather[d]);

		return stat[SUM];
	}

	double GetNbDayWithPrcp(const CWeatherDay& weather)
	{
		return (weather[H_PRCP][SUM]>=0.2?1:0);
	}

	double GetNbFrostDay(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetNbFrostDay(weather[m]);

		return stat[SUM];
	}

	double GetNbFrostDay(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetNbFrostDay(weather[d]);

		return stat[SUM];
	}

	double GetNbFrostDay(const CWeatherDay& weather)
	{
		return (weather[H_TMIN][LOWEST] <= 0 ? 1 : 0);
	}

	
	
}