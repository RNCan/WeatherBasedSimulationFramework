//***********************************************************
// 26/01/2023	1.0.3	Rémi Saint-Amant   Calibrate adult emergence with many stages data
// 26/01/2021	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "ALeucopodaModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include "Basic\Utilstd.h"
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/SimulatedAnnealingVector.h"


using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace WBSF::TZZ;


//static const bool BEGIN_JULY = true;
//static const size_t FIRST_Y = BEGIN_JULY ? 1 : 0;

namespace WBSF
{

	static const size_t NB_GENERATIONS_MAX = 6;

	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
	//enum { O_CDD, O_GENERATION, O_DIAPAUSED, O_EGG, O_LARVA, O_PUPA, O_ADULT, O_DEAD_ADULT, O_BROOD, NB_OUTPUTS };
	enum { O_EGG, O_LARVA, O_PREPUPA, O_PUPA, O_ADULT, O_DEAD_ADULT, O_BROOD, O_DEAD_ATTRITION, NB_OUTPUT_ONE_G, O_IN_DIAPAUSE = (NB_OUTPUT_ONE_G * NB_GENERATIONS_MAX - O_PUPA), O_D_DAY_LENGTH, NB_DAILY_OUTPUTS };


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CAprocerosLeucopodaModel::CreateObject);

	CAprocerosLeucopodaModel::CAprocerosLeucopodaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.3 (2023)";

		
		m_bCumul = false;
		m_bApplyAttrition = true;
		m_generationSurvival = 0.4;
//		m_stage = 0;
	//	m_T = 0;

		//m_EWD.fill(0);
		//m_EAS.fill(0);
		//Set parameters to equation
		//ASSERT(stand.m_equations.m_EWD.size() == m_EWD.size());
		for (size_t p = 0; p < m_EWD.size(); p++)
			m_EWD[p] = CAprocerosLeucopodaEquations::EWD[p];

		//ASSERT(stand.m_equations.m_EAS.size() == m_EAS.size());
		for (size_t p = 0; p < m_EAS.size(); p++)
			m_EAS[p] = CAprocerosLeucopodaEquations::EAS[p];


		//m_P.fill(0);
		/*m_P[Τᴴ²] = 19.1;
		m_P[delta] = 45;
		m_P[μ1] = 239.6;
		m_P[ѕ1] = 41.4;
		m_P[μ2] = 876.0;
		m_P[ѕ2] = 55.6;
		m_P[μ3] = 625.2;
		m_P[ѕ3] = 38.3;
			*/
	}

	CAprocerosLeucopodaModel::~CAprocerosLeucopodaModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CAprocerosLeucopodaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0; 

		//		m_stage = parameters[c++].GetInt() + 1;
			//	m_T = parameters[c++].GetInt()+1;

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

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
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

		//compute 30 days avg
		//stand.ComputeTavg30(year, weather);


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
			//if (output.IsInside(d))
				//stand.GetStat(d, output[d]);

			size_t nbGenerations = stand.GetFirstHost()->GetNbGeneration();
			if (nbGenerations > output.size())
				output.push_back(CModelStatVector(p, NB_STATS, 0));

			for (size_t g = 0; g < nbGenerations; g++)
				stand.GetStat(d, output[g][d], g);



			stand.AdjustPopulation();
			HxGridTestConnection();
		}




		//if (m_bCumul)
		//{
		//	for (size_t g = 0; g < output.size(); g++)
		//	{
		//		//cumulative result
		//		for (size_t s = S_EGG; s < S_ADULT; s++)
		//		{
		//			CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		//			CStatistic stat = output[g].GetStat(s, p);
		//			if (stat.IsInit() && stat[SUM] > 0)
		//			{
		//				output[g][0][s] = output[g][0][s] * 100 / stat[SUM];//when first day is not 0
		//				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
		//				{
		//					output[g][d][s] = output[g][d - 1][s] + output[g][d][s] * 100 / stat[SUM];
		//					_ASSERTE(!_isnan(output[g][d][s]));
		//				}
		//			}
		//		}
		//	}
		//}
	}

	

	//void CAprocerosLeucopodaModel::GetCDD(int year, const CWeatherYears& weather, CModelStatVector& CDD)
	//{
	//	CDegreeDays DDmodel(DD_METHOD, m_P[Τᴴ¹], m_P[Τᴴ²]);
	//	CModelStatVector DD;
	//	DDmodel.Execute(weather[year], DD);



	//	CDD.Init(DD.GetTPeriod(), 1, 0.0);

	//	//for (CTRef TRef = DD.GetFirstTRef(); TRef <= DD.GetLastTRef(); TRef++)
	//	CDD[0][0] = DD[0][CDegreeDays::S_DD];
	//	for (size_t i = 1; i < DD.size(); i++)
	//		CDD[i][0] = CDD[i - 1][0] + DD[i][CDegreeDays::S_DD];
	//}

	void CAprocerosLeucopodaModel::GetCDD(const CWeatherYears& weather, CModelStatVector& CDD)
	{
		//CTZZStand* pStand = GetStand();
		CDegreeDays DDmodel(DD_METHOD, m_EAS[Τᴴ¹], m_EAS[Τᴴ²]);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);


		CDD.Init(DD.GetTPeriod(), 1, -999);

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod();
			//p.Begin() = p.Begin() + int(m_P[delta]);
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

	//ASSERT(data.size() == NB_INPUTS);
	////SYC	site	Year	collection	col_date	emerge_date	daily_count	species	n_days P Time
	//if (stoi(data[I_VARIABLE]) == m_stage && stoi(data[I_T]) == m_T)
	//{
	//	CSAResult obs;
	//	obs.m_obs.resize(NB_INPUTS);
	//	for (size_t i = 0; i < NB_INPUTS; i++)
	//		obs.m_obs[i] = stod(data[i]);
	//	//CSAResult obs;

	//	//CStatistic egg_creation_date;

	//	//obs.m_ref.FromFormatedString(data[5]);
	//	//obs.m_obs.resize(NB_INPUTS);
	//	//obs.m_obs[I_N] = stod(data[6]);
	//	////obs.m_obs[I_S] = ;
	//	////obs.m_obs[I_P] = stod(data[10]);
	//	////obs.m_obs[I_CDD] = stod(data[11]);
	//	////obs.m_obs[I_P] = stod(data[12]);


	//	//if (data[7] == "LA" && data[8] == "1")
	//	//	obs.m_obs[I_S] = S_LA_G1;
	//	//else if (data[7] == "LA" && data[8] == "2")
	//	//	obs.m_obs[I_S] = S_LA_G2;
	//	//else if (data[7] == "LP")
	//	//	obs.m_obs[I_S] = S_LP;
	//	//else if (data[7] == "LN")
	//	//	obs.m_obs[I_S] = S_LN;

	//	m_SAResult.push_back(obs);
	//}
//	}



	//double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	//{
	//	double x = -999;

	//	if (obs > -999)
	//	{
	//		//if (obs > 0.01 && obs < 99.99)
	//		if (obs >= 100)
	//			obs = 99.99;//to avoid some problem of truncation

	//		long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
	//		if (index >= 1)
	//		{
	//			double obsX1 = output.GetFirstTRef().GetJDay() + index;
	//			double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

	//			double obsY1 = output[index][s];
	//			double obsY2 = output[index + 1][s];
	//			if (obsY2 != obsY1)
	//			{
	//				double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
	//				double obsX = obsX1 + (obs - obsY1)*slope;
	//				ASSERT(!_isnan(obsX) && _finite(obsX));

	//				x = obsX;
	//			}
	//		}
	//	}

	//	return x;
	//}

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
		
		//CTZZStand* pStand = GetStand();
		//for (size_t p = 0; p < m_EAS.size(); p++)
			//pStand->m_equations.m_EAS[p] = m_EAS[p];

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
				//m_data_weather.CreateYears(pp);

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

		//boost::math::lognormal_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);



		//boost::math::weibull_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::beta_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::exponential_distribution<double> emerge_dist(m_P[ѕ]);
		//boost::math::rayleigh_distribution<double> emerge_dist(m_P[ѕ]);


		//CModelStatVector CDD; 
		//GetCDD(m_weather, CDD);

		//double n = 0;

		//for (size_t i = 0; i < m_SAResult.size(); i++)
		//	n += m_SAResult[i].m_obs[I_N];


		//CModelStatVector CDD;
		//GetCDD(m_weather, CDD);
		////GetPobs(P);

		//
		//boost::math::logistic_distribution<double> emerge_dist(m_EAS[μ], m_EAS[ѕ]);
		//

		m_bCumul = true;
		ExecuteDaily(m_data_weather);

		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			size_t s = m_SAResult[i].m_obs[I_STAGE];
			
			
			//double GDD = CDD[m_SAResult[i].m_ref][CDegreeDays::S_DD];
			double obs = m_SAResult[i].m_obs[I_CUMUL];
			double sim = m_output[m_SAResult[i].m_ref][GetStageIndex(m_SAResult[i].m_obs)];//Larve Generation 1
			
			stat.Add(obs, sim);

		}//for all results

		return true;


		//bool bZero = false;
		//CStatistic x;
		//for (size_t i = 0; i < m_SAResult.size(); i++)
		//{
		//	//ASSERT(m_SAResult[i].m_obs[I_VARIABLE] == m_stage);
		//	//ASSERT(m_SAResult[i].m_obs[I_T] == m_T);

		//	size_t n = size_t(ceil(m_P[i]));
		//	bZero |= n == 0;

		//	for (size_t b = 0; b < n; b++)
		//		x += m_SAResult[i].m_obs[I_TIME];

		//}//for all results

		////if (!bZero  && int(x[NB_VALUE]) == int(m_SAResult[0].m_obs[I_N]) )
		//{
		//	double obs = m_SAResult[0].m_obs[I_MEAN_TIME];
		//	double sim = x[MEAN];
		//	stat.Add(obs, sim);

		//	obs = m_SAResult[0].m_obs[I_TIME_SD] * 10;
		//	sim = x[STD_DEV] * 10;
		//	stat.Add(obs, sim);

		//	obs = size_t(m_SAResult[0].m_obs[I_N]);
		//	sim = size_t(x[NB_VALUE]);
		//	stat.Add(obs, sim);



		//	CStatistic stat_ws;
		//	CStatistic stat_n;


		//	//compute sigma

		//	double mean = x[MEAN];
		//	double sd = x[STD_DEV];
		//	double n = x[NB_VALUE];
		//	if (n > 0)
		//	{
		//		double cv = sd / mean;
		//		double sigma = cv_2_sigma(cv, n);

		//		stat_ws += n * sigma;
		//		stat_n += n;
		//	}

		//	static const double SIGMA_OBS[4] = { 0.102, 0.157, 0.203, 0.118 };


		//	obs = SIGMA_OBS[m_stage];
		//	sim = stat_ws[SUM] / stat_n[SUM];

		//}

	}



	//void CAprocerosLeucopodaModel::FinalizeStat(CStatisticXY& stat)
	//{
	//	
	//}
}



