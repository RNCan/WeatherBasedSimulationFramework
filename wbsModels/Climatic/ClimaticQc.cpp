//**********************************************************************
// 10/09/2018	4.0.0	Rémi Saint-Amant    Change in units of aridity, pet 
//											mean of VPD instead of summation
//											Add PET as output
// 10/09/2018	3.1.2	Rémi Saint-Amant    Bug correction in aridity 
// 27/07/2018	3.1.1	Rémi Saint-Amant    Change in name SB -> Qc. Compile with VS 2017
// 20/09/2016	3.1.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 21/01/2016	3.0.0	Rémi Saint-Amant	Using Weather-based simulation framework (WBSF)
// 03/03/2009			Rémi Saint-Amant	Update with new BioSIMModelBase (hxGrid)
// 20/11/2008			Rémi Saint-Amant	New variable: TMean growing season and July Tmean
//**********************************************************************
#include <stdio.h>
#include <math.h>
#include <crtdbg.h>
#include <float.h>
#include <limits>

#include "basic/WeatherDefine.h"
#include "basic/Evapotranspiration.h"
#include "Basic/DegreeDays.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"
#include "ClimaticQc.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CClimaticQc::CreateObject);


	static double GetDaylightVaporPressureDeficit(const CWeatherYear& weather);
	static double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather);
	static double GetDaylightVaporPressureDeficit(const CWeatherDay& weather);

	static double GetNbDayWithPrcp(const CWeatherYear& weather);
	static double GetNbDayWithPrcp(const CWeatherMonth& weather);
	static double GetNbDayWithPrcp(const CWeatherDay& weather);

	static double GetNbFrostDay(const CWeatherYear& weather);
	static double GetNbFrostDay(const CWeatherMonth& weather);
	static double GetNbFrostDay(const CWeatherDay& weather);



	CClimaticQc::CClimaticQc()
	{
		NB_INPUT_PARAMETER = 1;
		VERSION = "4.0.0 (2018)";

		// initialise your variable here (optionnal)
		m_threshold = 0;
	}

	CClimaticQc::~CClimaticQc()
	{}


	//	Degré Jour > m_threshold degré C
	//	Précipitations totales
	//	Précipitations utiles (juin, juillet,août)
	//	Precipitations saison de croissance
	//  Température moyenne minimum annuelle
	//	Température moyenne annuelle
	//  Température moyenne maximum annuelle
	//	Température moyenne de la saison de croissance
	//	Température moyenne de juillet
	//	Jours sans gel
	//	Jours sans gel consécutifs
	//	Saison de croissance
	//	Gel tardif
	//	Gel hatif
	//	Déficit de prévision de valeur utile
	//	Déficit de prévision de valeur annuelle
	//  Évapotranspiration potentiel
	//	Aridité
	//	Fraction nivale (ou précipitations sous forme de neige)
	//	Neige total
	//  Radiation totale
	//  Radiation pour la saison de croissance

	enum TOutput
	{
		O_DD_SUM, O_TOTAL_PPT, O_UTIL_PPT, O_GS_PPT, O_TMIN, O_TMEAN, O_TMAX, O_GS_TMEAN, O_JULY_TMEAN,
		O_DAY_WITHOUT_FROST, O_FF_PERIOD_LENGTH, O_GROWING_SEASON_LENGTH, O_FF_PERIOD_BEGIN, O_FFPERIOD_END,
		O_MEAN_DAYLIGHT_VPD, O_MEAN_UVPD, O_PET, O_ARIDITY, O_SNOW_RATIO, O_ANNUAL_SNOW, O_TOTAL_RAD, O_GS_RAD, NB_OUTPUTS
	};

	ERMsg CClimaticQc::OnExecuteAnnual()
	{
		ERMsg message;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		m_output.Init(p, NB_OUTPUTS, -999);


		CDegreeDays DDE(CDegreeDays::DAILY_AVERAGE, m_threshold);
		CModelStatVector DD;
		DDE.Execute(m_weather, DD);
		DD.Transform(CTM(CTM::ANNUAL), SUM);

		CGrowingSeason GS;

		CThornthwaiteET TPET;
		CModelStatVector PET;
		TPET.Execute(m_weather, PET);
		PET.Transform(CTM(CTM::MONTHLY), SUM);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod utilPeriod(CTRef(year, JUNE, FIRST_DAY), CTRef(year, AUGUST, LAST_DAY));

			double dd = DD[y][0];
			double ppt = m_weather[y].GetStat(H_PRCP)[SUM];
			double utilPpt = m_weather[y].GetStat(H_PRCP, utilPeriod)[SUM];
			double Tmin = m_weather[y][H_TMIN][MEAN];
			double Tmean = m_weather[y][H_TAIR][MEAN];
			double Tmax = m_weather[y][H_TMAX][MEAN];

			CTPeriod FFPeriod = GS.GetFrostFreePeriod(m_weather[y]);
			size_t dayWithoutFrost = m_weather[y].GetNbDays() - GetNbFrostDay(m_weather[y]);
			ASSERT(dayWithoutFrost >= 0 && dayWithoutFrost <= m_weather[y].GetNbDays());

			CTPeriod growingSeason = GS.GetGrowingSeason(m_weather[y]);
			double pptGS = m_weather[y](H_PRCP, growingSeason)[SUM];
			double TmeanGS = m_weather[y](H_TAIR, growingSeason)[MEAN];
			double meanJuly = m_weather[y][JULY][H_TAIR][MEAN];

			double UVPD = GetUtilDeficitPressionVapeur(m_weather[y]) * 10.0;//[kPa] -> [hPa](mBar)
			double VPD = GetDaylightVaporPressureDeficit(m_weather[y]) * 10.0;//[kPa] -> [hPa](mBar)

			double PETa = 0;
			double ar = 0;
			for (size_t m = 0; m < 12; m++)
			{
				double PETm = PET[y * 12 + m][CThornthwaiteET::S_ET];
				double pptm = m_weather[y][m].GetStat(H_PRCP)[SUM];
				PETa += PETm;
				ar += max(0.0, PETm - pptm);//in mm since version 4.0.0
			}


			double annualSnow = 0;
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				double T = m_weather[TRef][H_TAIR][MEAN];
				double ppt = m_weather[TRef][H_PRCP][SUM];

				if (T < 0)
					annualSnow += ppt;
			}

			double snowRatio = annualSnow / ppt * 100;
			double rad = m_weather[y][H_SRMJ][SUM];
			double GSRad = m_weather[y](H_SRMJ, growingSeason)[SUM];

			m_output[y][O_DD_SUM] = dd;
			m_output[y][O_TOTAL_PPT] = ppt; //[mm]
			m_output[y][O_UTIL_PPT] = utilPpt;//[mm]
			m_output[y][O_GS_PPT] = pptGS;//[mm]
			m_output[y][O_TMIN] = Tmin;//°C
			m_output[y][O_TMEAN] = Tmean;//°C
			m_output[y][O_TMAX] = Tmax;//°C
			m_output[y][O_GS_TMEAN] = TmeanGS;//°C
			m_output[y][O_JULY_TMEAN] = meanJuly;//°C
			m_output[y][O_DAY_WITHOUT_FROST] = dayWithoutFrost;
			m_output[y][O_FF_PERIOD_LENGTH] = FFPeriod.as(CTM::DAILY).GetLength();
			m_output[y][O_GROWING_SEASON_LENGTH] = growingSeason.as(CTM::DAILY).GetLength();
			m_output[y][O_FF_PERIOD_BEGIN] = FFPeriod.Begin().GetJDay();
			m_output[y][O_FFPERIOD_END] = FFPeriod.End().GetJDay();
			m_output[y][O_MEAN_DAYLIGHT_VPD] = VPD; //[hPa] (mBar)
			m_output[y][O_MEAN_UVPD] = UVPD; //[hPa] (mBar)
			m_output[y][O_PET] = PETa;//[mm]
			m_output[y][O_ARIDITY] = ar;//[mm] since version 4.0., aridity are in mm, was in cm
			m_output[y][O_SNOW_RATIO] = snowRatio;//[%]
			m_output[y][O_ANNUAL_SNOW] = annualSnow; //[mm] of water
			m_output[y][O_TOTAL_RAD] = rad;//[MJ/m²]
			m_output[y][O_GS_RAD] = GSRad;//[MJ/m²]

			HxGridTestConnection();
		}



		return message;
	}


	/*long CClimaticQc::GetConsecutiveDayWithoutFrost(const CWeatherYear& weather, double th)
	{
		long nbDayMax = 0;
		int d = 0;
		while (d < weather.GetNbDay())
		{
			bool frostDay = false;
			long nbDay = 0;
			for (; d < weather.GetNbDay() && !frostDay; d++)
			{
				frostDay = weather.GetDay(d).GetFrostDay(th) != 0;
				if (!frostDay)
					nbDay++;
			}

			nbDayMax = __max(nbDayMax, nbDay);
		}

		return nbDayMax;
	}*/

	//mean of June, July and August vapor pressure deficit [kPa]
	double CClimaticQc::GetUtilDeficitPressionVapeur(const CWeatherYear& weather)
	{
		CStatistic udpv;

		int year = weather.GetTRef().GetYear();
		CTPeriod p(CTRef(year, JUNE, FIRST_DAY), CTRef(year, AUGUST, LAST_DAY));
		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			const CWeatherDay& day = weather.GetDay(TRef);
			double Tmin = day[H_TMIN][LOWEST];
			double Tmax = day[H_TMAX][HIGHEST];

			if (Tmin > 0 && Tmax > 0)
			{
				double T1 = 7.5*Tmax / (237.3 + Tmax);
				double T2 = 7.5*Tmin / (237.3 + Tmin);
				udpv += 0.6108*(pow(10., T1) - pow(10., T2));//[kPa]
			}
		}


		return udpv[MEAN];
	}

	//udpv2 += day.GetDaylightVaporPressureDeficit();


	//from wikipedia :https://en.wikipedia.org/wiki/Vapour-pressure_deficit
	//{\displaystyle A = -1.044\times 10 ^ {4}},
	//{ \displaystyle B = -11.29 } {\displaystyle B = -11.29},
	//{ \displaystyle C = -2.7\times 10 ^ {-2} } {\displaystyle C = -2.7\times 10 ^ {-2}},
	//{ \displaystyle D = 1.289\times 10 ^ {-5} } {\displaystyle D = 1.289\times 10 ^ {-5}},
	//{ \displaystyle E = -2.478\times 10 ^ {-9} } {\displaystyle E = -2.478\times 10 ^ {-9}},
	//{ \displaystyle F = 6.456 } F = 6.456,
	//{ \displaystyle T } T is temperature of t
	//	//static const double A = -1.88E4;
	//	//static const double B = -13.1;
	//	//static const double C = -1.5E-2;
	//	//static const double D = 8.0E-7;
	//	//static const double E = -1.69E-11;
	//	//static const double F = 6.456;
	//	double TT = (day.GetTMean() - 491.67) × 5 / 9
	//	//double TK = day.GetTMean() + 273.15;

	//	////svp is the saturation vapor pressure in kPa
	//	//double i = A/TK + B + C*TK + D*Square(TK) + E*Cube(TK) + F*log(TK);
	//	//double svp = exp(i);

	//	//udpv2 += svp*(1-day[DAILY_DATA::RELH]/100);


	////static const double A = -1.88E4;
	////static const double B = -13.1;
	////static const double C = -1.5E-2;
	////static const double D = 8.0E-7;
	////static const double E = -1.69E-11;
	////static const double F = 6.456;
	//double TT = (T(°R) - 491.67) × 5 / 9
	////double TK = day.GetTMean() + 273.15;

	////svp is the saturation vapor pressure in kPa
	//double i = A/TK + B + C*TK + D*Square(TK) + E*Cube(TK) + F*log(TK);
	//double svp = exp(i);

	//udpv2 += svp*(1-day[DAILY_DATA::RELH]/100);

	//this method is call to load your parameter in your variable
	ERMsg CClimaticQc::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg message;

		//transfer your parameter here
		int c = 0;
		m_threshold = parameters[c++].GetReal();
		//	m_snowModelType = parameters[c++].GetInt();


		return message;
	}

	//Saturation vapor pressure at daylight temperature[kPa]
	double GetDaylightVaporPressureDeficit(const CWeatherYear& weather)
	{
		CStatistic stat;
		for (size_t m = 0; m < weather.size(); m++)
			stat += GetDaylightVaporPressureDeficit(weather[m]);

		return stat[MEAN];
	}
	//Saturation vapor pressure at daylight temperature[kPa]
	double GetDaylightVaporPressureDeficit(const CWeatherMonth& weather)
	{
		CStatistic stat;
		for (size_t d = 0; d < weather.size(); d++)
			stat += GetDaylightVaporPressureDeficit(weather[d]);

		return stat[MEAN];
	}

	//Saturation vapor pressure at daylight temperature[kPa]
	double GetDaylightVaporPressureDeficit(const CWeatherDay& weather)
	{
		ASSERT(weather[H_EA].IsInit());

		double daylightT = weather.GetTdaylight();
		double daylightEs = eᵒ(daylightT);//kPa

		return max(0.0, daylightEs - weather[H_EA][MEAN]);//[kPa]
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
		return (weather[H_PRCP][SUM] >= 0.2 ? 1 : 0);
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