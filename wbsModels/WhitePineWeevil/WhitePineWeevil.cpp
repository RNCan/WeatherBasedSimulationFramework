//**************************************************************************************************************
// 04/05/2017	2.2.1	Rémi Saint-Amant    New hourly generation
// 20/09/2016	2.2.0	Rémi Saint-Amant    Change Tair and Trng by Tmin and Tmax
// 22/01/2016	2.0.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 04/03/2015			Rémi Saint-Amant	Update to BioSIM11
// 15/05/2013			Rémi Saint-Amant	Réestimation of parameters with field data
// 08/05/2012			Rémi Saint-Amant	Create from Lucie Voyer data
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "WhitePineWeevil.h"

using namespace std;

static const bool PARAMETRIZE = false;
namespace WBSF
{

	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CWhitePineWeevilModel::CreateObject);


	//Defining a simple continuing ratio model
	enum TInstar{ UNBROODED=-1, EGG, L1, L2, L3, L4, PUPA, ADULT, NB_STAGES };
	//extern const char HEADER[] = "Unbrooded,Egg,L1,L2,L3,L4,Pupa";
	typedef CContinuingRatio<NB_PARAMS, UNBROODED, ADULT> CWhitePineWeevilCR;


	//NbVal=    70	Bias= 0.14066	MAE= 3.21604	RMSE= 4.78397	CD= 0.98476	R²= 0.98489
	//	Egga                	= 163.522  
	//	Eggb                	= 2.82250
	//	L1a                 	= 234.141
	//	L1b                 	= 2.63746
	//	L2a                 	= 258.929  
	//	L2b                 	= 1.03911   
	//	L3a                 	= 316.171  
	//	L3b                 	= 2.60322
	//	L4a                 	= 468.543  
	//	L4b                 	= 3.66616  
	//	Pupaa               	= 733.439  
	//	Pupab               	= 2.75011  
	//	Adulta               	= 807.158  
	//	Adultb               	= 2.15406  
	

	const size_t CWhitePineWeevilModel::FISRT_HL_JDAY = 1 - 1;   //first of March zero base
	const double CWhitePineWeevilModel::THRESHOLD = 7.2;			//°C
	const double CWhitePineWeevilModel::A[NB_PARAMS] = { 163.522, 234.141, 258.929, 316.171, 468.543, 733.439, 807.158 };	//accumulated degree-days
	const double CWhitePineWeevilModel::B[NB_PARAMS] = { 2.82250, 2.63746, 1.03911, 2.60322, 3.66616, 2.75011, 2.15406 };	//variability


	CWhitePineWeevilModel::CWhitePineWeevilModel()
	{
		NB_INPUT_PARAMETER = PARAMETRIZE?17:1;
		VERSION = "2.2.1 (2017)";

		m_bCumulative = false;
		m_startJday = FISRT_HL_JDAY;
		m_threshold = THRESHOLD;
		for (size_t i = 0; i < NB_PARAMS; i++)
		{
			m_a[i] = A[i];
			m_b[i] = B[i];
		}
		m_lastParam = ADULT;
	}

	CWhitePineWeevilModel::~CWhitePineWeevilModel()
	{}


	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CWhitePineWeevilModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		m_bCumulative = parameters[c++].GetBool();

		if (parameters.size() == 17)
		{
			m_threshold = parameters[c++].GetReal();
			for (size_t i = 0; i < NB_PARAMS; i++)
			{
				m_a[i] = parameters[c++].GetReal();
				m_b[i] = parameters[c++].GetReal();
			}

			m_lastParam = parameters[c++].GetInt();
		}

		return msg;
	}

	ERMsg CWhitePineWeevilModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Excute model on a daily basis
		ExecuteDaily(m_output);

		return msg;
	}

	void CWhitePineWeevilModel::ExecuteDaily(CModelStatVector& output)
	{
		//execute daily using continuing ratio structure
		CWhitePineWeevilCR CR;
		CR.m_startJday = m_startJday;
		CR.m_lowerThreshold = m_threshold;
		CR.m_method = CDegreeDays::DAILY_AVERAGE;
		CR.m_bPercent = true;
		CR.m_bCumul = m_bCumulative;
		CR.m_bMultipleVariance = true;
		CR.m_bAdjustFinalProportion = true;

		for (size_t i = 0; i < NB_PARAMS; i++)
		{
			CR.m_a[i] = m_a[i];
			CR.m_b[i] = m_b[i];
		}

		CR.Execute(m_weather, output);
	}




	//*****************************************************************************************************************
	//Next 4 methods are for Simulated Annealing

	void CWhitePineWeevilModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		enum TInputAllStage{ E_ID, E_YEAR, E_MONTH, E_DAY, E_DD, E_EGG, E_EGG_CUMUL, E_L1, E_CUMUL_L1, E_L2, E_CUMUL_L2, E_L3, E_CUMUL_L3, E_L4, E_CUMUL_L4, E_PUPA, E_CUMUL_PUPA, E_ADULT, E_CUMUL_ADULT, NB_INPUTS_STAGE };
		enum TInputDD{ P_YEAR, P_MONTH, P_DAY, P_DD, NB_INPUTS_DD };

		

		if (header.size() == NB_INPUTS_STAGE)
		{
			CTRef TRef(ToInt(data[E_YEAR]), ToSizeT(data[E_MONTH]) - 1, ToSizeT(data[E_DAY]) - 1);
			std::vector<double> obs(NB_STAGES,-999);

			for (size_t i = 0; i < 7 && E_EGG_CUMUL + 2 * i<data.size(); i++)
				obs[i] = !data[E_EGG_CUMUL + 2 * i].empty()?ToDouble(data[E_EGG_CUMUL + 2 * i]):-999;

			m_SAResult.push_back(CSAResult(TRef, obs));
		}
		/*else if (header.size() == NB_INPUTS_DD)
		{
		//CTRef TRef(ToInt(data[P_YEAR]), ToSizeT(data[P_MONTH]) - 1, ToSizeT(data[P_DAY]) - 1);
			std::vector<double> obs(2);

			obs[PUP_NB_DAYS] = ToDouble(data[P_NB_DAYS]);
			obs[PUP_N] = ToDouble(data[P_N]);

			m_SAResult.push_back(CSAResult(TRef, obs));
		}*/
	}


	//bool CWhitePineWeevilModel::ComputePupation(const CModelStatVector& statSim, vector<pair<size_t, double>>& obs)
	//{
	//	//compute simulated pupation at the same date as observations
	//	CStatistic obsStat;
	//	for (size_t i = 0; i < m_SAResult.size(); i++)
	//	{
	//		double DD = statSim[m_SAResult[i].m_ref][CWhitePineWeevilCR::O_DD];
	//		DD += (20 - m_threshold)*m_SAResult[i].m_obs[PUP_NB_DAYS];

	//		int year = m_SAResult[i].m_ref.GetYear();
	//		int index = statSim.GetFirstIndex(CWhitePineWeevilCR::O_DD, DD, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
	//		if (index >= 0)
	//		{
	//			obsStat += m_SAResult[i].m_obs[1];
	//			obs.push_back(make_pair(index, m_SAResult[i].m_obs[1]));
	//		}
	//	}

	//	//not enough observations
	//	if (obs.size() < m_SAResult.size() - 5)
	//		return false;


	//	sort(obs.begin(), obs.end());

	//	//regroup all duplication index
	//	vector<pair<size_t, double>>::iterator itLast = obs.begin();
	//	for (vector<pair<size_t, double>>::iterator it = obs.begin() + 1; it < obs.end(); it++)
	//	{
	//		if (itLast->first == it->first)
	//		{
	//			itLast->second += it->second;
	//			it = obs.erase(it) - 1;
	//		}
	//		else
	//		{
	//			itLast = it;
	//		}
	//	}

	//	//compute cumulated
	//	double sum = obsStat[SUM];
	//	if (sum > 0)
	//	{
	//		obs[0].second = obs[0].second / sum * 100;
	//		for (size_t i = 1; i < obs.size(); i++)
	//		{
	//			obs[i].second = obs[i - 1].second + obs[i].second / sum * 100;
	//		}
	//	}

	//	return true;
	//}

	//because, in observation, eclosion and adult is cumulative and all other stages are non-cumulative,
	//we have to make a mixte between horizontal and vertical comparison
	void CWhitePineWeevilModel::ComputeCumulDiagonal(CModelStatVector& statSim, const CModelStatVector& statSim1)
	{
		statSim = statSim1;

		for (size_t p = S_EGG; p <= S_PUPA; p++)
		{
			size_t ss = CWhitePineWeevilCR::O_FIRST_STAGE + p + 1;
			double sum = statSim1.GetStat(ss)[SUM];
			if (sum > 0)
			{
				for (size_t d = 1; d < statSim1.size(); d++)
				{
					double sum2 = 0;
					for (size_t pp = S_EGG; pp <= S_PUPA; pp++)
					{
						size_t sss = CWhitePineWeevilCR::O_FIRST_STAGE + pp + 1;
						sum2 += statSim1[d][sss];
					}

					statSim[d][CWhitePineWeevilCR::O_FIRST_STAGE + 1] = 100 - statSim[d][CWhitePineWeevilCR::O_FIRST_STAGE];
					statSim[d][ss] = statSim[d - 1][ss] + (statSim1[d][ss] / (sum + sum2)) * 100;
				}
			}
		}
	}

	bool CWhitePineWeevilModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			//look to see if all pairs are ordered
			bool bValid = true;

			for (size_t p = S_L1; p <= S_ADULT&&bValid; p++)
			{
				if (m_a[p] < m_a[p - 1])
					bValid = false;
			}

			if (bValid)
			{

				//execute model with actual parameters
				CModelStatVector statSim0;
				m_bCumulative = true;
				ExecuteDaily(statSim0);

				CModelStatVector statSim;
				ComputeCumulDiagonal(statSim0, statSim);


				//now compare simuation with observation data
				//if (m_SAResult[0].m_obs.size() == 2)//if it's pupation data
				//{

				//	vector<pair<size_t, double>> obs;

				//	if (!ComputePupation(statSim, obs))
				//	{
				//		//Pupation is out of season
				//		//add dummy poor data to reject these parameters set
				//		stat.Add(999, -999);
				//		stat.Add(-2345, 546);
				//		stat.Add(22, -567);
				//		return;
				//	}


				//	for (size_t i = 0; i < obs.size(); i++)
				//	{
				//		double obsV = obs[i].second;
				//		double simV = statSim[obs[i].first][CWhitePineWeevilCR::O_FIRST_STAGE + P_L4_PUPA + 1];

				//		stat.Add(simV, obsV);
				//	}

				//}
				//else //it's developement data
				//{
				for (size_t i = 0; i<m_SAResult.size(); i++)
				{
					for (size_t p = S_EGG; p <= S_ADULT; p++)
					{
						if (m_SAResult[i].m_obs[p]>-999 &&
							statSim.IsInside(m_SAResult[i].m_ref))
						{
							double obsV = m_SAResult[i].m_obs[p];
							double simV = statSim[m_SAResult[i].m_ref][CWhitePineWeevilCR::O_FIRST_STAGE + p + 1];
							if (obsV>-999)
								stat.Add(simV, obsV);
						}
					}
				}
			//}
			}
		}

		return true;
	}
}