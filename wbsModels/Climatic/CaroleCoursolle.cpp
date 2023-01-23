//**********************************************************************
// 25/10/2022	1.0		Rémi Saint-Amant	Creation
//**********************************************************************

#include "CaroleCoursolle.h"
#include "Basic/Evapotranspiration.h"
#include "Basic/DegreeDays.h"
#include "Basic/GrowingSeason.h"
#include "ModelBase/EntryPoint.h"


using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CCCModel::CreateObject);

	//Contructor
	CCCModel::CCCModel()
	{
		//specify the number of input parameter
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.0.2 (2023)";

		m_nb_years = 1;//Annual;
	}

	CCCModel::~CCCModel()
	{}

	
	//MAT : mean annual air temperature
	//MCMT : mean coldest month temperature
	//EMT : mean of annual extreme minimum temperature
	//MWMT : mean warmest month temperature
	//BFFP : begin of frost free period
	//EFFP : end of frost free period
	//FFPL : frost free period length (days) 
	//NFFD : number of frost free days
	//CHDD0 : chilling degree days (< 0°C) from August 1st
	//CHDD5 : chilling degree days (< 5°C) from August 1st
	//CDD5: cumulative degree-days over 5°C (Daily Average after January 1st)
	//PPT: total annul precipitation
	//PPT5: total precipitation from May to September
	enum TAnnualStat { O_MAT, O_MCMT, O_EMT, O_MWMT, O_BFFP, O_EFFP, O_FFPL, O_NFFD, O_CHDD0, O_CHDD5, O_PPT, O_PPT5, NB_ANNUAL_STATS };
	

	//this method is call to load your parameter in your variable
	ERMsg CCCModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameter here
		int c = 0;
		m_nb_years = parameters[c++].GetInt();
		if (m_nb_years == 0)
			m_nb_years = m_weather.size();

		if (m_nb_years > m_weather.size())
			msg.ajoute("Invalid input parameters: number of years must be lesser than the number of weather years.");


		return msg;
	}


	ERMsg CCCModel::OnExecuteAnnual()
	{
		ASSERT(m_nb_years <= m_weather.size());

		ERMsg msg;

		CModelStatVector output;
		output.Init(m_weather.GetEntireTPeriod(CTM::ANNUAL), NB_ANNUAL_STATS, -999);

		
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather.GetFirstYear() + int(y);

			CFrostFreePeriod  FF;
			CTPeriod FFp = FF.GetPeriod(m_weather[y]);


			output[y][O_MAT] = MAT(m_weather[y]);
			output[y][O_MCMT] = MCMT(m_weather[y]);
			output[y][O_EMT] = EMT(m_weather[y]);
			output[y][O_MWMT] = MWMT(m_weather[y]);
			output[y][O_BFFP] = FFp.Begin().GetJDay() + 1;//Base 1
			output[y][O_EFFP] = FFp.End().GetJDay() + 1;//Base 1
			output[y][O_FFPL] = FFp.GetNbDay();
			output[y][O_NFFD] = NFFD(m_weather[y]);
			output[y][O_CHDD0] = CHDD(m_weather[y], 0);
			output[y][O_CHDD5] = CHDD(m_weather[y], 5);
			output[y][O_PPT] = m_weather[y].GetStat(H_PRCP)[SUM];
			output[y][O_PPT5] = PPT5(m_weather[y]);
			
			//output[y][O_CDD5] = CDD5(m_weather[y]);
		}


		CTPeriod p = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		p.Begin() += int(m_nb_years) - 1;

		m_output.Init(p, NB_ANNUAL_STATS, 0);

		//compute mean for the period;
		for (size_t y = 0; y < m_weather.GetNbYears()- m_nb_years+1; y++)
		{
			array <CStatistic, NB_ANNUAL_STATS> stats;
			for (size_t yy = 0; yy < m_nb_years; yy++)
			{
				for (size_t v = 0; v < NB_ANNUAL_STATS; v++)
					stats[v] += output[y + yy][v];
			}

			for (size_t v = 0; v < NB_ANNUAL_STATS; v++)
				m_output[y][v] = stats[v][MEAN];
		}


		return msg;
	}


	//MAT : mean annual air temperature
	double CCCModel::MAT(const CWeatherYear& weather)
	{
		return weather.GetStat(H_TNTX)[MEAN];
	}

	//MCMT : mean coldest month temperature
	double CCCModel::MCMT(const CWeatherYear& weather)
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

	//EMT : extreme minimum temperature
	double CCCModel::EMT(const CWeatherYear& weather)
	{
		return weather.GetStat(H_TMIN)[LOWEST];
	}

	//MWMT : mean warmest month temperature
	double CCCModel::MWMT(const CWeatherYear& weather)
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

	//NFFD: Total number of frost free day
	double CCCModel::NFFD(const CWeatherYear& weather)
	{
		size_t NFFD = 0;
		for (size_t d = 0; d < weather.GetNbDays(); d++)
		{
			double T = weather.GetDay(d)[H_TMIN][LOWEST];
			if (T > 0)
				NFFD++;
		}

		return NFFD;
	}


	
	//CHDD0 : chilling degree days under 0 (from August first)
	double CCCModel::CHDD(const CWeatherYear& weather, double threshold)
	{
		int year = weather.GetTRef().GetYear();
		CTPeriod pp(CTRef(year, AUGUST, DAY_01 ), CTRef(year, DECEMBER, DAY_31));

		double FDD = 0;
		for (CTRef TRef = pp.Begin(); TRef <= pp.End(); TRef++)
		{
			double Tair = weather.GetDay(TRef)[H_TNTX][MEAN];
			if (Tair < threshold)
				FDD += -(Tair- threshold);
		}


		return FDD;
	}


	
	double CCCModel::CDD5(const CWeatherYear& weather, double threshold)
	{
		CDegreeDays model(CDegreeDays::DAILY_AVERAGE, threshold);

		CModelStatVector output;
		model.Execute(weather, output);

		//CTTransformation TT(output.GetTPeriod(), CTM::ANNUAL);
		//CTStatMatrix stats(output, TT);

		CTPeriod pp= output.GetTPeriod();

		double CDD = 0;
		for (CTRef TRef = pp.Begin(); TRef <= pp.End(); TRef++)
			CDD += output[TRef][CDegreeDays::S_DD];


		return CDD;// stats[CDegreeDays::S_DD][SUM];
	}


	
	double CCCModel::PPT5(const CWeatherYear& weather)
	{
		double sumP = 0;
		for (size_t m = MAY; m <= SEPTEMBER; m++)
		{
			sumP += weather[m].GetStat(H_PRCP)[SUM];
			
		}

		return sumP;
	}
}