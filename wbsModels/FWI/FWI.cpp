//  F32.c  written by BMW @ PNFI
//Modifications
// 01/01/1994  precision fix etc........bmw
// 01/10/2002
//     to eliminate negative evapouration from DC skit in November
//     thru March.  Change of code to match equations in FTR-33 (1985) 
//     report (change from F32.FOR)
// 01/06/2005  to add STDLIB.h to solve recent problems in conversion of startups
//    from char to float
// 10/12/2007 	Rémi Saint-Amant	Incorporate in BioSIMModelBase
// 01/02/2009	Rémi Saint-Amant	Use new snow model to predic last snow day
// 27/02/2009	Rémi Saint-Amant	Include new variable for montly model
// 05/03/2010	Rémi Saint-Amant	Include overwintering DC and continious mode when no freezing
// 11/03/2010	Rémi Saint-Amant	Include start threshold
// 18/05/2011   Rémi Saint-Amant	Change result format( January 1 to December 31), correction of a bug in computing new DC
//									Add of m_carryOverFraction and EFFECTIVENESS_OF_WINTER as static parameters
// 06/02/2012	Rémi Saint-Amant	Correction of a bug in the first date. The first date must start 3 days after the snow melt 
//**********************************************************************
#include <math.h>
#include "Basic/GrowingSeason.h"
#include "Basic/SnowAnalysis.h"
#include "Basic/WeatherDefine.h"
#include "Basic/UtilMath.h"
#include "FWI.h"
//******************************************


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{



	CFWI::CFWI()
	{
		Reset();
	}

	CFWI::~CFWI()
	{}

	void CFWI::Reset()
	{
		m_method = NOON_CALCULATION;

		m_bAutoSelect = true;
		m_nbDaysStart = 3;
		m_TtypeStart = CGSInfo::TT_TNOON;
		m_thresholdStart = 12;

		m_nbDaysEnd = 3;
		m_TtypeEnd = CGSInfo::TT_TMAX;
		m_thresholdEnd = 5;

		m_firstDay = 0;
		m_lastDay = 365;
		m_FFMC = 85.0;
		m_DMC = 6.0;
		m_DC = 15.0;

		m_carryOverFraction = 1;
		m_effectivenessOfWinterPrcp = 0.75;
	}

	double CFWI::GetFFMC(double oldFFMC, const CHourlyData& data)
	{
		return GetFFMC(oldFFMC, data[H_TAIR], data[H_RELH], data[H_WNDS], data[H_PRCP]);
	}

	double CFWI::GetFFMC(double oldFFMC, const CWeatherDay& data)
	{
		ASSERT(data.IsHourly());
		return GetFFMC(oldFFMC, data[12][H_TAIR], data[12][H_RELH], data[12][H_WNDS], data[H_PRCP][SUM]);
	}

	//T: Temperature (°C)
	//Hr: relative humidity (%)
	//Ws: wind speed (km/h)
	//prcp: precipitation in mm
	double CFWI::GetFFMC(double oldFFMC, double T, double Hr, double Ws, double prcp)
	{
		double wmo = 147.2*(101 - oldFFMC) / (59.5 + oldFFMC);
		if (prcp > 0.5)
		{
			double ra = prcp - 0.5;
			if (wmo > 150)
				wmo += 42.5*ra*exp(-100.0 / (251 - wmo))*(1.0 - exp(-6.93 / ra)) + 0.0015*(wmo - 150)*(wmo - 150)*sqrt(ra);
			else wmo += 42.5*ra*exp(-100.0 / (251 - wmo))*(1.0 - exp(-6.93 / ra));
		}

		if (wmo > 250)
			wmo = 250;

		double temp = T;
		double ed = 0.942*pow(Hr, 0.679) + (11.0*exp((Hr - 100.0) / 10.0)) + 0.18*(21.1 - temp)*(1.0 - 1.0 / exp(Hr*0.115));
		double ew = 0.618*pow(Hr, 0.753) + (10.0*exp((Hr - 100.0) / 10.0)) + 0.18*(21.1 - temp)*(1.0 - 1.0 / exp(Hr*0.115));

		double wm = wmo;
		if (wm < ed && wm < ew)
		{
			double z = 0.424*(1.0 - pow(((100.0 - Hr) / 100.0), 1.7)) + 0.0694*sqrt(Ws)*(1.0 - pow((100.0 - Hr) / 100.0, 8.0));
			double x = z*0.581*exp(0.0365*temp);
			wm = ew - (ew - wmo) / pow(10.0, x);
		}
		else if (wm > ed)
		{
			double z = 0.424*(1.0 - pow((Hr / 100.), 1.7)) + 0.0694*sqrt(Ws)*(1 - pow(Hr / 100, 8.0));
			double x = z*0.581*exp(0.0365*temp);
			wm = ed + (wmo - ed) / pow(10.0, x);
		}


		double ffmc = max(0.0, min(101.0, 59.5*(250.0 - wm) / (147.2 + wm)));

		return ffmc;
	}


	double CFWI::GetDMC(double oldDMC, const CHourlyData& data)
	{
		return GetDMC(oldDMC, data.GetTRef().GetMonth(), data[H_TAIR], data[H_RELH], data[H_PRCP]);
	}

	double CFWI::GetDMC(double oldDMC, const CWeatherDay& data)
	{
		ASSERT(data.IsHourly());
		return GetDMC(oldDMC, data.GetTRef().GetMonth(), data[12][H_TAIR], data[12][H_RELH], data[H_PRCP][SUM]);
	}


	double CFWI::GetDMC(double oldDMC, size_t m, double T, double Hr, double prcp)
	{
		//short m = day.Month();
		static const double el[12] = { 6.5, 7.5, 9.0, 12.8, 13.9, 13.9, 12.4, 10.9, 9.4, 8.0, 7.0, 6.0 };

		double temp = T;
		double t = max(-1.1, temp);

		double rk = 1.894*(t + 1.1)*(100.0 - Hr)*el[m] * 0.0001;

		_ASSERTE(oldDMC >= 0);
		double pr = oldDMC;
		if (prcp > 1.5)
		{
			double ra = prcp;
			double rw = 0.92*ra - 1.27;
			double wmi = 20.0 + 280.0 / exp(0.023*oldDMC);

			double b = 6.2*log(oldDMC) - 17.2;
			if (oldDMC <= 33)
				b = 100.0 / (0.5 + 0.3*oldDMC);
			else if (oldDMC <= 65)
				b = 14.0 - 1.3*log(oldDMC);

			double wmr = wmi + 1000.0*rw / (48.77 + b*rw);
			pr = max(0.0, 43.43*(5.6348 - log(wmr - 20.0)));
		}

		double dmc = max(0.0, pr + rk);

		return dmc;
	}

	double CFWI::GetDC(double oldDC, const CHourlyData& data)
	{
		return GetDC(oldDC, data.GetTRef().GetMonth(), data[H_TAIR], data[H_PRCP]);
	}

	double CFWI::GetDC(double oldDC, const CWeatherDay& data)
	{
		ASSERT(data.IsHourly());
		return GetDC(oldDC, data.GetTRef().GetMonth(), data[12][H_TAIR], data[H_PRCP][SUM]);
	}

	double CFWI::GetDC(double oldDC, size_t m, double T, double prcp)
	{
		ASSERT(oldDC >= 0);
		static const double fl[12] = { -1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5.0, 2.4, 0.4, -1.6, -1.6 };

		double temp = T;
		double t = max(-2.8, temp);

		double pe = max(0.0, (0.36*(t + 2.8) + fl[m]) / 2.0);

		double dr = oldDC;
		if (prcp > 2.8)
		{
			double ra = prcp;
			double rw = 0.83*ra - 1.27;
			double smi = 800 * exp(-oldDC / 400);
			dr = max(0.0, oldDC - 400.0*log(1.0 + 3.937*rw / smi));
		}

		double dc = max(0.0, dr + pe);

		return dc;
	}

	double CFWI::GetISI(double FFMC, const CHourlyData& data)
	{
		return GetISI(FFMC, data[H_WNDS]);
	}

	double CFWI::GetISI(double FFMC, const CWeatherDay& data)
	{
		return GetISI(FFMC, data[12][H_WNDS]);
	}

	double CFWI::GetISI(double FFMC, double Ws)
	{
		double fm = 147.2*(101.0 - FFMC) / (59.5 + FFMC);
		double sf = 19.115*exp(-0.1386*fm)*(1.0 + pow(fm, 5.31) / 4.93e07);
		double isi = sf*exp(0.05039*Ws);

		return isi;
	}

	double CFWI::GetBUI(double dmc, double dc)
	{
		double bui = 0;

		if (dmc == 0 && dc == 0) bui = 0;
		else bui = 0.8*dc*dmc / (dmc + 0.4*dc);

		if (bui < dmc)
		{
			double p = (dmc - bui) / dmc;
			double cc = 0.92 + pow((0.0114*dmc), 1.7);
			bui = max(0.0, dmc - cc*p);
		}

		return bui;
	}

	double CFWI::GetFWI(double bui, double isi)
	{
		double bb = 0;
		if (bui > 80)
			bb = 0.1*isi*(1000.0 / (25.0 + 108.64 / exp(0.023*bui)));
		else bb = 0.1*isi*(0.626*pow(bui, 0.809) + 2.0);

		double fwi = exp(2.72*pow(0.434*log(bb), 0.647));
		if (bb <= 1)fwi = bb;

		return fwi;
	}

	double CFWI::GetDSR(double fwi)
	{
		double dsr = 0.0272*pow(fwi, 1.77);
		return dsr;
	}


	//If there is at lean 75% of the day of January an February that have at least 1cm
	//and there is at least 10 cm then hte first day is 3 days after the snowmelt date
	CTRef CFWI::GetFirstDay(const CWeatherYear& weather)
	{
		CTRef firstDate;


		int nbDay = 0;
		double maxSnow = -1;

		CTPeriod period = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CTRef end(period.Begin().GetYear(), JULY, 15);
		for (CTRef TRef = period.Begin(); TRef < end; TRef++)
		{
			if (weather[TRef][H_SNDH][MEAN] > maxSnow)
				maxSnow = weather[TRef][H_SNDH][MEAN];

			if (TRef.GetJDay() < 61 && weather[TRef][H_SNDH][MEAN] > 1)
				nbDay++;
		}


		double snowPropJF = nbDay / 61.0;
		double MINIMUM_SNOW = 10;//in cm
		if (maxSnow >= MINIMUM_SNOW && snowPropJF > 0.75)
		{
			CSnowAnalysis snow;
			firstDate = snow.GetLastSnowTRef(weather) + 3;//3 day after snow melt
		}

		return firstDate;
	}

	CTRef CFWI::GetLastDay(const CWeatherYear& weather)
	{
		//find begin of snow
		CSnowAnalysis snow;
		CTRef TRefSnow = snow.GetFirstSnowTRef(weather);

		CGrowingSeason GS(0, 0, 0, m_TtypeEnd, m_nbDaysEnd, m_thresholdEnd);
		CTPeriod p = GS.GetGrowingSeason(weather);

		//find freeze-up
		CTRef TRefFreezeUp = p.End();

		CTPeriod period = weather.GetEntireTPeriod();
		CTRef TRef = period.End();

		if (TRefSnow.IsInit())
			TRef = min(TRefSnow, TRef);

		if (TRefFreezeUp.IsInit())
			TRef = min(TRefFreezeUp, TRef);


		return TRef;

	}


	//short CFWI::GetFirstDay3xThreshold(const CWeatherYear& weather)
	//{
	//	short firstDay=-1;
	//	
	//
	//	//Beginning of fire weather 
	//	//look for the first occurrence of 3 successive days Tnoon > T_THRESHOLD
	//	for(int jd=0; jd<200; jd++)
	//	{
	//		double Tnoon0 = weather.GetDay(jd).GetTnoon();//GetTnoon(16, weather.GetDay(jd).GetTMin(), weather.GetDay(jd).GetTMax());
	//		double Tnoon1 = weather.GetDay(jd+1).GetTnoon();//GetTnoon(16, weather.GetDay(jd+1).GetTMin(), weather.GetDay(jd+1).GetTMax());
	//		double Tnoon2 = weather.GetDay(jd+2).GetTnoon();//GetTnoon(16, weather.GetDay(jd+2).GetTMin(), weather.GetDay(jd+2).GetTMax());
	//
	//		if( Tnoon0 >= m_thresholdStart && 
	//			Tnoon1 >= m_thresholdStart && 
	//			Tnoon2 >= m_thresholdStart )
	//		{
	//			firstDay=jd+3;//FWI begin after these 3 days
	//			break;
	//		}
	//	}
	//
	////	CTPeriod p = weather.GetGrowingSeason(false, m_nbDaysStart, m_TtypeStart, m_thresholdStart);
	//
	//	return firstDay;
	//}

	/*short CFWI::GetFirstDay3x6mean(const CWeather& weather, short y)
	{
	short firstDay=-1;

	//Beginning of the growing season
	//look for the first occurrence of 3 successive days with frost
	for(int jd=0; jd<200; jd++)
	{
	double Tmean0 = weather[y].GetDay(jd).GetTMean();
	double Tmean1 = weather[y].GetDay(jd+1).GetTMean();
	double Tmean2 = weather[y].GetDay(jd+2).GetTMean();

	if( Tmean0>=6 && Tmean1>=6 && Tmean2>=6 )
	{
	firstDay=jd+3;//it's the tirth day
	break;
	}
	}

	return firstDay;
	}
	*/
	size_t CFWI::GetNbDayLastRain(const CWeatherYear& weather, size_t firstDay)
	{
		size_t nbDay = 0;
		for (size_t jd = firstDay; jd >= 0; jd--, nbDay++)
		{
			//Get the last day with at least 2mm of water
			if (weather.GetDay(jd)[H_PRCP][SUM] > 2)
			{
				break;
			}
		}

		return nbDay;
	}

	short CFWI::GetInitialValue(const CWeatherStation& weather, size_t y, size_t lastDay, double& FFMC, double& DMC, double& DC)
	{
		double lastDC = DC;

		size_t firstDay = GetFirstDay(weather[y]).GetJDay();

		if (firstDay >= 0)
		{
			FFMC = 85;
			DMC = 6;
			DC = 15;
		}
		else
		{
			//no snow
			//Find 3 consecutives days where noon temperature is above m_thresholdStart (12°C)
			//firstDay = GetFirstDay3xThreshold(weather[y]); 
			CGrowingSeason GS(m_nbDaysStart, m_TtypeStart, m_thresholdStart, 0, 0, 0);
			CTPeriod p = GS.GetGrowingSeason(weather[y]);
			//firstDay = p.Begin().GetJDay();
			//firstDay = weather[y].GetFir.GetFirstDayThreshold(m_nbDaysStart, m_TtypeStart, m_thresholdStart, '>').GetJDay();

			if (p.Begin().IsInit())
			{
				firstDay = p.Begin().GetJDay();
				size_t nbDay = GetNbDayLastRain(weather[y], firstDay);
				FFMC = 85;
				DMC = 2 * nbDay;
				DC = 5 * nbDay;
			}
			else
			{
				//we start the first on july
				//a very cold place, no fire problem...
				firstDay = 183;
				FFMC = 85;
				DMC = 6;
				DC = 15;
			}
		}

		//now we apply DC transfer
		if (y > 0 && lastDay >= 0)
		{
			ASSERT(y > 0);
			CTPeriod p(CJDayRef(weather[y - 1].GetTRef().GetYear(), lastDay), CJDayRef(weather[y].GetTRef().GetYear(), firstDay));
			double Rw = weather.GetStat(H_PRCP, p)[SUM];

			if (Rw < 200)
			{
				//if sum of precipitation is under 200, then we report last DC
				const double a = m_carryOverFraction;
				const double b = m_effectivenessOfWinterPrcp;

				double Qf = 800 * exp(-lastDC / 400);
				double Qs = a*Qf + b*3.94*Rw;
				DC = max(0.0, 400 * log(800 / Qs));
			}
		}

		return firstDay;
	}


	void CFWI::Execute(const CWeatherStation& weather, CModelStatVector& output)
	{
		ASSERT(weather.IsHourly());


		output.clear();
		output.Init(weather.GetEntireTPeriod(CTM(m_method == ALL_HOURS_CALCULATION?CTM::HOURLY:CTM::DAILY)), CFWIStat::NB_D_STAT, MISSING);


		bool bContinueMode = false;
		size_t firstDay = NOT_INIT;
		size_t lastDay = NOT_INIT;
		double oldFFMC = m_FFMC;
		double oldDMC = m_DMC;
		double oldDC = m_DC;

		for (size_t y = 0; y < weather.size(); y++)//for all years
		{
			int year = weather.GetFirstYear() + y;

			if (m_bAutoSelect)//auto init parameter
			{
				if (bContinueMode)
					firstDay = 0;
				else
					firstDay = GetInitialValue(weather, y, lastDay, oldFFMC, oldDMC, oldDC);

				//compute the new last day for this year
				lastDay = GetLastDay(weather[y]).GetJDay();
			}
			else
			{
				firstDay = m_firstDay.GetTRef(year).GetJDay();
				lastDay = m_lastDay.GetTRef(year).GetJDay();
				oldFFMC = m_FFMC;
				oldDMC = m_DMC;
				oldDC = m_DC;
			}

			if (m_method == ALL_HOURS_CALCULATION)
			{

				CTPeriod p = weather[y].GetEntireTPeriod(CTM::HOURLY);
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all day
				{
					const CHourlyData& hour = weather.GetHour(TRef);
					size_t jd = TRef.GetJDay();

					if (jd >= firstDay && jd <= lastDay)
					{
						// compute FFMC
						double FFMC = GetFFMC(oldFFMC, hour);

						// compute DMC
						double DMC = GetDMC(oldDMC, hour);

						// compute DC 
						double DC = GetDC(oldDC, hour);

						// compute ISI
						double ISI = GetISI(FFMC, hour);
						//double ISI = max(0.0, -0.04 + 1.1841 * GetISI(FFMC, day));

						// compute BUI from DMC and DC
						double BUI = GetBUI(DMC, DC);

						// compute FWI from BUI ans ISI
						double FWI = GetFWI(BUI, ISI);

						// compute DSR from FWI
						double DSR = GetDSR(FWI);
						ASSERT(DSR < 200);

						//save result
						output[TRef][CFWIStat::TMEAN_NOON] = hour[H_TAIR];
						output[TRef][CFWIStat::RELH_NOON] = hour[H_RELH];
						output[TRef][CFWIStat::WNDS_NOON] = hour[H_WNDS];
						output[TRef][CFWIStat::PRCP] = hour[H_PRCP];
						output[TRef][CFWIStat::FFMC] = FFMC;
						output[TRef][CFWIStat::DMC] = DMC;
						output[TRef][CFWIStat::DC] = DC;
						output[TRef][CFWIStat::ISI] = ISI;
						output[TRef][CFWIStat::BUI] = BUI;
						output[TRef][CFWIStat::FWI] = FWI;
						output[TRef][CFWIStat::DSR] = DSR;

						oldFFMC = FFMC;
						oldDMC = DMC;
						oldDC = DC;
					}
				} //for all hours
			}
			else if (m_method == NOON_CALCULATION)
			{
				CTPeriod p = weather[y].GetEntireTPeriod(CTM::DAILY);
				for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)//for all day
				{
					const CWeatherDay& day = weather.GetDay(TRef);
					size_t jd = TRef.GetJDay();

					if (jd >= firstDay && jd <= lastDay)
					{

						double Tnoon = day[12][H_TAIR];//GetTnoon(16, day.GetTMin(), day.GetTMax());
						double HRnoon = day[12][H_RELH];//GetHrnoon(day.GetTMin(), day.GetTMax(), Tnoon, day[RELH]);
						double WSnoon = day[12][H_WNDS];
						//we make a correction on the wind speed because sometime bioSIM 9.52 return strange value
						//double WSNoon = day[WNDS]>-999?Min(120.0, day.GetWSnoon() ):0;
						//double WSNoon = day[WNDS]>-999?GetWSnoon( Min(50.0, day[WNDS]) ):0;
						//day(TMIN) = Tnoon;
						//day(TMAX) = Tnoon;
						//day(RELH) = Hrnoon;
						//day(WNDS) = WSNoon;

						// compute FFMC
						double FFMC = GetFFMC(oldFFMC, day);

						// compute DMC
						double DMC = GetDMC(oldDMC, day);

						// compute DC 
						double DC = GetDC(oldDC, day);

						// compute ISI
						double ISI = GetISI(FFMC, day);
						//double ISI = max(0.0, -0.04 + 1.1841 * GetISI(FFMC, day));

						// compute BUI from DMC and DC
						double BUI = GetBUI(DMC, DC);

						// compute FWI from BUI ans ISI
						double FWI = GetFWI(BUI, ISI);

						// compute DSR from FWI
						double DSR = GetDSR(FWI);
						ASSERT(DSR < 200);

						//CFWIDay FWIDay( weather[y].GetYear(), d,day, FFMC,DMC,DC,ISI,BUI,FWI,DSR);
						//m_FWIDayArray.Add(FWIDay);

						//save result
						output[TRef][CFWIStat::TMEAN_NOON] = Tnoon;
						output[TRef][CFWIStat::RELH_NOON] = HRnoon;
						output[TRef][CFWIStat::WNDS_NOON] = WSnoon;
						output[TRef][CFWIStat::PRCP] = day[H_PRCP][SUM];
						output[TRef][CFWIStat::FFMC] = FFMC;
						output[TRef][CFWIStat::DMC] = DMC;
						output[TRef][CFWIStat::DC] = DC;
						output[TRef][CFWIStat::ISI] = ISI;
						output[TRef][CFWIStat::BUI] = BUI;
						output[TRef][CFWIStat::FWI] = FWI;
						output[TRef][CFWIStat::DSR] = DSR;

						oldFFMC = FFMC;
						oldDMC = DMC;
						oldDC = DC;
					}
				}
			}//for all day
		}//for all year 
	}



//
//void CFWI::GetResult(CFWIDStatVector& vector)const
//{
//	vector.clear();
//	if( m_FWIDayArray.GetSize() > 0)
//	{
//		//CTRef begin;
//		//CTRef end;
//
//		//begin.SetJDay(m_FWIDayArray.begin()->m_year, m_FWIDayArray.begin()->m_day);
//		//end.SetJDay(m_FWIDayArray.rbegin()->m_year, m_FWIDayArray.rbegin()->m_day);
//
//		CTRef begin(m_FWIDayArray.begin()->m_year, FIRST_MONTH, FIRST_DAY);
//		CTRef end(m_FWIDayArray.rbegin()->m_year, LAST_MONTH, LAST_DAY);
//		vector.SetFirstTRef(begin);
//		vector.resize(end-begin+1, MISSING);
//		
//		for(int i=0; i<m_FWIDayArray.GetSize(); i++)
//		{
//			CTRef d;
//			d.SetJDay(m_FWIDayArray[i].m_year, m_FWIDayArray[i].m_day);
//
//			vector[d][CFWIStat::TMEAN_NOON] = m_FWIDayArray[i].m_weatherDay.GetTMean();
//			vector[d][CFWIStat::RELH_NOON] = m_FWIDayArray[i].m_weatherDay[DAILY_DATA::RELH];
//			vector[d][CFWIStat::WNDS_NOON] = m_FWIDayArray[i].m_weatherDay[DAILY_DATA::WNDS];
//			_ASSERTE( vector[d][CFWIStat::WNDS_NOON] < 100);
//			vector[d][CFWIStat::PRCP] = m_FWIDayArray[i].m_weatherDay[DAILY_DATA::PRCP];
//			vector[d][CFWIStat::FFMC] = m_FWIDayArray[i].m_ffmc;
//			vector[d][CFWIStat::DMC] = m_FWIDayArray[i].m_dmc;
//			vector[d][CFWIStat::DC] = m_FWIDayArray[i].m_dc;
//			vector[d][CFWIStat::ISI] = m_FWIDayArray[i].m_isi;
//			vector[d][CFWIStat::BUI] = m_FWIDayArray[i].m_bui;
//			vector[d][CFWIStat::FWI] = m_FWIDayArray[i].m_fwi;
//			vector[d][CFWIStat::DSR] = m_FWIDayArray[i].m_dsr;
//		}
//	}
//}

//**************************************************************
	void CFWIStat::Covert2D(const CModelStatVector& result, CModelStatVector& resultD)
	{
		_ASSERTE(result.GetNbStat() == CFWIStat::NB_D_STAT);
		CStatistic::SetVMiss(CFWI::MISSING);

		CTPeriod p = result.GetTPeriod();
		if (p.GetTM().Type() == CTM::DAILY)
		{
			resultD = result;
		}
		else
		{
			CTStatMatrix tmp(result, CTM::DAILY);
			resultD.Init(tmp.m_period, CFWIStat::NB_D_STAT, CFWI::MISSING);

			
			for (CTRef d = tmp.m_period.Begin(); d <= tmp.m_period.End(); d++)
			{
				for (size_t v = 0; v<resultD.GetNbStat(); v++)
					resultD[d][v] = tmp[d][v][v == PRCP?SUM:MEAN];
			}
		}
	}

void CFWIStat::Covert2M(const CModelStatVector& resultD, CModelStatVector& resultM)
{
	_ASSERTE( resultD.GetNbStat() == CFWIStat::NB_D_STAT);
	CStatistic::SetVMiss(CFWI::MISSING);

	CTRef firstDate = resultD.GetFirstTRef();
	CTRef lastDate = resultD.GetLastTRef();
	
	int nbYear = lastDate.GetYear()-firstDate.GetYear()+1;
	resultM.SetFirstTRef( CTRef(firstDate.GetYear(), 0) );
	resultM.resize(nbYear*12);

	for(CTRef d=firstDate; d<=lastDate; )
	{
		int year = d.GetYear();//firstDate.GetYear() + y;

		for(int m=0; m<12; m++)
		{
			CStatistic stat[CFWIStat::NB_D_STAT];
			while(d<=lastDate && 
				  d.GetYear() == year &&
				  d.GetMonth() ==  m )
			{
				if( resultD[d][FWI] > CFWI::MISSING ) 
				{
					for(int v=0; v<resultD.GetNbStat(); v++)
						stat[v] += resultD[d][v];
				}

				d++;
			}

			CTRef ref(year, m);
				
			resultM[ref][CFWIStat::NUM_VALUES] = stat[0][NB_VALUE];
			for(int v=0; v<resultD.GetNbStat(); v++)
			{
				//short s = (v==CFWIStat::PRCP)?SUM:MEAN;
				if( v==CFWIStat::PRCP )
				{
					CTPeriod p(CTRef(year, m, FIRST_DAY), CTRef(year, m, LAST_DAY) );
					resultM[ref][CFWIStat::TMEAN_NOON+v] = stat[v][SUM];
					resultM[ref][CFWIStat::TMEAN_MIN+v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, true);
					resultM[ref][CFWIStat::TMEAN_MAX+v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, false);
				}
				else
				{
					resultM[ref][CFWIStat::TMEAN_NOON+v] = stat[v][MEAN];
					resultM[ref][CFWIStat::TMEAN_MIN+v] = stat[v][LOWEST];
					resultM[ref][CFWIStat::TMEAN_MAX+v] = stat[v][HIGHEST];
				}
			}
		}
	}
}

void CFWIStat::Covert2A(const CModelStatVector& resultD, CModelStatVector& resultA)
{
	_ASSERTE( resultD.GetNbStat() == CFWIStat::NB_D_STAT);

	CTRef firstDate = resultD.GetFirstTRef();
	CTRef lastDate = resultD.GetLastTRef();
	
	int nbYear = lastDate.GetYear()-firstDate.GetYear()+1;
	resultA.SetFirstTRef( CTRef((short)firstDate.GetYear()) );
	resultA.resize(nbYear, CFWI::MISSING);

	for(CTRef d=firstDate; d<=lastDate; )
	{
		short year = d.GetYear();
		CTRef firstDay;
		CTRef lastDay;

		CStatistic stat[CFWIStat::NB_D_STAT];
		while(d<=lastDate && 
			  d.GetYear() == year )
		{
			//_ASSERTE( resultD[d][FWI] > -9999 );
			if( resultD[d][FWI] > CFWI::MISSING ) 
			{
				if( !firstDay.IsInit() )
					firstDay = d;

				for(int v=0; v<resultD.GetNbStat(); v++)
					stat[v] += resultD[d][v];

				lastDay = d;
			}

			d++;
		}

			
		CTRef ref((short)year);
		resultA[ref][CFWIStat::NUM_VALUES] = stat[0][NB_VALUE];
		resultA[ref][CFWIStat::FIRST_FWI_DAY] = firstDay.GetJDay()+1;	
		resultA[ref][CFWIStat::LAST_FWI_DAY] = lastDay.GetJDay()+1;
			
		for(int v=0; v<resultD.GetNbStat(); v++)
		{
			if( v==CFWIStat::PRCP )
			{
				CTPeriod p(CTRef(year, FIRST_MONTH, FIRST_DAY), CTRef(year, LAST_MONTH, LAST_DAY) );
				resultA[ref][CFWIStat::TMEAN_NOON+v] = stat[v][SUM];
				resultA[ref][CFWIStat::TMEAN_MIN+v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, true);
				resultA[ref][CFWIStat::TMEAN_MAX+v] = resultD.GetNbDay(CFWIStat::PRCP, "<=1", p, false);
			}
			else
			{
				resultA[ref][CFWIStat::TMEAN_NOON+v] = stat[v][MEAN];
				resultA[ref][CFWIStat::TMEAN_MIN+v] = stat[v][LOWEST];
				resultA[ref][CFWIStat::TMEAN_MAX+v] = stat[v][HIGHEST];
			}
		}
	}
}

}//WBSF

