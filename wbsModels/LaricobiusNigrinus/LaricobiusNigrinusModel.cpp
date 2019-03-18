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
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
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
				m_D[s][p] = CLaricobiusNigrinusEquations::D[s][p];
			}
		}

		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
		{
			m_O[p] = CLaricobiusNigrinusEquations::O[p];
		}
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
		

		if (parameters.size() == 1 + 4 * NB_RDR_PARAMS + NB_OVIP_PARAMS  )
		{
			for (size_t s = 0; s < 4; s++)
			{
				for (size_t p = 0; p < NB_RDR_PARAMS; p++)
				{
					m_D[s][p] = parameters[c++].GetFloat();
				}
			}
			
			for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
			{
				m_O[p] = parameters[c++].GetFloat();
			}
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
		CLNFStand stand(this, m_O[Τᴴ]);

		//Set parameters to equation
		for (size_t s = 0; s < ADULT; s++)
		{
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
			{
				stand.m_equations.m_D[s][p] = m_D[s][p];
			}
		}
		
		//stand.m_equations.Reinit();//re-initialize to recompute rate whit new F
		

		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
		{
			stand.m_equations.m_O[p] = m_O[p];
		}
		
		

		int year = weather.GetTRef().GetYear();


		//Create host
		CLNFHostPtr pHost(new CLNFHost(&stand));
		
		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		//Zilahi-Balogh (2001)
//Oviposition began in week 9 (27 December) with the active egg laying period between weeks 10 and 32 (7 January and 13
//June, respectively) (Figure 2.5). Peak egg laying occurred week in 18 (7 March) (
		pHost->Initialize<CLaricobiusNigrinus>(CInitialPopulation(400, 100, EGG));
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
		ASSERT(data.size() == 5);

		CSAResult obs;
		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(3);
		for (size_t i = 0; i < 3; i++)
		{
			obs.m_obs[i] = stod(data[i + 2]);
			if (obs.m_obs[i]>-999)
				m_nb_days[i] += obs.m_ref.GetJDay();
		}

		m_SAResult.push_back(obs);

		m_years.insert(obs.m_ref.GetYear());

	}

	double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		double x = -999;

		if (obs > -999)
		{
			//if (obs > 0.01 && obs < 99.99)
			if (obs >= 100)
				obs = 99.99;//to avoid some problem of truncation

			long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
			if (index >= 1)
			{
				double obsX1 = output.GetFirstTRef().GetJDay() + index;
				double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

				double obsY1 = output[index][s];
				double obsY2 = output[index + 1][s];
				if (obsY2 != obsY1)
				{
					double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
					double obsX = obsX1 + (obs - obsY1)*slope;
					ASSERT(!_isnan(obsX) && _finite(obsX));

					x = obsX;
				}
			}
		}

		return x;
	}

	bool CLaricobiusNigrinusModel::IsParamEqual()const
	{
		bool equal = true;
		for (size_t s = 0; s <= LARVAE&&equal; s++)
		{
			CStatistic rL;
			for (double Э = 0.01; Э < 0.5; Э += 0.01)
			{
				double r = 1.0 - log((pow(Э, m_D[s][Ϙ]) - 1.0) / (pow(0.5, m_D[s][Ϙ]) - 1.0)) / m_D[s][к];
				if(r>=0.4 && r<=2.5 )
					rL += 1.0 / r;//reverse for comparison
			}

			CStatistic rH;
			for (double Э = 0.51; Э < 1.0; Э += 0.01)
			{
				double r = 1.0 - log((pow(Э, m_D[s][Ϙ]) - 1.0) / (pow(0.5, m_D[s][Ϙ]) - 1.0)) / m_D[s][к];
				if (r >= 0.4 && r <= 2.5)
					rH += r;
			}

			if (rL.IsInit() && rH.IsInit())
				equal = fabs(rL[SUM] - rH[SUM])<5.3; //in Régnière (2012) obtain a max of 5.3
			else
				equal = false;
		}

		return equal;
	}

	void CLaricobiusNigrinusModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!m_SAResult.empty())
		{
			if(!m_bCumul)
				m_bCumul = true;//SA always cumulative

			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();

			//low and hi relative development rate must be approximatively the same
			if( !IsParamEqual())
				return ;

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
						for (size_t j = 0; j <3; j++)
						{
							double obs_y = m_SAResult[i].m_obs[j];
							double sim_y = output[m_SAResult[i].m_ref][STAT_STAGE[j]];
							
							if (obs_y > -999)
								stat.Add(obs_y, sim_y);


							double obs_x = m_SAResult[i].m_ref.GetJDay(); 
							double sim_x = GetSimX(STAT_STAGE[j], m_SAResult[i].m_ref, obs_y, output);

							if (sim_x > -999)
							{
								obs_x = 100 * (obs_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE];
								sim_x = 100 * (sim_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE];
								stat.Add(obs_x, sim_x);
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
