//***********************************************************
// 06/05/2026	1.1.0	Rémi Saint-Amant	Change in attrition
//											Clean up
// 26/01/2023	1.0.3	Rémi Saint-Amant	Calibrate adult emergence with many stages data
// 26/01/2021	1.0.0	Rémi Saint-Amant	Creation
//***********************************************************
#include "ALeucopodaModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include "Basic/Utilstd.h"
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/SimulatedAnnealingVector.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace WBSF::TZZ;


namespace WBSF
{

	static const size_t NB_GENERATIONS_MAX = 6;

	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
	enum { O_EGG, O_LARVA, O_PREPUPA, O_PUPA, O_ADULT, O_DEAD_ADULT, O_BROOD, O_DEAD_ATTRITION, NB_OUTPUT_ONE_G, O_IN_DIAPAUSE = (NB_OUTPUT_ONE_G * NB_GENERATIONS_MAX - O_PUPA), O_D_DAY_LENGTH, NB_DAILY_OUTPUTS };


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CAprocerosLeucopodaModel::CreateObject);

	CAprocerosLeucopodaModel::CAprocerosLeucopodaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.1.0 (2026)";

		
		m_bCumul = false;
		m_bApplyAttrition = true;
		m_generationSurvival = 0.4;
		//Set parameters to equation
		for (size_t p = 0; p < m_EWD.size(); p++)
			m_EWD[p] = CAprocerosLeucopodaEquations::EWD[p];

		for (size_t p = 0; p < m_EAS.size(); p++)
			m_EAS[p] = CAprocerosLeucopodaEquations::EAS[p];
	}

	CAprocerosLeucopodaModel::~CAprocerosLeucopodaModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CAprocerosLeucopodaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0; 

		m_bCumul = parameters[c++].GetBool();
		m_bApplyAttrition = parameters[c++].GetBool();
		m_generationSurvival = parameters[c++].GetReal();


		if (parameters.size() == m_EWD.size() + m_EAS.size()+3)
		{
			//entering winter diapause  parameters
			for (size_t p = 0; p < m_EWD.size(); p++)
			{
				m_EWD[p] = parameters[c++].GetFloat();
			}

			//Emerging Adult from Soil (spring) parameters
			for (size_t p = 0; p < m_EAS.size(); p++)
			{
				m_EAS[p] = parameters[c++].GetFloat();
			}
		}
		

		return msg;
	}





	ERMsg CAprocerosLeucopodaModel::OnExecuteDaily()
	{
		return ExecuteDaily(m_weather);
	}

	ERMsg CAprocerosLeucopodaModel::ExecuteDaily(CWeatherStation& weather)
	{
		ERMsg msg;


		if (!weather.IsHourly())
			weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		m_output.Init(weather.GetEntireTPeriod(CTM(CTM::DAILY)), NB_DAILY_OUTPUTS, 0);

		for (size_t y = 0; y < weather.size(); y++)
		{
			int year = weather[y].GetTRef().GetYear();
			//one output by generation
			vector<CModelStatVector> outputs;
			ExecuteDaily(year, weather, outputs);


			//merge generations vector into one output vector (max of 5 generations)
			size_t maxG = min(NB_GENERATIONS_MAX, outputs.size());

			CTPeriod p = weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CStatistic diapause;
				for (size_t g = 0, ss = 0; g < maxG; g++)
				{
					size_t s_i = (g==0)?PUPA:EGG;
					for (size_t s = s_i; s < NB_OUTPUT_ONE_G; s++, ss++)
					{
						m_output[TRef][ss] = outputs[g][TRef][s];
						diapause += outputs[g][TRef][S_DIAPAUSE];
					}
				}

				
				m_output[TRef][O_IN_DIAPAUSE] = diapause[SUM];
				m_output[TRef][O_D_DAY_LENGTH] = weather.GetDayLength(TRef) / 3600.;
			}

			if (m_bCumul)
			{
				for (size_t g = 0, ss = 0; g < maxG; g++)
				{
					size_t s_i = (g == 0) ? PUPA : EGG;
					for (size_t s = s_i; s < NB_OUTPUT_ONE_G; s++, ss++)
					{
						if (ss>0 && (s != O_DEAD_ADULT || s != O_BROOD || s != O_DEAD_ATTRITION))
						{
							CStatistic stat_g1 = m_output.GetStat(ss, p);
							if (stat_g1.IsInit() && stat_g1[SUM] > 0)
							{
								double cumul_output = m_output[0][ss] * 100 / stat_g1[SUM];//when first day is not 0
								for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
								{
									m_output[d][ss] = m_output[d - 1][ss] + m_output[d][ss] * 100 / stat_g1[SUM];
									_ASSERTE(!_isnan(m_output[d][ss]));
								}
							}
						}
					}
				}
			}
		}

		return msg;
	}

	void CAprocerosLeucopodaModel::ExecuteDaily(int year, const CWeatherYears& weather, vector<CModelStatVector>& output)
	{
		//Create stand
		CTZZStand stand(this);
		stand.m_bApplyAttrition = m_bApplyAttrition;
		stand.m_generationSurvival = m_generationSurvival;

		
		//Set parameters to equation

		ASSERT(stand.m_equations.m_EWD.size() == m_EWD.size());
		for (size_t p = 0; p < m_EWD.size(); p++)
			stand.m_equations.m_EWD[p] = m_EWD[p];

		ASSERT(stand.m_equations.m_EAS.size() == m_EAS.size());
		for (size_t p = 0; p < m_EAS.size(); p++)
			stand.m_equations.m_EAS[p] = m_EAS[p];

		stand.init(year, weather);

		//Create host
		CTZZHostPtr pHost(new CTZZHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CAprocerosLeucopoda>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, TZZ::PUPA, WBSF::FEMALE, true, 0));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		


		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));

			size_t nbGenerations = stand.GetFirstHost()->GetNbGeneration();
			if (nbGenerations > output.size())
				output.push_back(CModelStatVector(p, NB_STATS, 0));

			for (size_t g = 0; g < nbGenerations; g++)
				stand.GetStat(d, output[g][d], g);



			stand.AdjustPopulation();
			HxGridTestConnection();
		}

	}

	void CAprocerosLeucopodaModel::GetCDD(const CWeatherYears& weather, CModelStatVector& CDD)
	{
		CDegreeDays DDmodel(DD_METHOD, m_EAS[Τᴴ¹], m_EAS[Τᴴ²]);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);


		CDD.Init(DD.GetTPeriod(), 1, -999);

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod();
			CDD[p.Begin()][0] = DD[p.Begin()][CDegreeDays::S_DD];

			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				CDD[TRef][0] = CDD[TRef - 1][0] + DD[TRef][CDegreeDays::S_DD];
		}

	}

	size_t GetStage(std::string name)
	{
		static const array<char*, NB_STAGES> STAGE_NAME = { {"EGG", "LARVA", "PREPUPA", "PUPA", "ADULT"} };
		
		auto it = find(STAGE_NAME.begin(), STAGE_NAME.end(), MakeUpper(name));
		return distance(STAGE_NAME.begin(), it);
	}



	enum TInput { I_KEYID, I_DATE, I_STAGE, I_GENERATION, I_N, I_CUMUL, NB_INPUTS };
	void CAprocerosLeucopodaModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;
		obs.m_ref.FromFormatedString(data[I_DATE]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t i = 2; i < NB_INPUTS; i++)
		{
			if (i == I_STAGE)
				obs.m_obs[i] = GetStage(data[i]);
			else
				obs.m_obs[i] = stod(data[i]);
		}

		m_years.insert(obs.m_ref.GetYear());
		m_SAResult.push_back(obs);
	}

	bool CAprocerosLeucopodaModel::IsParamValid()const
	{
		bool bValid = true;
		if (m_EAS[Τᴴ¹] > m_EAS[Τᴴ²])
			bValid = false;


		return bValid;
	}





	size_t GetStageIndex(const std::vector<double>& obs)
	{
		return obs[I_GENERATION] * NB_OUTPUT_ONE_G + obs[I_STAGE] - PUPA;
	}


	bool CAprocerosLeucopodaModel::GetFValueDaily(CStatisticXY& stat)
	{
		
		ASSERT(!m_SAResult.empty() );

		if (!IsParamValid())
			return false;

		if (m_data_weather.GetNbYears() == 0)
		{
			CTPeriod pp(CTRef(*m_years.begin(), JANUARY, DAY_01), CTRef(*m_years.rbegin(), DECEMBER, DAY_31));
			pp = pp.Intersect(m_weather.GetEntireTPeriod(CTM::DAILY));
			if (pp.IsInit())
			{
				((CLocation&)m_data_weather) = m_weather;
				m_data_weather.SetHourly(m_weather.IsHourly());

				for (int year = pp.GetFirstYear(); year <= pp.GetLastYear(); year++)
				{
					m_data_weather[year] = m_weather[year];
				}
			}
			else
			{
				//remove these obs, no input weather
				ASSERT(false);
				m_SAResult.clear();
				return true;
			}
		}


		m_bCumul = true;
		ExecuteDaily(m_data_weather);

		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			size_t s = m_SAResult[i].m_obs[I_STAGE];

			double obs = m_SAResult[i].m_obs[I_CUMUL];
			double sim = m_output[m_SAResult[i].m_ref][GetStageIndex(m_SAResult[i].m_obs)];//Larva Generation 1
			
			stat.Add(obs, sim);

		}//for all results

		return true;



	}

}



