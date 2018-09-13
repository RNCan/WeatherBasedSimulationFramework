//**************************************************************************************************************
// 13/04/2018	1.0.1	Rémi Saint-Amant	Compile with VS 2017
// 08/05/2017	1.0.0	Rémi Saint-Amant	Create from articles
//												Lyons and Jones 2006
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "EmeraldAshBorer.h"

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{


	class CMicroClimate
	{
	public:

		CMicroClimate(const CWeatherDay& weather);

		double GetT(size_t h, size_t hourTmax = 16)const;
		double GetTair()const{ return (m_Tmin + m_Tmax) / 2.0; }

	protected:

		double m_Tmin;
		double m_Tmax;
	};

	CMicroClimate::CMicroClimate(const CWeatherDay& weather)
	{
		double Tmin = weather[H_TMIN][MEAN];
		double Tmax = weather[H_TMAX][MEAN];
		double Trange = Tmax - Tmin;
		double Sin = sin(2 * 3.14159*(weather.GetTRef().GetJDay() / 365. - 0.25));

		//convert air temperature to bark temperature
		m_Tmin = -0.1493 + 0.8359*Tmin + 0.5417*Sin + 0.16980*Trange + 0.00000*Tmin*Sin + 0.005741*Tmin*Trange + 0.02370*Sin*Trange;
		m_Tmax = 0.4196 + 0.9372*Tmax - 0.4265*Sin + 0.05171*Trange + 0.03125*Tmax*Sin - 0.004270*Tmax*Trange + 0.09888*Sin*Trange;

		if (m_Tmin > m_Tmax)
			Switch(m_Tmin, m_Tmax);
	}

	double CMicroClimate::GetT(size_t h, size_t hourTmax)const
	{
		double OH = 0;

		double range = m_Tmax - m_Tmin;
		assert(range >= 0);

		int time_factor = (int)hourTmax - 6;  //  "rotates" the radian clock to put the hourTmax at the top  
		double theta = ((int)h - time_factor)*3.14159 / 12.0;
		double T = (m_Tmin + m_Tmax) / 2 + range / 2 * sin(theta);

		return T;
	}



	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEmeraldAshBorerModel::CreateObject);


	//Defining a simple continuing ratio model
	enum TInstar{ EGG, L1, L2, L3, L4, PRE_PUPA, PUPA, TENERAL, ADULT, DEAD_ADULT, EMERGENCE, NB_STAGES };
	extern const char HEADER[] = "EGG,L1,L2,L3,L4,Pre-Pupa,Pupa,Teneral,Adult,DeadAdult,Emergence";
	//typedef CContinuingRatio<NB_PARAMS, EGG, ADULT, HEADER> CHemlockLooperCR;
	static const double T_THRESHOLD[2][NB_STAGES] =
	{
		{ 13.9, 0, 0, 0, 0, 12.0, 13.6, 13.6, 0 },
		{ 13.9, 0, 0, 0, 0, 11.5, 14.7, 10.1, 0 },
	};

	static const double DD_THRESHOLD[2][NB_STAGES] =
	{
		{ 155.2, 0, 0, 0, 0, 118.3, 139.2, 43.1, 0 },
		{ 155.2, 0, 0, 0, 0, 121.0, 114.6, 64.4, 0 },
	};

	//const size_t CEmeraldAshBorerModel::FISRT_HL_JDAY = 61 - 1;   //first of March zero base
	//const double CEmeraldAshBorerModel::THRESHOLD = 5.4;			//°C
	//const double CEmeraldAshBorerModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 1075.5 };	//accumulated degree-days
	//const double CEmeraldAshBorerModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 1.2437 };	//variability
	//const double CEmeraldAshBorerModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 945.9};	//accumulated degree-days
	//const double CEmeraldAshBorerModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 0.741 };	//variability


	CEmeraldAshBorerModel::CEmeraldAshBorerModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2018)";

		//m_bCumulative = false;
		//m_startJday = FISRT_HL_JDAY;
		//m_threshold = THRESHOLD;
		//for (int i = 0; i < NB_PARAMS; i++)
		//{
		//	m_a[i] = A[i];
		//	m_b[i] = B[i];
		//}
		//m_lastParam = ADULT;
	}

	CEmeraldAshBorerModel::~CEmeraldAshBorerModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CEmeraldAshBorerModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		/*size_t c = 0;
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
*/
		return msg;
	}

	ERMsg CEmeraldAshBorerModel::OnExecuteDaily()
	{
		ERMsg msg;

		//Excute model on a daily basis
		ExecuteDaily(m_output);

		return msg;
	}

	void CEmeraldAshBorerModel::ExecuteDaily(CModelStatVector& output)
	{
		//for (size_t y = 0; y < m_weather.size(); y++)
			//m_weather[y][H_TAIR2];

		//m_weather.SetHourly(false);


		//execute daily using continuing ratio structure
		/*CHemlockLooperCR CR;
		CR.m_startJday = m_startJday;
		CR.m_lowerThreshold = m_threshold;
		CR.m_method = CDegreeDays::DAILY_AVERAGE;
		CR.m_bPercent = true;
		CR.m_bCumul = m_bCumulative;
		CR.m_bMultipleVariance = true;
		CR.m_bAdjustFinalProportion = true;*/

	/*	for (size_t i = 0; i < NB_PARAMS; i++)
		{
			CR.m_a[i] = m_a[i];
			CR.m_b[i] = m_b[i];
		}*/

		output.Init(m_weather.GetEntireTPeriod(), NB_STAGES, 0, HEADER);//+3 for DD, death and AI
		COverheat overheating(0.0);

		for (size_t y = 0; y < m_weather.size(); y++)
		{
			size_t stage[2] = { PRE_PUPA, PRE_PUPA };
			double DD[2] = { 0 };
			double DD_E[2] = { 0 };

			//double DD = 0.0;
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& wDay = m_weather.GetDay(TRef);
				CMicroClimate micro(wDay);
				double T = micro.GetTair();
				
				for (size_t s = 0; s < 2; s++)
				{
					if (stage[s] < ADULT)
					{
						//if (TRef.GetJDay() >= m_startJday)
						DD[s] += max(0.0, T - T_THRESHOLD[s][stage[s]]);
						if (DD[s] > DD_THRESHOLD[s][stage[s]])
						{
							DD[s] = 0;
							stage[s] ++;
						}
					}
					else if (stage[s] == ADULT)
					{
						for (size_t h = 0; h < 24; h++)
						{
							double R = 0.0004130*exp(0.187197*T);
							DD[s] += R/24;
						}

						if (DD[s] >= 1)
						{
							DD[s] = 0;
							stage[s] ++;
						}
					}

					static const double MAX_DDE[2] = {303.0,344.8};
					if (DD_E[s] < MAX_DDE[s] && DD_E[s] + max(0.0, T - 13.5 ) >= MAX_DDE[s])
					{
						output[TRef][EMERGENCE] += 50;
					}
					DD_E[s] += max(0.0, T - 13.5);

					output[TRef][stage[s]] += 50;
				}
			}
		}
		
		//CR.Execute(m_weather, output);


	}



	

	//*****************************************************************************************************************
	//Next 4 methods are for Simulated Annealing
//	enum TPupasion{ PUP_NB_DAYS, PUP_N, NB_PUPAISON };
//	void CEmeraldAshBorerModel::AddDailyResult(const StringVector& header, const StringVector& data)
//	{
//		enum TInputAllStage{ E_SITE, E_DATE, E_YEAR, E_MONTH, E_DAY, E_JDAY, E_EGG, E_L1, E_L2, E_L3, E_L4, E_PUPAE, E_ADULT, E_CUMUL_EGG, E_CUMUL_L1, E_CUMUL_L2, E_CUMUL_L3, E_CUMUL_L4, E_CUMUL_PUPAE, E_CUMUL_ADULTS, NB_COLUMNS };
//		//enum TInputAllStage{ E_NAME, E_ID, E_YEAR, E_MONTH, E_DAY, E_JDAY, E_CUMUL_L1, E_CUMUL_L2, E_CUMUL_L3, E_CUMUL_L4, E_CUMUL_PUPA, E_CUMUL_ADULT, NB_INPUT, NB_COLUMN = NB_INPUT + 7 };
//		enum TInputPupasion{ P_NAME, P_ID, P_YEAR, P_MONTH, P_DAY, P_NB_DAYS, P_N, NB_INPUT_PUPAISON };
//
//		CTRef TRef(ToInt(data[E_YEAR]), ToSizeT(data[E_MONTH]) - 1, ToSizeT(data[E_DAY]) - 1);
//
//		if (header.size() == NB_COLUMNS)
//		{
//			std::vector<double> obs(NB_COLUMNS - E_CUMUL_EGG);
//
//			for (size_t i = 0; i < obs.size(); i++)
//				obs[i] = ToDouble(data[E_CUMUL_EGG + i]);
//
//			m_SAResult.push_back(CSAResult(TRef, obs));
//		}
//		else if (header.size() == NB_INPUT_PUPAISON)
//		{
//			std::vector<double> obs(2);
//
//			obs[PUP_NB_DAYS] = ToDouble(data[P_NB_DAYS]);
//			obs[PUP_N] = ToDouble(data[P_N]);
//
//			m_SAResult.push_back(CSAResult(TRef, obs));
//		}
//	}
//
//
//	bool CEmeraldAshBorerModel::ComputePupation(const CModelStatVector& statSim, vector<pair<size_t, double>>& obs)
//	{
//		//compute simulated pupation at the same date as observations
//		CStatistic obsStat;
//		for (size_t i = 0; i < m_SAResult.size(); i++)
//		{
//			double DD = statSim[m_SAResult[i].m_ref][CHemlockLooperCR::O_DD];
//			DD += (20 - m_threshold)*m_SAResult[i].m_obs[PUP_NB_DAYS];
//
//			int year = m_SAResult[i].m_ref.GetYear();
//			int index = statSim.GetFirstIndex(CHemlockLooperCR::O_DD, DD, 1, CTPeriod(year, FIRST_MONTH, FIRST_DAY, year, LAST_MONTH, LAST_DAY));
//			if (index >= 0)
//			{
//				obsStat += m_SAResult[i].m_obs[1];
//				obs.push_back(make_pair(index, m_SAResult[i].m_obs[1]));
//			}
//		}
//
//		//not enough observations
//		if (obs.size() < m_SAResult.size() - 5)
//			return false;
//
//
//		sort(obs.begin(), obs.end());
//
//		//regroup all duplication index
//		vector<pair<size_t, double>>::iterator itLast = obs.begin();
//		for (vector<pair<size_t, double>>::iterator it = obs.begin() + 1; it < obs.end(); it++)
//		{
//			if (itLast->first == it->first)
//			{
//				itLast->second += it->second;
//				it = obs.erase(it) - 1;
//			}
//			else
//			{
//				itLast = it;
//			}
//		}
//
//		//compute cumulated
//		double sum = obsStat[SUM];
//		if (sum > 0)
//		{
//			obs[0].second = obs[0].second / sum * 100;
//			for (size_t i = 1; i < obs.size(); i++)
//			{
//				obs[i].second = obs[i - 1].second + obs[i].second / sum * 100;
//			}
//		}
//
//		return true;
//	}
//
//	//because, in observation, eclosion and adult is cumulative and all other stages are non-cumulative,
//	//we have to make a mixte between horizontal and vertical comparison
//	void CEmeraldAshBorerModel::ComputeCumulDiagonal(CModelStatVector& statSim, const CModelStatVector& statSim1)
//	{
//		statSim = statSim1;
//
//		for (size_t p = P_L1_L2; p <= P_L4_PUPA; p++)
//		{
//			size_t ss = CHemlockLooperCR::O_FIRST_STAGE + p + 1;
//			double sum = statSim1.GetStat(ss)[SUM];
//			if (sum > 0)
//			{
//				for (size_t d = 1; d < statSim1.size(); d++)
//				{
//					double sum2 = 0;
//					for (size_t pp = P_L1_L2; pp <= P_L4_PUPA; pp++)
//					{
//						size_t sss = CHemlockLooperCR::O_FIRST_STAGE + pp + 1;
//						sum2 += statSim1[d][sss];
//					}
//
//					statSim[d][CHemlockLooperCR::O_FIRST_STAGE + 1] = 100 - statSim[d][CHemlockLooperCR::O_FIRST_STAGE];
//					statSim[d][ss] = statSim[d - 1][ss] + (statSim1[d][ss] / (sum + sum2)) * 100;
//				}
//			}
//		}
//	}
//
//	void CEmeraldAshBorerModel::GetFValueDaily(CStatisticXY& stat)
//	{
//		ERMsg msg;
//
//		if (m_SAResult.size() > 0)
//		{
//			//look to see if all pairs are ordered
//			bool bValid = true;
//
//			for (size_t p = P_L1_L2; p < NB_PARAMS&&bValid; p++)
//			{
//				if (m_a[p] < m_a[p - 1])
//					bValid = false;
//			}
//
//			if (bValid)
//			{
//
//				//execute model with actual parameters
//				CModelStatVector statSim;
//				m_bCumulative = true;
//				ExecuteDaily(statSim);
//
//
//				//now compare simuation with observation data
//				if (m_SAResult[0].m_obs.size() == 2)//if it's pupation data
//				{
//
//					vector<pair<size_t, double>> obs;
//
//					if (!ComputePupation(statSim, obs))
//					{
//						//Pupation is out of season
//						//add dummy poor data to reject these parameters set
//						stat.Add(999, -999);
//						stat.Add(-2345, 546);
//						stat.Add(22, -567);
//						return;
//					}
//
//
//					for (size_t i = 0; i < obs.size(); i++)
//					{
//						double obsV = obs[i].second;
//						double simV = statSim[obs[i].first][CHemlockLooperCR::O_FIRST_STAGE + P_L4_PUPA + 1];
//
//						stat.Add(simV, obsV);
//					}
//
//				}
//				else //it's developement data 
//				{
//					for (size_t i = 0; i<m_SAResult.size(); i++)
//					{
//						for (size_t p = P_EGG_L1; p <= m_lastParam; p++)
//						{
//							if (m_SAResult[i].m_obs[p]>-999 &&
//								statSim.IsInside(m_SAResult[i].m_ref))
//							{
//								double obsV = m_SAResult[i].m_obs[p];
//								double simV = statSim[m_SAResult[i].m_ref][CHemlockLooperCR::O_FIRST_STAGE + p];
//
//								stat.Add(simV, obsV);
//							}
//						}
//					}
//				}
//			}
//		}
//	}
}