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
		VERSION = "1.0.0 (2022)";

		m_nb_years = 1;//Annual;
	}

	CCCModel::~CCCModel()
	{}

	
	//a.Température annuelle moyenne(MAT : mean annual air temperature)
	//b.Température moyenne du mois le plus froid(MCMT : mean coldest month temperature)
	//c.Température moyenne minimal(EMT : extreme minimum temperature)
	//d.Température moyenne du mois le plus chaud(MWMT : mean warmest month temperature)
	//e.Jour julien moyen du premier gel à l’automne(eFFP : end of frost free period)
	//f.Nombre moyen de jours sans gel(NFFD : number of frost free days)
	//g.Jours juliens de froid(< 0°C) moyen(O_CHDD_0 : chilling degree days) à partir du 1 août


	enum TAnnualStat { O_MAT, O_MCMT, O_EMT, O_MWMT, O_EFFP, O_NFFD, O_CHDD_0, NB_ANNUAL_STATS };
	//	typedef CModelStatVectorTemplate<NB_ANNUAL_STATS> CAnnualStatVector;

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
			output[y][O_EFFP] = FFp.End().GetJDay() + 1;//Base 1
			output[y][O_NFFD] = FFp.GetNbDay();
			output[y][O_CHDD_0] = CHDD(m_weather[y]);
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
				m_output[y][v] = stats[v];
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


	
	//CHDD_0 : chilling degree days under 0 (from August first)
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



}