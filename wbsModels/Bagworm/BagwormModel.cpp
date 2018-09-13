//************************************************************************************
// 27/11/2017	Rémi Saint-Amant    Convert to BioSIM 11
// 05/04/2012   Rémi Saint-Amant    Creation 
//************************************************************************************

#include "BagwormModel.h"
#include "ModelBase\EntryPoint.h"

using namespace WBSF::HOURLY_DATA;

namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CBagwormModel::CreateObject);



	CBagwormModel::CBagwormModel()
	{
		// initialise your variables here (optional)
		NB_INPUT_PARAMETER = 1;
		VERSION = "1.1.0 (2017)";

		m_nbBugs = 400;
	}

	CBagwormModel::~CBagwormModel()
	{
	}

	//This method is called to compute solution
	ERMsg CBagwormModel::OnExecuteDaily()
	{
		ERMsg msg;

		//create specific object statistic vector
		CBagwormOutputVector stat;

		//Compute statistics
		ExecuteDaily(stat);

		//create output variable vector
		CDailyOutputVector output;

		//transform object statistic to output variables
		ComputeDailylOutput(stat, output);

		//Set output to the framework
		SetOutput(output);

		return msg;
	}

	//This method is called to compute solution
	ERMsg CBagwormModel::OnExecuteAnnual()
	{
		ERMsg msg;

		//create specific object statistic vector
		CBagwormOutputVector stat;

		//Compute statistics
		ExecuteDaily(stat);

		//create output variable vector
		CAnnualOutputVector output;

		//transform object statistic to output variables
		ComputeAnnualOutput(stat, output);

		//Set output to the framework
		SetOutput(output);

		return msg;
	}

	CTRef CBagwormModel::GetLowestDate(int year)
	{
		CTRef dayWhenMinimum;
		double Tmin = 999;
		//from mid-july to mid july
		CTRef firstDate(year, JULY, DAY_15);
		CTRef lastDate(year + 1, JULY, DAY_15);
		for (CTRef d = firstDate; d < lastDate; d++)
		{
			const CWeatherDay& day = m_weather.GetDay(d);
			if (day[H_TMIN][MEAN] < Tmin)
			{
				dayWhenMinimum = d;
				Tmin = day[H_TMIN][MEAN];
			}
		}

		return dayWhenMinimum;
	}

	void CBagwormModel::ExecuteDaily(CBagwormOutputVector& stat)
	{
		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//init
		CTPeriod p = m_weather.GetEntireTPeriod(CTM::DAILY);
		stat.Init(p);

		CStand stand(this);
		CHostPtr pHost( new CHost(&stand) );

		//host.Initialise(m_nbBugs, m_weather.GetTRef(), 0, EGG, false);
		pHost->Initialize<CBagworm>(CInitialPopulation(p.Begin(), 0, m_nbBugs, 100, EGG, RANDOM_SEX, false, 0));

		//add tree to stand			
		stand.m_host.push_front(pHost);


		for (size_t y = 0; y < m_weather.size(); y++)
		{
			CTPeriod period = m_weather[y].GetEntireTPeriod();
			for (CTRef d = period.Begin(); d <= period.End(); d++)
			{
				stand.Live(m_weather.GetDay(d));
				stand.GetStat(d, stat[d]);
				stand.AdjustPopulation();
			}

			stand.HappyNewYear();
		}
	}



	void CBagwormModel::ComputeDailylOutput(CBagwormOutputVector& stat, CDailyOutputVector& output)
	{
		output.Init(stat.size(), stat.GetFirstTRef());

		for (CTRef d = output.GetFirstTRef(); d <= stat.GetLastTRef(); d++)
		{
			output[d][O_EGG] = stat[d][STAT_EGG];
			output[d][O_LARVAL] = stat[d][STAT_LARVAL];
			output[d][O_ADULT] = stat[d][STAT_ADULT];
			output[d][O_DEAD_ADULT] = stat[d][STAT_DEAD_ADULT];
			output[d][O_BROOD] = stat[d][STAT_NB_BROOD];
			output[d][O_CLUSTER_WEIGHT] = stat[d][STAT_NB_CLUSTER] > 0 ? stat[d][STAT_CLUSTER_WEIGHT] / stat[d][STAT_NB_CLUSTER] : 0;
			output[d][O_FROZEN_EGG] = stat[d][STAT_DEAD_FROZEN];
			output[d][O_FROZEN_OTHERS] = stat[d][STAT_DEAD_OTHERS];
		}

	}

	void CBagwormModel::ComputeAnnualOutput(CBagwormOutputVector& stat, CAnnualOutputVector& output)
	{
		CTPeriod p = stat.GetTPeriod();
		ASSERT(p.GetNbYears() >= 2);
		output.Init(p.GetNbYears() - 1, CTRef(p.GetFirstAnnualTRef(1).GetYear()));

		for (size_t y = 1; y < p.GetNbYears(); y++)
		{
			double nbEggs = stat[p.GetFirstAnnualTRef(y)][STAT_EGG];
			double nbDeadAdults = stat[p.GetLastAnnualTRef(y)][STAT_DEAD_ADULT];

			if (nbEggs > 0 && nbDeadAdults >= 0)
				output[y - 1][O_SURVIVAL] = nbDeadAdults / nbEggs * 100;
			else 
				output[y - 1][O_SURVIVAL] = 0;


			double nbDeadFrozen = stat[p.GetLastAnnualTRef(y)][STAT_DEAD_FROZEN];

			if (nbEggs > 0 && nbDeadFrozen >= 0)
				output[y - 1][O_EGG_SURVIVAL] = (nbEggs - nbDeadFrozen) / nbEggs * 100;
			else 
				output[y - 1][O_EGG_SURVIVAL] = 0;
		}

	}

	//this method is called to load your parameters in your variables
	ERMsg CBagwormModel::ProcessParameter(const CParameterVector& parameters)
	{
		ERMsg msg;

		//transfer your parameters here
		int c = 0;
		m_nbBugs = parameters[c++].GetInt();

		return msg;
	}

}