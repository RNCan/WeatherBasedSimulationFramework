//**************************************************************************************************************
// 07/04/2017	2.0.1	Rémi Saint-Amant	Adjustement of pupea stage base on 1950 data
// 22/01/2016	2.0.0	Rémi Saint-Amant	Using Weather-Based Simulation Framework (WBSF)
// 04/03/2015			Rémi Saint-Amant	Update to BioSIM11
// 15/05/2013			Rémi Saint-Amant	Réestimation of parameters with field data
// 08/05/2012			Rémi Saint-Amant	Create from Lucie Voyer data
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "HemlockLooper.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CHemlockLooperModel::CreateObject);


	//Defining a simple continuing ratio model
	enum TInstar{ EGG, L1, L2, L3, L4, PUPA, ADULT, NB_STAGES };
	extern const char HEADER[] = "EGG,L1,L2,L3,L4,Pupa,Adult";
	typedef CContinuingRatio<NB_PARAMS, EGG, ADULT, HEADER> CHemlockLooperCR;


	//NbVal=   265	Bias= 0.67693	MAE= 8.24845	RMSE=12.83694	CD= 0.89424	R²= 0.89478
	//	Egga                	= 179.70150  
	//	Eggb                	=   0.63714  
	//	L1a                 	= 259.76711  
	//	L1b                 	=   2.59116  
	//	L2a                 	= 397.95620  
	//	L2b                 	=   1.56396  
	//	L3a                 	= 495.09259  
	//	L3b                 	=   1.41404  
	//	L4a                 	= 723.79758  
	//	L4b                 	=   1.53101  
	//	Pupaa               	=1075.45382  
	//	Pupab               	=   1.24377  

	const size_t CHemlockLooperModel::FISRT_HL_JDAY = 61 - 1;   //first of March zero base
	const double CHemlockLooperModel::THRESHOLD = 5.4;			//°C
	//const double CHemlockLooperModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 1075.5 };	//accumulated degree-days
	//const double CHemlockLooperModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 1.2437 };	//variability
	const double CHemlockLooperModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 945.9};	//accumulated degree-days
	const double CHemlockLooperModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 0.741 };	//variability


	CHemlockLooperModel::CHemlockLooperModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "2.0.1 (2017)";

		m_bCumulative = false;
		m_startJday = FISRT_HL_JDAY;
		m_threshold = THRESHOLD;
		for (int i = 0; i < NB_PARAMS; i++)
		{
			m_a[i] = A[i];
			m_b[i] = B[i];
		}
		m_lastParam = ADULT;
	}

	CHemlockLooperModel::~CHemlockLooperModel()
	{}


	ERMsg CHemlockLooperModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Excute model on a daily basis
		ExecuteDaily(m_output);

		return msg;
	}

	void CHemlockLooperModel::ExecuteDaily(CModelStatVector& output)
	{
		//execute daily using continuing ratio structure
		CHemlockLooperCR CR;
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

		//kill adult when temperature under -5°C
		for (CTRef TRef = output.GetFirstTRef(); TRef <= output.GetLastTRef(); TRef++)
		{
			if (m_weather[TRef][H_TMIN2][MEAN] < -5)
			{
				for (CTRef TRef2 = output.GetFirstTRef(); TRef <= output.GetLastTRef(); TRef++)
				{

				}

				break;
			}
		}

	}



	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CHemlockLooperModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		m_bCumulative = parameters[c++].GetBool();

		if (parameters.size() == 15)
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

	//*****************************************************************************************************************
	//Next 4 methods are for Simulated Annealing
	enum TPupasion{ PUP_NB_DAYS, PUP_N, NB_PUPAISON };
	void CHemlockLooperModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		enum TInputAllStage{ E_SITE, E_DATE, E_YEAR, E_MONTH, E_DAY, E_JDAY, E_EGG, E_L1, E_L2, E_L3, E_L4, E_PUPAE, E_ADULT, E_CUMUL_EGG, E_CUMUL_L1, E_CUMUL_L2, E_CUMUL_L3, E_CUMUL_L4, E_CUMUL_PUPAE, E_CUMUL_ADULTS, NB_COLUMNS };
		//enum TInputAllStage{ E_NAME, E_ID, E_YEAR, E_MONTH, E_DAY, E_JDAY, E_CUMUL_L1, E_CUMUL_L2, E_CUMUL_L3, E_CUMUL_L4, E_CUMUL_PUPA, E_CUMUL_ADULT, NB_INPUT, NB_COLUMN = NB_INPUT + 7 };
		enum TInputPupasion{ P_NAME, P_ID, P_YEAR, P_MONTH, P_DAY, P_NB_DAYS, P_N, NB_INPUT_PUPAISON };

		CTRef TRef(ToInt(data[E_YEAR]), ToSizeT(data[E_MONTH]) - 1, ToSizeT(data[E_DAY]) - 1);

		if (header.size() == NB_COLUMNS)
		{
			std::vector<double> obs(NB_COLUMNS - E_CUMUL_EGG);

			for (size_t i = 0; i < obs.size(); i++)
				obs[i] = ToDouble(data[E_CUMUL_EGG + i]);

			m_SAResult.push_back(CSAResult(TRef, obs));
		}
		else if (header.size() == NB_INPUT_PUPAISON)
		{
			std::vector<double> obs(2);

			obs[PUP_NB_DAYS] = ToDouble(data[P_NB_DAYS]);
			obs[PUP_N] = ToDouble(data[P_N]);

			m_SAResult.push_back(CSAResult(TRef, obs));
		}
	}


	bool CHemlockLooperModel::ComputePupation(const CModelStatVector& statSim, vector<pair<size_t, double>>& obs)
	{
		//compute simulated pupation at the same date as observations
		CStatistic obsStat;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			double DD = statSim[m_SAResult[i].m_ref][CHemlockLooperCR::O_DD];
			DD += (20 - m_threshold)*m_SAResult[i].m_obs[PUP_NB_DAYS];

			int year = m_SAResult[i].m_ref.GetYear();
			int index = statSim.GetFirstIndex(CHemlockLooperCR::O_DD, DD, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
			if (index >= 0)
			{
				obsStat += m_SAResult[i].m_obs[1];
				obs.push_back(make_pair(index, m_SAResult[i].m_obs[1]));
			}
		}

		//not enough observations
		if (obs.size() < m_SAResult.size() - 5)
			return false;


		sort(obs.begin(), obs.end());

		//regroup all duplication index
		vector<pair<size_t, double>>::iterator itLast = obs.begin();
		for (vector<pair<size_t, double>>::iterator it = obs.begin() + 1; it < obs.end(); it++)
		{
			if (itLast->first == it->first)
			{
				itLast->second += it->second;
				it = obs.erase(it) - 1;
			}
			else
			{
				itLast = it;
			}
		}

		//compute cumulated
		double sum = obsStat[SUM];
		if (sum > 0)
		{
			obs[0].second = obs[0].second / sum * 100;
			for (size_t i = 1; i < obs.size(); i++)
			{
				obs[i].second = obs[i - 1].second + obs[i].second / sum * 100;
			}
		}

		return true;
	}

	//because, in observation, eclosion and adult is cumulative and all other stages are non-cumulative,
	//we have to make a mixte between horizontal and vertical comparison
	void CHemlockLooperModel::ComputeCumulDiagonal(CModelStatVector& statSim, const CModelStatVector& statSim1)
	{
		statSim = statSim1;

		for (size_t p = P_L1_L2; p <= P_L4_PUPA; p++)
		{
			size_t ss = CHemlockLooperCR::O_FIRST_STAGE + p + 1;
			double sum = statSim1.GetStat(ss)[SUM];
			if (sum > 0)
			{
				for (size_t d = 1; d < statSim1.size(); d++)
				{
					double sum2 = 0;
					for (size_t pp = P_L1_L2; pp <= P_L4_PUPA; pp++)
					{
						size_t sss = CHemlockLooperCR::O_FIRST_STAGE + pp + 1;
						sum2 += statSim1[d][sss];
					}

					statSim[d][CHemlockLooperCR::O_FIRST_STAGE + 1] = 100 - statSim[d][CHemlockLooperCR::O_FIRST_STAGE];
					statSim[d][ss] = statSim[d - 1][ss] + (statSim1[d][ss] / (sum + sum2)) * 100;
				}
			}
		}
	}

	void CHemlockLooperModel::GetFValueDaily(CStatisticXY& stat)
	{
		ERMsg msg;

		if (m_SAResult.size() > 0)
		{
			//look to see if all pairs are ordered
			bool bValid = true;

			for (size_t p = P_L1_L2; p < NB_PARAMS&&bValid; p++)
			{
				if (m_a[p] < m_a[p - 1])
					bValid = false;
			}

			if (bValid)
			{

				//execute model with actual parameters
				CModelStatVector statSim;
				m_bCumulative = true;
				ExecuteDaily(statSim);


				//now compare simuation with observation data
				if (m_SAResult[0].m_obs.size() == 2)//if it's pupation data
				{

					vector<pair<size_t, double>> obs;

					if (!ComputePupation(statSim, obs))
					{
						//Pupation is out of season
						//add dummy poor data to reject these parameters set
						stat.Add(999, -999);
						stat.Add(-2345, 546);
						stat.Add(22, -567);
						return;
					}


					for (size_t i = 0; i < obs.size(); i++)
					{
						double obsV = obs[i].second;
						double simV = statSim[obs[i].first][CHemlockLooperCR::O_FIRST_STAGE + P_L4_PUPA + 1];

						stat.Add(simV, obsV);
					}

				}
				else //it's developement data 
				{
					for (size_t i = 0; i<m_SAResult.size(); i++)
					{
						for (size_t p = P_EGG_L1; p <= m_lastParam; p++)
						{
							if (m_SAResult[i].m_obs[p]>-999 &&
								statSim.IsInside(m_SAResult[i].m_ref))
							{
								double obsV = m_SAResult[i].m_obs[p];
								double simV = statSim[m_SAResult[i].m_ref][CHemlockLooperCR::O_FIRST_STAGE + p];

								stat.Add(simV, obsV);
							}
						}
					}
				}
			}
		}
	}
}