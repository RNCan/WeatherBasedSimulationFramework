//**********************************************************************
// 27/07/2018	3.1.1	Rémi Saint-Amant    Chnage name SB -> Qc. Compile with VS 2017
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
		VERSION = "3.1.1 (2018)";

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
	//	Aridité
	//	Fraction nivale (ou précipitations sous forme de neige)
	//	Neige total
	//  Radiation totale
	//  Radiation pour la saison de croissance

	enum TOutput
	{
		O_DD_SUM, O_TOTAL_PPT, O_UTIL_PPT, O_GS_PPT, O_TMIN, O_TMEAN, O_TMAX, O_GS_TMEAN, O_JULY_TMEAN,
		O_DAY_WITHOUT_FROST, O_FF_PERIOD_LENGTH, O_GROWING_SEASON_LENGTH, O_FF_PERIOD_BEGIN, O_FFPERIOD_END,
		O_MEAN_VPD, O_MEAN_UVPD, O_ARIDITY, O_SNOW_RATIO, O_ANNUAL_SNOW, O_TOTAL_RAD, O_GS_RAD, NB_OUTPUT
	};

	typedef CModelStatVectorTemplate<NB_OUTPUT> COutputStat;

	ERMsg CClimaticQc::OnExecuteAnnual()
	{
		ERMsg message;

		CDegreeDays DDE(CDegreeDays::DAILY_AVERAGE, m_threshold);
		CModelStatVector DD;
		DDE.Execute(m_weather, DD);
		DD.Transform(CTM(CTM::ANNUAL), SUM);

		CGrowingSeason GS;

		CThornthwaiteET TPET;
		CModelStatVector PET;
		TPET.Execute(m_weather, PET);
		//CTStatMatrix PET;
		//TPET.Transform(CTTransformation(_PET.GetTPeriod(), CTM(CTM::ANNUAL)), _PET, PET);
		PET.Transform(CTM(CTM::ANNUAL), SUM);

		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::ANNUAL));
		COutputStat stat(p);


		
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			CTPeriod utilPeriod(CTRef(year, JUNE, FIRST_DAY), CTRef(year, AUGUST, LAST_DAY));

			double dd = DD[y][0];
			double ppt = m_weather[y].GetStat(H_PRCP)[SUM];
			double utilPpt = m_weather[y].GetStat(H_PRCP, utilPeriod)[SUM];
			double Tmin = m_weather[y][H_TMIN2][MEAN];
			double Tmean = m_weather[y][H_TAIR2][MEAN];
			double Tmax = m_weather[y][H_TMAX2][MEAN];

			CTPeriod FFPeriod = GS.GetFrostFreePeriod(m_weather[y]);
			size_t dayWithoutFrost = m_weather[y].GetNbDays() - GetNbFrostDay(m_weather[y]);
			ASSERT(dayWithoutFrost >= 0 && dayWithoutFrost <= m_weather[y].GetNbDays());
			//ASSERT(GetConsecutiveDayWithoutFrost(m_weather[y], 0) == FFPeriod.GetLength());

			CTPeriod growingSeason = GS.GetGrowingSeason(m_weather[y]);
			double pptGS = m_weather[y](H_PRCP, growingSeason)[SUM];
			double TmeanGS = m_weather[y](H_TAIR2, growingSeason)[MEAN];
			double meanJuly = m_weather[y][JULY][H_TAIR2][MEAN];

			//WARNING: In Climatic Annual, VPS is givent in kPa and are take from database, but here 
			// humidity is an aproximation from temperature
			double UVPD = GetUtilDeficitPressionVapeur(m_weather[y]);//mbars; 
			double VPD = GetDaylightVaporPressureDeficit(m_weather[y]) * 10;//mbarsm, *10 de kPa -> hPas(mBar)
		

			//TPET.SetLoc(m_info.m_loc);
			//(m_weather[y], 0, CThornthwaitePET::POTENTIEL_STANDARD);
			//double ar = TPET.GetWaterDeficit(m_weather[y]) / 10;//in cm
			double ar = max(0.0, PET[y][CThornthwaiteET::S_ET] - m_weather[y].GetStat(H_PRCP)[SUM]) / 10;//in cm

			//double ar = m_weather[y].GetWaterDeficit()/10;//in cm
			//double annualSnow = m_weather[y].GetStat( STAT_SNOW, SUM);

			double annualSnow = 0;
			CTPeriod p = m_weather[y].GetEntireTPeriod();
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				double T = m_weather[TRef][H_TAIR2][MEAN];
				double ppt = m_weather[TRef][H_PRCP][SUM];

				if (T < 0)
					annualSnow += ppt;
			}

			double snowRatio = annualSnow / ppt * 100;
			double rad = m_weather[y][H_SRMJ][SUM];
			double GSRad = m_weather[y](H_SRMJ, growingSeason)[SUM];

			stat[y][O_DD_SUM] = dd;
			stat[y][O_TOTAL_PPT] = ppt;
			stat[y][O_UTIL_PPT] = utilPpt;
			stat[y][O_GS_PPT] = pptGS;
			stat[y][O_TMIN] = Tmin;
			stat[y][O_TMEAN] = Tmean;
			stat[y][O_TMAX] = Tmax;
			stat[y][O_GS_TMEAN] = TmeanGS;
			stat[y][O_JULY_TMEAN] = meanJuly;
			stat[y][O_DAY_WITHOUT_FROST] = dayWithoutFrost;
			stat[y][O_FF_PERIOD_LENGTH] = FFPeriod.GetLength();
			stat[y][O_GROWING_SEASON_LENGTH] = growingSeason.GetLength();
			stat[y][O_FF_PERIOD_BEGIN] = FFPeriod.Begin().GetJDay();
			stat[y][O_FFPERIOD_END] = FFPeriod.End().GetJDay();
			stat[y][O_MEAN_VPD] = VPD; //in mBar
			stat[y][O_MEAN_UVPD] = UVPD; //in mBar
			stat[y][O_ARIDITY] = ar;
			stat[y][O_SNOW_RATIO] = snowRatio;
			stat[y][O_ANNUAL_SNOW] = annualSnow;
			stat[y][O_TOTAL_RAD] = rad;
			stat[y][O_GS_RAD] = GSRad;

			HxGridTestConnection();
		}

		SetOutput(stat);

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

	//deficit pressure in mbars
	double CClimaticQc::GetUtilDeficitPressionVapeur(const CWeatherYear& weather)
	{
		double udpv = 0;
		
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		CTRef TRef = p.Begin();

		for (int d = 151; d <= 242; d++)
		{
			const CWeatherDay& day = (const CWeatherDay&)(weather[TRef + d]);
			double Tmin = day[H_TMIN2][LOWEST];
			double Tmax = day[H_TMAX2][HIGHEST];

			if (Tmin > 0 && Tmax > 0)
			{
				double T1 = 7.5*Tmax / (237.3 + Tmax);
				double T2 = 7.5*Tmin / (237.3 + Tmin);
				udpv += pow(10., T1) - pow(10., T2);
			}

			//udpv2 += day.GetDaylightVaporPressureDeficit();
			//static const double A = -1.88E4;
			//static const double B = -13.1;
			//static const double C = -1.5E-2;
			//static const double D = 8.0E-7;
			//static const double E = -1.69E-11;
			//static const double F = 6.456;
			//double TK = day.GetTMean() + 273.15;

			////svp is the saturation vapor pressure in kPa
			//double i = A/TK + B + C*TK + D*Square(TK) + E*Cube(TK) + F*log(TK);
			//double svp = exp(i);

			//udpv2 += svp*(1-day[DAILY_DATA::RELH]/100);
		}
		udpv *= 6.108;

		return udpv;
	}

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
		double daylightEs = eᵒ(daylightT) * 1000;//Pa

		return max(0.0, daylightEs - weather[H_EA2][MEAN]);
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
		return (weather[H_TMIN2][LOWEST] <= 0 ? 1 : 0);
	}


}