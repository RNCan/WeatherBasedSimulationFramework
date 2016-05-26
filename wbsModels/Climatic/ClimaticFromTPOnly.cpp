//**********************************************************************
// 21/01/2016	2.5.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 05/04/2013			Rémi Saint-Amant	Remove DD and PET
// 08/12/2012			Rémi Saint-Amant	Compile with hxGrid TestConnection
// 02/07/2012			Rémi Saint-Amant	New version without humidity and wind
// 29/06/2010			Rémi Saint-Amant	Compatible with HxGrid. Remove extrem
// 30/10/2009			Rémi Saint-Amant	Change CPeriod by CTPeriod
// 03/03/2009			Rémi Saint-Amant	Integrate with new BioSIMModelBase (hxGrid)
// 19/11/2008			Rémi Saint-Amant	Update with VS9 and new BioSIMModelBase 
// 27/05/2008			Rémi Saint-Amant	Used of wind speed in the computation of ASC2000 PET
// 01/12/2002			Rémi Saint-Amant	2 variables was added: Degree-day and % of snow
// 15/07/2002			Rémi Saint-Amant	Creation

//**********************************************************************


#include "Basic/Evapotranspiration.h"
#include "ModelBase/EntryPoint.h"
#include "ClimaticFromTPOnly.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CClimaticFromTPOnlyModel::CreateObject);


	enum TAnnualStat{ ANNUAL_LOWEST_TMIN, ANNUAL_MEAN_TMIN, ANNUAL_MEAN_TMEAN, ANNUAL_MEAN_TMAX, ANNUAL_HIGHEST_TMAX, ANNUAL_PPT, ANNUAL_SNOW, ANNUAL_MEAN_TDEW, ANNUAL_MEAN_REL_HUM, /*ANNUAL_SUM_DD,*/ ANNUAL_FROST_DAY, ANNUAL_FROST_FREE_PERIOD, ANNUAL_GROWING_SEASON, /*ANNUAL_MEAN_PET, ANNUAL_ARIDITY,*/ ANNUAL_MEAN_VPD, ANNUAL_SUN, ANNUAL_NB_DAY_WITH_PRCP, ANNUAL_NB_DAY_WITHOUT_PRCP, NB_ANNUAL_STATS };
	enum TMonthlyStat{ MONTHLY_LOWEST_TMIN, MONTHLY_MEAN_TMIN, MONTHLY_MEAN_TMEAN, MONTHLY_MEAN_TMAX, MONTHLY_HIGHEST_TMAX, MONTHLY_PPT, MONTHLY_SNOW, MONTHLY_MEAN_TDEW, MONTHLY_MEAN_REL_HUM, /*MONTHLY_SUM_DD,*/ MONTHLY_FROST_DAY, /*MONTHLY_MEAN_PET, MONTHLY_ARIDITY,*/ MONTHLY_MEAN_VPD, MONTHLY_SUN, MONTHLY_NB_DAY_WITH_PRCP, MONTHLY_NB_DAY_WITHOUT_PRCP, NB_MONTHLY_STATS };
	enum TDailyStat{ DAILY_TMIN, DAILY_TMEAN, DAILY_TMAX, DAILY_PPT, DAILY_SNOW, DAILY_SNOWPACK, DAILY_MEAN_TDEW, DAILY_MEAN_REL_HUM, /*DAILY_SUM_DD, DAILY_MEAN_PET,*/ DAILY_MEAN_VPD, DAILY_SUN, NB_DAILY_STATS };
	enum THourlyStat{ HOURLY_T, HOURLY_PRCP, HOURLY_TDEW, HOURLY_REL_HUM, HOURLY_MEAN_VPD, HOURLY_SUN, NB_HOURLY_STATS };

	typedef CModelStatVectorTemplate<NB_ANNUAL_STATS> CAnnualStatVector;
	typedef CModelStatVectorTemplate<NB_MONTHLY_STATS> CMonthlyStatVector;
	typedef CModelStatVectorTemplate<NB_DAILY_STATS> CDailyStatVector;
	typedef CModelStatVectorTemplate<NB_HOURLY_STATS> CHourlyStatVector;


	//Contructor
	CClimaticFromTPOnlyModel::CClimaticFromTPOnlyModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "2.5.0 (2016)";
	}

	CClimaticFromTPOnlyModel::~CClimaticFromTPOnlyModel()
	{}

	ERMsg CClimaticFromTPOnlyModel::OnExecuteAnnual()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod();
		CAnnualStatVector stat(p);

		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			double DPV = m_weather[y].GetStat(H_VPD)[MEAN]; //kPa
			double annualMinimum = m_weather[y].GetStat(H_TMIN)[LOWEST];
			double annualMinMean = m_weather[y].GetStat(H_TMIN)[MEAN];
			double annualMean = m_weather[y].GetStat(H_TAIR)[MEAN];
			double annualMaxMean = m_weather[y].GetStat(H_TMAX)[MEAN];
			double annualMaximum = m_weather[y].GetStat(H_TMAX)[HIGHEST];
			double annualPpt = m_weather[y].GetStat(H_PRCP)[SUM];
			double srad = m_weather[y].GetStat(H_SRAD)[SUM];

			/*double annualSnow = 0;
			for(int day=0; day<m_weather[y].GetNbDay(); day++)
			{
			const CWeatherDay& d = m_weather[y].GetDay(day);

			if( d.GetTMean() < 0 )
			annualSnow += d.GetPpt();

			}*/

			double annualSnow = m_weather[y].GetStat(H_SNOW)[SUM];
			double Tdew = m_weather[y].GetStat(H_TDEW)[MEAN];
			double relHum = m_weather[y].GetStat(H_RELH)[MEAN];
			
			int frostDay = m_weather[y].GetFrostDay(0);
			ASSERT(m_weather[y].GetFrostDay(0) == GetFrostDay(y, 0));

			CTPeriod growingSeason = m_weather[y].GetGrowingSeason();
			CTPeriod frostFreePeriod = m_weather[y].GetFrostFreePeriod();
			int NBDayWithPrcp = m_weather[y].GetNbDayWith(PRCP, ">1");
			int NBDayWithoutPrcp = m_weather[y].GetNbDayWith(PRCP, "<=1", true);

			stat[y][ANNUAL_LOWEST_TMIN] = annualMinimum;
			stat[y][ANNUAL_MEAN_TMIN] = annualMinMean;
			stat[y][ANNUAL_MEAN_TMEAN] = annualMean;
			stat[y][ANNUAL_MEAN_TMAX] = annualMaxMean;
			stat[y][ANNUAL_HIGHEST_TMAX] = annualMaximum;
			stat[y][ANNUAL_PPT] = annualPpt;
			stat[y][ANNUAL_SNOW] = annualSnow;
			stat[y][ANNUAL_MEAN_TDEW] = Tdew;
			stat[y][ANNUAL_MEAN_REL_HUM] = relHum;
			stat[y][ANNUAL_FROST_DAY] = frostDay;
			stat[y][ANNUAL_FROST_FREE_PERIOD] = frostFreePeriod.GetLength();
			stat[y][ANNUAL_GROWING_SEASON] = growingSeason.GetLength();
			stat[y][ANNUAL_MEAN_VPD] = DPV;
			stat[y][ANNUAL_SUN] = srad;
			stat[y][ANNUAL_NB_DAY_WITH_PRCP] = NBDayWithPrcp;
			stat[y][ANNUAL_NB_DAY_WITHOUT_PRCP] = NBDayWithoutPrcp;

			HxGridTestConnection();
		}

		SetOutput(stat);

		return msg;
	}

	ERMsg CClimaticFromTPOnlyModel::OnExecuteMonthly()
	{
		ERMsg msg;


		CMonthlyStatVector stat(m_weather.GetNbYear() * 12, CTRef(m_weather[0].GetYear(), CFL::FIRST_MONTH));


		//ComputeSnow();//compute snow: waiting new BioSIM
		for (int y = 0; y < m_weather.GetNbYear(); y++)
		{
			//CPriestlyTaylorPET PriestlyTaylorPET;
			//CThornthwaitePET ThornthwaitePET;
			//CBlaneyCriddlePET BlaneyCriddlePET;
			//CASCE2000PET ASCE2000PET;

			//CThornthwaitePET TPET(m_weather[y], 0, CThornthwaitePET::POTENTIEL_STANDARD);
			//


			//switch( m_PETType )
			//{
			//case PRIESTLY_TAYLOR: PriestlyTaylorPET.Compute( m_weather[y], m_info.m_loc.GetLat(), m_info.m_loc.GetElev() ); break;
			//case THORNTHWAITE: ThornthwaitePET.Compute(m_weather[y], m_info.m_loc.GetLat());break;
			//case BLANEY_CRIDDLE: BlaneyCriddlePET.Compute(m_weather[y], m_info.m_loc.GetLat());break;
			//case ASCE2000:	ASCE2000PET.Compute(m_weather[y], m_info.m_loc.GetLat(), m_info.m_loc.GetElev(), CASCE2000PET::LONG_REF); break;
			//default: _ASSERTE(false);
			//}

			for (int m = 0; m < 12; m++)
			{
				//double ar = TPET.GetWaterDeficit(m_weather[y][m]);//in mm
				double DPV = m_weather[y][m].GetStat(STAT_VPD, MEAN); //kPa

				double monthlyMinimum = m_weather[y][m].GetStat(STAT_TMIN, LOWEST);
				double monthlyMinMean = m_weather[y][m].GetStat(STAT_TMIN, MEAN);
				double monthlyMean = m_weather[y][m].GetStat(STAT_T_MN, MEAN);
				double monthlyMaxMean = m_weather[y][m].GetStat(STAT_TMAX, MEAN);
				double monthlyMaximum = m_weather[y][m].GetStat(STAT_TMAX, HIGHEST);
				double monthlyPpt = m_weather[y][m].GetStat(STAT_PRCP, SUM);
				double srad = m_weather[y][m].GetStat(STAT_SRAD, SUM);

				/*double monthlySnow = 0;
				for(int day=0; day<m_weather[y][m].GetNbDay(); day++)
				{
				const CWeatherDay& d = m_weather[y][m].GetDay(day);

				if( d.GetTMean() < 0 )
				monthlySnow += d.GetPpt();

				}*/

				double monthlySnow = m_weather[y][m].GetStat(STAT_SNOW, SUM);

				//double EvapTrans=0;
				//_ASSERTE(NB_PET==6);
				//switch( m_PETType )
				//{
				//case PRIESTLY_TAYLOR: EvapTrans = PriestlyTaylorPET.Get(m); break;
				//case THORNTHWAITE: EvapTrans = ThornthwaitePET.Get(m); break;
				//case BLANEY_CRIDDLE: EvapTrans = BlaneyCriddlePET.Get(m); break;
				//case ASCE2000: EvapTrans = ASCE2000PET.Get(m); break;
				//default: _ASSERT(false);
				//}


				double Tdew = m_weather[y][m].GetStat(STAT_TDEW, MEAN);
				double relHum = m_weather[y][m].GetStat(STAT_RELH, MEAN);
				//double wnds = m_weather[y][m].GetStat( STAT_WNDS, MEAN);
				//double DD = m_weather[y][m].GetDD(m_threshold);
				int frostDay = m_weather[y][m].GetFrostDay(0);
				_ASSERTE(frostDay == m_weather[y][m].GetNbDayWith(TMIN, "<0"));
				int NBDayWithPrcp = m_weather[y][m].GetNbDayWith(PRCP, ">1");
				int NBDayWithoutPrcp = m_weather[y][m].GetNbDayWith(PRCP, "<=1", true);

				stat[y * 12 + m][MONTHLY_LOWEST_TMIN] = monthlyMinimum;
				stat[y * 12 + m][MONTHLY_MEAN_TMIN] = monthlyMinMean;
				stat[y * 12 + m][MONTHLY_MEAN_TMEAN] = monthlyMean;
				stat[y * 12 + m][MONTHLY_MEAN_TMAX] = monthlyMaxMean;
				stat[y * 12 + m][MONTHLY_HIGHEST_TMAX] = monthlyMaximum;
				stat[y * 12 + m][MONTHLY_PPT] = monthlyPpt;
				stat[y * 12 + m][MONTHLY_SNOW] = monthlySnow;
				stat[y * 12 + m][MONTHLY_MEAN_TDEW] = Tdew;
				stat[y * 12 + m][MONTHLY_MEAN_REL_HUM] = relHum;
				//		stat[y*12+m][MONTHLY_MEAN_WNDS] = wnds;
				//stat[y*12+m][MONTHLY_SUM_DD] = DD;
				stat[y * 12 + m][MONTHLY_FROST_DAY] = frostDay;
				//stat[y*12+m][MONTHLY_MEAN_PET] = EvapTrans;
				//stat[y*12+m][MONTHLY_ARIDITY] = ar;
				stat[y * 12 + m][MONTHLY_MEAN_VPD] = DPV;
				stat[y * 12 + m][MONTHLY_SUN] = srad;
				stat[y * 12 + m][MONTHLY_NB_DAY_WITH_PRCP] = NBDayWithPrcp;
				stat[y * 12 + m][MONTHLY_NB_DAY_WITHOUT_PRCP] = NBDayWithoutPrcp;

				HxGridTestConnection();
			}
		}

		SetOutput(stat);


		return msg;
	}


	ERMsg CClimaticFromTPOnlyModel::OnExecuteDaily()
	{
		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod();
		CDailyStatVector stat(p);
		
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//int year = p.beg
			for (size_t m = 0; m < m_weather[y].size() m++)
			{
				for (size_t d = 0; d < m_weather[y][m].size(); d++)
				{
					const CWeatherDay& wDay = m_weather[y][m][d];

					double DPV = wDay.GetVaporPressureDeficit();
					double Tmin = wDay.GetTMin();
					double Tmean = wDay.GetTMean();
					double Tmax = wDay.GetTMax();
					double ppt = wDay.GetPpt() >= 0.1 ? wDay.GetPpt() : 0;
					double snow = wDay[SNOW];
					double snowpack = wDay[SNDH];
					double Tdew = wDay[TDEW];
					double relHum = wDay[RELH];
					double srad = wDay.GetRad();

					
					CTRef ref(m_weather[y].GetYears(), m, d);

					stat[ref][DAILY_TMIN] = Tmin;
					stat[ref][DAILY_TMEAN] = Tmean;
					stat[ref][DAILY_TMAX] = Tmax;
					stat[ref][DAILY_PPT] = ppt;
					stat[ref][DAILY_SNOW] = snow;
					stat[ref][DAILY_SNOWPACK] = snowpack;
					stat[ref][DAILY_MEAN_TDEW] = Tdew;
					stat[ref][DAILY_MEAN_REL_HUM] = relHum;
					//stat[ref][DAILY_MEAN_WNDS] = wnds;
					//stat[ref][DAILY_SUM_DD] = DD;
					//stat[ref][DAILY_MEAN_PET] = EvapTrans;
					stat[ref][DAILY_MEAN_VPD] = DPV;
					stat[ref][DAILY_SUN] = srad;


					HxGridTestConnection();
				}
			}
		}

		SetOutput(stat);
		return msg;
	}

	/*
	double GetL(double t)
	{

	double T = t + 273.15;
	double a = (2.501E6-2.257E6)/(273.15-373.15);
	double b = 2.501E6-a*273.15;
	//L = 2.501E6 //J kg-1 at T = 273.15 K and L
	//L = 2.257E6 //J kg-1 at T = 373.15 K.
	double L = a*T+b;
	return L;
	}

	//A1 = 17.625, B1 =243.04°C, and C1 = 0.61094 kPa. These provide values
	//for es with a relative error of < 0.4% over the range [-40°C,50°C].
	double GetEs(double t)
	{
	return 0.61094*exp((17.625*t)/(t+243.04));
	}
	//The Relationship between Relative Humidity and the Dewpoint Temperature in Moist Air
	//A Simple Conversion and Applications
	//Equation 11
	double RH2Td(double t, double RH)
	{
	RH = Max(1, RH);

	const double A1=17.625;
	const double B1=243.04;

	//	double T = t + 273.15;//K
	double num = B1*(log(RH/100)+(A1*t)/(B1+t));
	double den = A1-log(RH/100)-(A1*t)/(B1+t);
	double Td = num/den;//°C
	return Td;
	}

	double Td2RH(double t, double Td)
	{
	double T= t + 273.15;//K
	double TTd= Td + 273.15;//K
	double L = GetL(t);
	const double Rw=461.5;//J K-1 kg-1;

	return Min(100, Max(0, 100*exp(-L*(T-TTd)/(Rw*T*TTd))));
	}

	double GetHourlyRH(double Tmin,double Tmax,double T,double RHmin,double RHmax)
	{
	//Equation from :Development of Hourly Meteorological Values From Daily Data and Signi?cance to
	//Hydrological Modeling at H. J. Andrews Experimental Forest
	return Min(100, Max(0, RHmax+(T-Tmin)/(Tmax-Tmin)*(RHmin-RHmax)));
	}

	double GetAllenRH(const CWeatherDay& day, int hour, int hourTmax=16, double overheat=0)
	{
	//There is a third hypothesis regarding hourly interpolation of relative humidity: values within a day result from
	//changes in temperature only, and not from changes in the absolute moisture content of air.
	//From : Estimating air humidity from temperature and precipitation measures for modelling applications


	CWeatherDay days[3] = {day.GetPreviousDay(), day, day.GetNextDay()};

	CStatistic Es[3];
	for(int i=0; i<3; i++)
	for(int h=0; h<24; h++)
	Es[i]+=GetEs(days[i].GetAllenT(h));

	double e_mean0 = days[0][RELH]*Es[0][MEAN]/100;
	double e_mean1 = days[1][RELH]*Es[1][MEAN]/100;
	double e_mean2 = days[2][RELH]*Es[2][MEAN]/100;


	//double T1 = hour<(hourTmax-8)?days[0].GetAllenT(hourTmax-8):days[1].GetAllenT(hourTmax-8);
	//double T2 = hour<(hourTmax-8)?days[0].GetAllenT(hourTmax-8):days[1].GetAllenT(hourTmax-8);
	double e1 = hour<12?e_mean0:e_mean1;
	double e2 = hour<12?e_mean1:e_mean2;
	double a = (e2-e1)/24;
	double e = e1+((hour+12)%24)*a;
	double t = day.GetAllenT(hour, hourTmax, overheat);
	double es=GetEs(t);
	double RH = e/es*100;

	return Max(0, Min(100, RH));
	}
	*/
	ERMsg CClimaticFromTPOnlyModel::OnExecuteHourly()
	{
		ERMsg msg;

		CWeather weather(m_weather);
		weather.AjusteMeanForHourlyRequest(m_info.m_loc);


		CHourlyStatVector stat(weather.GetNbDay() * 24, weather.GetFirstTRef().Transform(CTRef::HOURLY));
		for (int y = 0; y < weather.GetNbYear(); y++)
		{
			double DD = 0;
			for (int m = 0; m < 12; m++)
			{
				for (int d = 0; d < weather[y][m].GetNbDay(); d++)
				{
					CDay hDay;
					weather[y][m][d].GetHourlyVar(hDay, m_info.m_loc);

					for (int h = 0; h < 24; h++)
					{
						CTRef ref(weather[y].GetYear(), m, d, h);
						stat[ref][HOURLY_T] = hDay[h][H_TEMP];
						stat[ref][HOURLY_PRCP] = hDay[h][H_PRCP];
						stat[ref][HOURLY_TDEW] = hDay[h][H_TDEW];
						stat[ref][HOURLY_REL_HUM] = hDay[h][H_RELH];
						stat[ref][HOURLY_MEAN_VPD] = hDay[h](H_VPD);
						stat[ref][HOURLY_SUN] = hDay[h][H_SRAD];

						HxGridTestConnection();
					}
				}
			}
		}

		SetOutput(stat);
		return msg;
	}


	int CClimaticFromTPOnlyModel::GetFrostDay(int year, const double& th)
	{
		int nbDays = 0;
		for (int day = 0; day < m_weather[year].GetNbDay(); day++)
		{
			const CWeatherDay& d = m_weather[year].GetDay(day);

			if (d.GetTMin() < th)
				nbDays++;
		}

		return nbDays;
	}

	int CClimaticFromTPOnlyModel::GetDaysBelow(int year, const double& th)
	{
		int nbDays = 0;
		for (int day = 0; day < m_weather[year].GetNbDay(); day++)
		{
			const CWeatherDay& d = m_weather[year].GetDay(day);

			if (d.GetTMean() < th)
				nbDays++;
		}


		return nbDays;
	}

	//This method is call to load your parameter in your variable
	ERMsg CClimaticFromTPOnlyModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		int c = 0;
		//    m_threshold = parameters[c++].GetReal();
		//m_PETType = parameters[c++].GetInt();

		return msg;
	}

}