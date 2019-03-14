//***********************************************************
// 07/03/2019	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LaricobiusNigrinusModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"

using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LNF;
using namespace std;

namespace WBSF
{
	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::DAILY_AVERAGE;
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::SINGLE_TRIANGLE;
	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::DOUBLE_SINE;


	enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLaricobiusNigrinusModel::CreateObject);

	CLaricobiusNigrinusModel::CLaricobiusNigrinusModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2019)";

		m_start = CTRef(YEAR_NOT_INIT, JANUARY, DAY_01);
		m_threshold = 5.6;
		m_sumDD = 540;

		m_bCumul = false;
		for (size_t s = 0; s < 4; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				m_P[s][p] = CLaricobiusNigrinusEquations::P[s][p];
			}
		}

		
		//calibrated with simulated annealing
		m_peak=51;
		m_s=17.2;
		m_maxTsoil = 4.8;
	}

	CLaricobiusNigrinusModel::~CLaricobiusNigrinusModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLaricobiusNigrinusModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		
		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 4)
		{
			m_start = CJDayRef(parameters[c++].GetInt());
			m_threshold = parameters[c++].GetFloat();
			m_sumDD = parameters[c++].GetFloat();
		}
		

		if (parameters.size() == 4 * NB_RDR_PARAMS +3 +1) 
		{
			for (size_t s = 0; s < 4; s++)
			{
				for (size_t p = 0; p < NB_RDR_PARAMS; p++)
				{
					m_P[s][p] = parameters[c++].GetFloat();
				}
			}
			
			m_peak = parameters[c++].GetFloat();
			m_s = parameters[c++].GetFloat();
			m_maxTsoil = parameters[c++].GetFloat();
		}

		return msg;
	}





	ERMsg CLaricobiusNigrinusModel::OnExecuteAnnual()
	{
		_ASSERTE(m_weather.size() > 1);

		ERMsg msg;
		CTRef today = CTRef::GetCurrentTRef();

		CTPeriod outputPeriod = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		//outputPeriod.Begin()++;//begin output at the second year
		m_output.Init(outputPeriod, NB_OUTPUTS);

		//for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();

			//CTRef begin = CTRef(year, NOVEMBER, DAY_01);
			//CTRef end = CTRef(year + 1, NOVEMBER, DAY_01);
			//CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
			//CTRef end = CTRef(year + 1, m_start.GetMonth(), m_start.GetDay());
			//CTRef begin = CTRef(year+1, JANUARY, DAY_01);
			//CTRef end = CTRef(year + 1, DECEMBER, DAY_31);
			//CTRef begin = m_weather[y].GetEntireTPeriod().Begin();
			//CTRef end = m_weather[y].GetEntireTPeriod().End();
			CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
			//CTRef begin = CTRef(year, JANUARY, DAY_01);
			CTRef end = CTRef(year, DECEMBER, DAY_31);

			

			double CDD = 0;

			CTRef day717;
			double actualCDD = -999;
			CDegreeDays DD(DD_METHOD, m_threshold);

			for (CTRef d = begin; d < end; d++)
			{
				CDD += DD.GetDD(m_weather.GetDay(d));
				if (CDD >= m_sumDD && !day717.IsInit())
					day717 = d;

				if (d.as(CTM(CTM::DAILY, CTM::OVERALL_YEARS)) == today.as(CTM(CTM::DAILY, CTM::OVERALL_YEARS)))
					actualCDD = CDD;
			}

			m_output[y][ACTUAL_CDD] = actualCDD;
			if (day717.IsInit())
			{
				m_output[y][DATE_DD717] = day717.GetRef();
				m_output[y][DIFF_DAY] = (int)day717.GetJDay() - (int)today.GetJDay();
			}
		}

		return msg;
	}

	//This method is called to compute the solution
	ERMsg CLaricobiusNigrinusModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}

	void CLaricobiusNigrinusModel::ExecuteDaily(const CWeatherYear& weather, CModelStatVector& output)
	{
		//Create stand
		CLNFStand stand(this);
		stand.m_maxTsoil = m_maxTsoil;


//		static const size_t COL[ADULT] = { 0,1,1,1,1,2,3 };
		//Set parameters to equation
		for (size_t s = 0; s < ADULT; s++)
		{
			//size_t ss = COL[s];
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				stand.m_equations.m_P[s][p] = m_P[s][p];
			}
		}

		stand.m_equations.Reinit();//reinit to recompute rate whit new F
		

		int year = weather.GetTRef().GetYear();


		//Create host
		CLNFHostPtr pHost(new CLNFHost(&stand));
		
		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;
		//Zilahi-Balogh (2001)
//Oviposition began in week 9 (27 December) with the active egg laying period between weeks 10 and 32 (7 January and 13
//June, respectively) (Figure 2.5). Peak egg laying occurred week in 18 (7 March) (
		pHost->Initialize<CLaricobiusNigrinus>(CInitialPopulation(CJDayRef(year, m_peak), m_s, 400, 100, EGG));
		//pTree->Initialize<CSpruceBudworm>(CInitialPopulation(p.Begin(), 0, 1, 100, L2o, NOT_INIT, m_bFertility, 0));

		
			
		//add host to stand			
		stand.m_host.push_front(pHost);

		//if Simulated Annealing, set 
		//if (ACTIVATE_PARAMETRIZATION)
		{
			//stand.m_rates.SetRho25(m_rho25Factor);
			//stand.m_rates.Save("D:\\Rates.csv");
		}

		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
		//for (size_t y = 0; y < m_weather.size(); y++)
		//{
//			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			stand.GetStat(d, output[d]);

			stand.AdjustPopulation();
			HxGridTestConnection();
		}

		//stand.HappyNewYear();
		//}

		if (m_bCumul)
		{
			//cumulative result
			for (size_t s = S_EGG; s < NB_STATS; s++)
			{ 
				if (s >= S_ADULT && s <= S_AVERAGE_INSTAR)
					continue;

				CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));
				CStatistic stat = output.GetStat(s,p);
				if (stat.IsInit())
				{
					for (CTRef d = p.Begin()+1; d <= p.End(); d++)
						output[d][s] = output[d - 1][s] + output[d][s] * 100 / stat[SUM];
				}
			}

		}
	}


	void CLaricobiusNigrinusModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//Location,Year,Month,Day,date,EggNotLaid,Egg,L1,L2,L3,L4,PrePupa,Larvae
		/*ASSERT(data.size() == 12);

		CSAResult obs;
		obs.m_ref = CTRef(stoi(data[1]), stoi(data[2]) - 1, stoi(data[3]) - 1);
		obs.m_obs.resize(7);
		for (size_t i = 0; i < 7; i++)
			obs.m_obs[i] = stod(data[i + 5]);

		m_SAResult.push_back(obs);

		m_years.insert(obs.m_ref.GetYear());*/

		ASSERT(data.size() == 5);

		CSAResult obs;
		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(3);
		for (size_t i = 0; i < 3; i++)
			obs.m_obs[i] = stod(data[i + 2]);

		m_SAResult.push_back(obs);

		m_years.insert(obs.m_ref.GetYear());

	}

	void CLaricobiusNigrinusModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!m_SAResult.empty())
		{
			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();


			//for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if (m_years.find(year) == m_years.end())
					//if (m_years.find(year + 1) == m_years.end())
					continue;

				//CTRef begin = m_weather[y].GetEntireTPeriod().Begin();
				//CTRef end = m_weather[y].GetEntireTPeriod().End();
				//CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
				//CTRef end = CTRef(year, DECEMBER, DAY_31);

				CModelStatVector output;
				output.Init(m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY)), NB_STATS, 0);
				ExecuteDaily(m_weather[y], output);

				static const size_t STAT_STAGE[3] = { S_EGG, S_LARVAE, S_ADULT };
			
				for (size_t i = 0; i < m_SAResult.size(); i++)
				{
					if (output.IsInside(m_SAResult[i].m_ref))
					{
						for (size_t j = 0; j < m_SAResult[i].m_obs.size(); j++)
						{
							double obs = m_SAResult[i].m_obs[j];
							double sim = output[m_SAResult[i].m_ref][STAT_STAGE[j]];
							if (obs > -999)
							{
								stat.Add(obs, sim);
							}
						}
					}
				}
			}
		}
	}

	void CLaricobiusNigrinusModel::AddAnnualResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 2);

		CSAResult obs;
		obs.m_ref.FromFormatedString(data[1]);
		m_SAResult.push_back(obs);

		m_years.insert(obs.m_ref.GetYear());

	}

	void CLaricobiusNigrinusModel::GetFValueAnnual(CStatisticXY& stat)
	{
		if (!m_SAResult.empty())
		{


			//for (size_t y = 0; y < m_weather.GetNbYears() - 1; y++)
			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if (m_years.find(year) == m_years.end())
					//if (m_years.find(year + 1) == m_years.end())
					continue;

				//CTRef begin = m_weather[y].GetEntireTPeriod().Begin();
				//CTRef end = m_weather[y].GetEntireTPeriod().End();
				//CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
				//CTRef end = CTRef(year, DECEMBER, DAY_31);



				CTRef begin = CTRef(year, m_start.GetMonth(), m_start.GetDay());
				CTRef end = CTRef(year + 1, DECEMBER, DAY_31);


				//CTRef begin = CTRef(year, NOVEMBER, DAY_01);
				//CTRef end = CTRef(year + 1, NOVEMBER, DAY_01);

				double CDD = 0;
				CDegreeDays DD(DD_METHOD, m_threshold);

				for (CTRef d = begin; d < end && CDD < m_sumDD; d++)
				{
					CDD += DD.GetDD(m_weather.GetDay(d));
					if (CDD >= m_sumDD)
					{
						for (size_t j = 0; j < m_SAResult.size(); j++)
						{
							if (m_SAResult[j].m_ref.GetYear() == d.GetYear())
							{
								stat.Add(d.GetJDay(), m_SAResult[j].m_ref.GetJDay());
							}
						}
					}
				}
			}
		}
	}

}
