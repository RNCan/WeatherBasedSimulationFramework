//**********************************************************************
// 28/05/2025	1.0		Rémi Saint-Amant	Creation
//**********************************************************************

#include "VHR_to_Landsat.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/DegreeDays.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//CDD: cumulative degree-days over 5°C (Double-Sine after January 1st)
	//SWI: Summer Warmth Index, Walker 2003
	//MST: Mean summer temperature
	//MWT: Mean winter temperature
	//MSP: Mean summer precipitation
	//MWP: Mean winter precipitation.
	//MCMT: mean coldest month temperature
	//MWMT: mean warmest month temperature

	enum TAnnualStat { O_CDD, O_SWI, O_MST, O_MWT, O_MSP, O_MWP, O_MCMT, O_MWMT, NB_OUTPUTS };


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CVHR2LandsatModel::CreateObject);

	//Contructor
	CVHR2LandsatModel::CVHR2LandsatModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 0;
		VERSION = "1.0.0 (2025)";

		//m_nb_years = 1;//Annual;
	}

	CVHR2LandsatModel::~CVHR2LandsatModel()
	{}


	
	//this method is call to load your parameter in your variable
	ERMsg CVHR2LandsatModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		int c = 0;
		/*m_nb_years = parameters[c++].GetInt();
		if (m_nb_years == 0)
			m_nb_years = m_weather.size();

		if (m_nb_years > m_weather.size())
			msg.ajoute("Invalid input parameters: number of years must be lesser than the number of weather years.");*/


		return msg;
	}


	ERMsg CVHR2LandsatModel::OnExecuteAnnual()
	{
		//ASSERT(m_nb_years <= m_weather.size());

		ERMsg msg;

		CTPeriod p = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		m_output.Init(p, NB_OUTPUTS, -999);

		
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather.GetFirstYear() + int(y);

		
			
			m_output[y][O_CDD] = CDD(m_weather[y]);
			m_output[y][O_SWI] = SWI(m_weather[y]);
			m_output[y][O_MST] = MST(m_weather[y]);
			m_output[y][O_MSP] = MSP(m_weather[y]);
			m_output[y][O_MCMT] = MCMT(m_weather[y]);
			m_output[y][O_MWMT] = MWMT(m_weather[y]);

			if (y > 0)
			{
				m_output[y][O_MWT] = MWT(m_weather, year);
				m_output[y][O_MWP] = MWP(m_weather, year);
			}
		}

		return msg;
	}

	//CDD:cumulative degree-days over 5°C (Double-Sine after January 1st)
	double CVHR2LandsatModel::CDD(const CWeatherYear& weather, double threshold)
	{
		CDegreeDays model(CDegreeDays::ALLEN_WAVE, threshold);

		CModelStatVector output;
		model.Execute(weather, output);

		//CTPeriod pp = output.GetTPeriod();

		double CDD = 0;
		for (size_t i=0; i < output.size(); i++)
			CDD += output[i][CDegreeDays::S_DD];


		return CDD;
	}

	//SWI:Summer Warmth Index: sum of mean monthly temperatures greater than 0°C
	double CVHR2LandsatModel::SWI(const CWeatherYear& weather)
	{
		double SWI = 0;

		for (size_t m = 0; m < 12; m++)
		{
			double Tmean = weather[m].GetStat(H_TNTX)[MEAN];
			if (Tmean > 0)
				SWI += Tmean;
		}

		return SWI;
	}

	//MST:Mean summer temperature
	double CVHR2LandsatModel::MST(const CWeatherYear& weather)
	{
		int year = weather.GetTRef().GetYear();
		return weather.GetStat(H_TNTX, CTPeriod(year,MAY,DAY_01, year,SEPTEMBER,DAY_30))[MEAN];
	}

	//MWT:Mean winter temperature
	double CVHR2LandsatModel::MWT(const CWeatherYears& weather, int year)
	{
		//int year = weather.GetTRef().GetYear();
		//CStatistic stats;
		//stats += weather[DECEMBER].GetStat(H_TNTX);
		//stats += weather[JANUARY].GetStat(H_TNTX);
		//stats += weather[FEBRUARY].GetStat(H_TNTX);
		//
		//double R1 = stats[MEAN];
		//double R2 = weather.GetStat(H_TNTX, CTPeriod(year, OCTOBER, DAY_01, year, FEBRUARY, LAST_DAY))[MEAN];
		//assert(Round(R1, 2) == Round(R2, 2));


		return weather.GetStat(H_TNTX, CTPeriod(year - 1, DECEMBER, DAY_01, year, FEBRUARY, LAST_DAY))[MEAN];
	}

	//MSP:Mean summer precipitation
	double CVHR2LandsatModel::MSP(const CWeatherYear& weather)
	{
		int year = weather.GetTRef().GetYear();

		CStatistic stats;
		for(size_t m=MAY; m<=SEPTEMBER; m++)
			stats += weather[m].GetStat(H_PRCP)[SUM];
		
		return stats[MEAN];

		//return weather.GetStat(H_PRCP, CTPeriod(year, JUNE, DAY_01, year, JULY, DAY_31))[MEAN];
	}

	//MWP:Mean winter precipitation.
	double CVHR2LandsatModel::MWP(const CWeatherYears& weather, int year)
	{
		ASSERT(weather.IsYearInit(year));
		//int year = weather.GetTRef().GetYear();
		CStatistic stats;

		stats += weather[year-1][DECEMBER].GetStat(H_PRCP)[SUM];
		stats += weather[year][JANUARY].GetStat(H_PRCP)[SUM];
		stats += weather[year][FEBRUARY].GetStat(H_PRCP)[SUM];

		//double R1 = stats[MEAN];
		//double R2 = weather.GetStat(H_PRCP, CTPeriod(year, OCTOBER, DAY_01, year, FEBRUARY, LAST_DAY))[MEAN];
		//assert(Round(R1, 2) == Round(R2, 2));

		return stats[MEAN];
		//return weather.GetStat(H_PRCP, CTPeriod(year-1, OCTOBER, DAY_01, year, FEBRUARY, LAST_DAY))[MEAN];
	}

	//MCMT : mean coldest month temperature
	double CVHR2LandsatModel::MCMT(const CWeatherYear& weather)
	{
		double minT = 999;
		for (size_t m = 0; m < 12; m++)
		{
			double T = weather[m].GetStat(H_TNTX)[MEAN];
			if (T < minT)
				minT = T;
		}

		return minT;
	}

	//MWMT : mean warmest month temperature
	double CVHR2LandsatModel::MWMT(const CWeatherYear& weather)
	{
		double maxT = -999;
		for (size_t m = 0; m < 12; m++)
		{
			double T = weather[m].GetStat(H_TNTX)[MEAN];
			if (T > maxT)
				maxT = T;
		}

		return maxT;
	}

}