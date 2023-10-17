#include "stdafx.h"
#include <float.h>

#include "Basic/WeatherStation.h"
#include "Basic/UtilMath.h"
#include "ModelBase/SnowMelt.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;

namespace WBSF
{


	//*****************************************************************************
	//CSnowMelt

	//Constructor: Reset states
	CSnowMelt::CSnowMelt(void)
	{
		Reset();
	}

	//Destructor
	CSnowMelt::~CSnowMelt(void)
	{}

	//Reset states
	void CSnowMelt::Reset()
	{
		m_landType = FORESTED;
		m_snowPropModel = BROWN;

		m_C1 = 0.05;
		m_C2 = 31.747;

		m_Tr = 11.632;

		//these parameters was compute from Canadian snow information	
		m_alphaTt = 1.581;
		m_betaTt = 0.021;
		m_alphaTmelt = -3.762;
		m_betaTmelt = -0.043;


		m_lon = -100;//-100 is an arbitrary value
		ComputeTtTmelt();//update longitude dependent parameters

		//clear all results
		m_result.clear();
	}

	//Update m_Tt and m_Tmelt from longitude
	//m_alphaTt and m_betaTt was evaluated from Canadian weather observation
	//m_alphaTmelt and m_betaTmelt was evaluated from Canadian weather observation
	//because of this dependence on the longitude, this model can only used in North America
	void CSnowMelt::ComputeTtTmelt()
	{
		m_Tt = m_alphaTt + m_betaTt*m_lon;
		m_Tmelt = m_alphaTmelt + m_betaTmelt*m_lon;

	}

	//lon: longitue in °
	//result: m_Tt and m_Tmelt will be updated
	void CSnowMelt::SetLon(double lon)
	{
		m_lon = lon;
		ComputeTtTmelt();
	}





	//compute snow accumulation in 2 pass
	//at the end, m_result will contain all output information
	void CSnowMelt::Compute(const CWeatherStation& weather)
	{
		CSnowMeltParam smp;
		Compute(weather, smp, false);//first time: computing snow for entire period
		Compute(weather, smp, true);//second time: update the beginning of the first year until end of snowmelt
	}

	void CSnowMelt::Compute(const CWeatherStation& weather, CSnowMeltParam& smp, bool bUpdateBegin)
	{
		ASSERT(weather.IsComplete("TN TX P"));
		//Main source: 
		//	Brown RD, Brasnett B, Robinson D. 2003. 
		//	Gridded North American monthly snow depth and snow water equivalent for GCM evaluation. 
		//	Atmosphere-Ocean 41: 1-14
		//	Notes from JR:	Poorly written paper. Units not mentioned or poorly so. 
		//					Symbols in equations (7), (8) and (9) not clear.
		//					Numerical steps not clearly described. This is my best interpretation.


		//This model simulate in days
		CTPeriod period = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CTRef ref = period.Begin();
		if (!bUpdateBegin)
		{
			m_result.m_period = period;//.SetFirstDate(ref);
			m_result.resize(period.GetNbDay());
		}

		m_result[0].m_date = ref;
		m_result[0].m_hs = smp.hs;//Snow depth (cm)
		m_result[0].m_rs = smp.rs;//Snow density (kg/m³ or g/cm³)
		m_result[0].m_SWE = smp.SWE;//snow water equivalent (mm cm?????)

		
		int dayIndex = 0;
		for (size_t y = 0; y < weather.GetNbYears(); y++)//for all years
		{
			for (size_t m = 0; m < 12; m++)//for all months
			{
				for (size_t d = 0; d < weather[y][m].GetNbDays(); d++)//for all days
				{
					const CDay& wDay = weather[y][m][d];
					if (y == 0 && m == 0 && d == 0 && wDay[H_SNDH].IsInit())//if we have real snow depth observation
					{
						smp.hs = wDay[H_SNDH];
					}

					short timeStep = 4;
					CDailyWaveVector t;
					wDay.GetHourlyGeneration(t, HG_ALLEN_WAVE, timeStep);
					double prcp = wDay[H_PRCP][SUM] / t.size();//in mm/h
					
					double newSWESum = 0;
					for (size_t h = 0; h < t.size(); h++) //for all time steps in a day (parameter sent toGetAllenWave()
					{
						double T = t[h];
						double snowProp = GetSnowProp(T, m);

						double newSWE = prcp*snowProp; //Snow Water Eguivalent of new snow (mm/h)
						double newRain = prcp*(1 - snowProp);//New Rain (mm/h)

						
						if (wDay[H_SNOW].IsInit())//if we have snow observation
						{
							ASSERT(wDay[H_SNOW][SUM] >= 0);
							//ASSERT(wDay[H_SNOW][SUM] <= wDay[H_PRCP][SUM]+0.1);
							
							newSWE = wDay[H_SNOW][SUM] / t.size();
							
							if (newSWE < 0)
								newSWE = 0;

							if (newSWE > prcp)
								newSWE = prcp;

							newRain = prcp - newSWE;//New Rain (mm/h)
							if (prcp > 0)
							{
								snowProp = newSWE / prcp;
							}
						}
						

						newSWESum += newSWE;

						//compute new snow
						if (newSWE > 0)
						{
							double rsNew = (T <= 0) ? (67.9 + 51.3*exp(T / 2.6)) : (119.2 + 20 * T); //Density of new snow (kg/m³). Brown et al (2003) equs. (1) & (2)
							double hsNew = 100 * newSWE / rsNew; //depth of new snow (cm). From SWE = height(m) x density(kg/m³) (note 1 mm water is 1 kg/m²)

							
							_ASSERTE(rsNew > 0);
							_ASSERTE(hsNew > 0);

							//compute total snow depth and weighted average density
							smp.rs = (smp.SWE*smp.rs + newSWE*rsNew) / (smp.SWE + newSWE);
							smp.hs = smp.hs + hsNew;
						}


						//compute melt if snow
						if (smp.SWE > 0 || newSWE > 0)
						{
							//Melt snow
							double melt = GetMelt(T, smp.rs, timeStep);
							double rainMelt = GetRainMelt(T, newRain);

							smp.SWE = max(0.0, smp.SWE + newSWE - melt - rainMelt); //Add new snow to snow pack, remove all melt terms 
						}

						//Compute compaction and new snow height
						if (smp.SWE > 0)
						{
							//compact snow 
							smp.rs = Compaction(T, smp.SWE, smp.hs, smp.rs, timeStep);

							//compute new snow depth
							smp.hs = (smp.rs > 0) ? 100 * smp.SWE / smp.rs : 0;
						}
						else
						{
							//Reset snow and desity to zero
							smp.hs = 0;
							smp.rs = 0;
						}

						_ASSERTE(smp.SWE < 1000);
						_ASSERTE(smp.rs >= 0);
						_ASSERT(smp.hs >= 0);
						_ASSERT(smp.SWE >= 0);
					}//end for h

					
					_ASSERTE(!_isnan(smp.hs));
					_ASSERTE(!_isnan(smp.rs));
					_ASSERTE(!_isnan(smp.SWE));
					_ASSERTE(newSWESum <= wDay[H_PRCP][SUM] + 0.001);

					
					if (wDay[H_SNDH].IsInit())//if we have real snow depth observation
					{
						smp.hs = wDay[H_SNDH][MEAN];
						if (smp.hs < 0)
							smp.hs = 0;
					}

					//multi annual model
					int year = int(weather.GetFirstYear() + y);
					//we increment index before because observation is always one day later

					m_result[dayIndex].m_newSWE = newSWESum >= 0.2 ? newSWESum : 0;
					if (dayIndex + 1 < (int)m_result.size())
					{
						m_result[dayIndex + 1].m_date = CTRef(year, m, d)++;
						m_result[dayIndex + 1].m_hs = smp.hs;
						m_result[dayIndex + 1].m_rs = smp.rs;
						m_result[dayIndex + 1].m_SWE = smp.SWE;
					}
					dayIndex++;

					//stop if we update the beginning and they are no longer snow 
					if (bUpdateBegin && smp.hs == 0 && smp.SWE == 0)
						return;
				}//for d
			}//for m
		}//for y
	}

	//T: temperature in C°
	//m: month (0 .. 11)
	//return the proportion of snow (0..1)
	double CSnowMelt::GetSnowProp(double T, size_t m)const
	{
		ASSERT(m_Tr != 0);//by default m_Tr = 11.632
		ASSERT(m_snowPropModel >= 0 && m_snowPropModel < NB_SNOWPROP);

		double snowProp = 0;
		if (m_snowPropModel == KIENZLE)
		{
			double Tmts = m_Tt + (m_Tt*sin((m + 3) / 1.91));
			double Tmrs = max(0.1, m_Tr*0.6*(0.55 + sin(m + 5.0)));

			double Term = (T - Tmts) / (1.4*Tmrs);
			double K = Signe(Tmts - T)*6.76*Square(Term);
			double Prain = 0.5 + 5 * pow(Term, 3) + 3.19*Term + K;
			snowProp = 1 - Prain;
		}
		else
		{
			//Proportion falling as snow. Brown et al (2003) page 3, 3rd paragraph of column 2
			snowProp = (m_Tt + m_Tr / 2 - T) / max(0.1, m_Tr);
		}

		return max(0.0, min(1.0, snowProp));
	}

	//T: temperature in C°
	//rs: density in kg/m³
	//timeStep: number of hours for this step
	//return snow melt (mm / h);
	double CSnowMelt::GetMelt(double T, double rs, short timeStep)const
	{
		ASSERT(m_landType >= 0 && m_landType<NB_LAND);

		double gamma = 0;
		if (m_landType == FORESTED)
			gamma = max(1.4, min(3.5, (0.0104*rs - 0.7)));
		else gamma = max(1.5, min(5.5, (0.0196*rs - 2.39)));

		double melt = (T>m_Tmelt) ? (T - m_Tmelt)*gamma*timeStep / 24 : 0; //Brown et al (2003) equ. (5)

		return melt;
	}

	//T: temperature in °C
	//Rain: liquit precipitation in mm/n
	//return effect of rain on melting snow in mm/h
	double CSnowMelt::GetRainMelt(double T, double rain)const
	{
		static const double Cw = 4.184;// J/g·K == (J/g·°C)
		static const double Lf = 333;// J/g
		static const double r_ice = 0.917;// g/cm³
		static const double r_water = 1; //g/cm³

		//Rain melting snow. Brown et al (2003) equ. (6)
		double rainMelt = (Cw*rain*T*r_water) / (Lf*r_ice);

		return rainMelt;
	}

	//T: temperature in °C
	//SWE: snow water equivalent in mm
	//hs: snow depth in cm
	//rs: density of snow in kg/m³
	//timeStep: number of hours for this step in h
	//return new density of snow in kg/m³
	double CSnowMelt::Compaction(double T, double SWE, double hs, double rs, short timeStep)const
	{
		//Compaction 
		if (hs > 0) //compact old snow only if there is old snow to compact
		{
			if (T > m_Tmelt) //Warm compaction
			{
				double rstar = 700 - (20470 / hs)*(1 - exp(-hs / 67.3)); //Brown et al (2003) equ. (9)
				for (int h = 0; h < timeStep; h++)
				{
					if (rs < rstar)
						rs = rstar - 200 * exp(log((rstar - rs) / 200) - 3600 * 0.000002778); //Brown et al (2003) equ. (8)
				}
			}
			else //Cold compaction
			{
				static const double F = 0.6;
				for (int h = 0; h < timeStep; h++)
				{
					//SWE convert from mm to cm
					//rs convert from kg/m³ to g/cm³
					double deltar = 1000 * m_C1*exp(-0.08*(m_Tmelt - T))*F*(SWE/10)*exp(-m_C2*rs / 1000); //Brown et al (2003) equ. (7)
					rs = min(700.0, rs + deltar);
				}
			}

			
		}
		else //If there is no old snow, make sure its depth and density are zero
		{
			rs = 0;
		}

		_ASSERTE(!_isnan(rs));
		return rs;
	}
}