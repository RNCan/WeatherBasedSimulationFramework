//***********************************************************
// 17/10/2020	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LeucopisModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include "ModelBase/SimulatedAnnealingVector.h"

//Best parameters (separate)
//			Th1		Th2		mu		s		delta
//La G1		3.2		12.5	199.5	31.38	44
//La G2		3.4		40.5	861.9	54.45	39
//Ln		4.6		12.4	394.9	22.87	44

//Final parameters
//
//Th1 =  2.9
//Th2 = 19.1
//delta = 45 (February 15)
//		 mu		 s		
//La G1	239.6	41.4
//La G2	876.0	55.6
//Ln	625.2	38.3



using namespace WBSF::HOURLY_DATA;
using namespace std;


static const bool BEGIN_JULY = true;
static const size_t FIRST_Y = BEGIN_JULY ? 1 : 0;

namespace WBSF
{
	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
	enum { O_CDD, O_EMERGENCE_LA_G1, O_EMERGENCE_LA_G2, O_EMERGENCE_LP, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLeucopisModel::CreateObject);

	CLeucopisModel::CLeucopisModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2020)";

		m_bCumul = false;

		m_P[Τᴴ¹] = 2.9;
		m_P[Τᴴ²] = 19.1;
		m_P[delta] = 45;
		m_P[μ1] = 239.6;
		m_P[ѕ1] = 41.4;
		m_P[μ2] = 876.0;
		m_P[ѕ2] = 55.6;
		m_P[μ3] = 625.2;
		m_P[ѕ3] = 38.3;
			
	}

	CLeucopisModel::~CLeucopisModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLeucopisModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 1 + NB_PARAMS)
		{
			for (size_t p = 0; p < NB_PARAMS; p++)
				m_P[p] = parameters[c++].GetFloat();
		}

		return msg;
	}





	//This method is called to compute the solution
	ERMsg CLeucopisModel::OnExecuteDaily()
	{
		ERMsg msg;

		/*	if (m_weather.GetNbYears() < 2)
			{
				msg.ajoute("Laricobius Osakensis model need at least 2 years of data");
				return msg;
			}*/

			//if (!m_weather.IsHourly())
				//	m_weather.ComputeHourlyVariables();

			//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, -999);


		CModelStatVector CDD;
		GetCDD(m_weather, CDD);

		boost::math::logistic_distribution<double> emerge_dist1(m_P[μ1], m_P[ѕ1]);
		boost::math::logistic_distribution<double> emerge_dist2(m_P[μ2], m_P[ѕ2]);
		boost::math::logistic_distribution<double> emerge_dist3(m_P[μ3], m_P[ѕ3]);
		//boost::math::logistic_distribution<double> emerge_dist4(m_P[μ4], m_P[ѕ4]);

		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			m_output[d][O_CDD] = CDD[d][0];
			if (CDD[d][0] > -999)
			{
				m_output[d][O_EMERGENCE_LA_G1] = Round(100 * cdf(emerge_dist1, CDD[d][0]), 1);
				m_output[d][O_EMERGENCE_LA_G2] = Round(100 * cdf(emerge_dist2, CDD[d][0]), 1);
				m_output[d][O_EMERGENCE_LP] = Round(100 * cdf(emerge_dist3, CDD[d][0]), 1);
				//m_output[d][O_EMERGENCE_LN] = Round(100 * cdf(emerge_dist4, CDD[d][0]), 1);
			}
		}


		return msg;
	}

	//void CLeucopisModel::GetCDD(int year, const CWeatherYears& weather, CModelStatVector& CDD)
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

	void CLeucopisModel::GetCDD(const CWeatherYears& weather, CModelStatVector& CDD)
	{
		CDegreeDays DDmodel(DD_METHOD, m_P[Τᴴ¹], m_P[Τᴴ²]);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);



		CDD.Init(DD.GetTPeriod(), 1, -999);

		for (size_t y = 1; y < weather.GetNbYears(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod();
			p.Begin() = p.Begin() + int(m_P[delta]);
			CDD[p.Begin()][0] = DD[p.Begin()][CDegreeDays::S_DD];

			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				CDD[TRef][0] = CDD[TRef - 1][0] + DD[TRef][CDegreeDays::S_DD];
		}

	}

	enum TSpecies { S_LA_G1, S_LA_G2, S_LP, S_LN };
	enum TInput { I_N, I_S/*, I_P, I_CDD*/, NB_INPUTS };
	void CLeucopisModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 13);
		//SYC	site	Year	collection	col_date	emerge_date	daily_count	species	n_days P Time

		if (data[7] != "LN")
		{

			CSAResult obs;

			CStatistic egg_creation_date;

			obs.m_ref.FromFormatedString(data[5]);
			obs.m_obs.resize(NB_INPUTS);
			obs.m_obs[I_N] = stod(data[6]);
			//obs.m_obs[I_S] = ;
			//obs.m_obs[I_P] = stod(data[10]);
			//obs.m_obs[I_CDD] = stod(data[11]);
			//obs.m_obs[I_P] = stod(data[12]);


			if (data[7] == "LA" && data[8] == "1")
				obs.m_obs[I_S] = S_LA_G1;
			else if (data[7] == "LA" && data[8] == "2")
				obs.m_obs[I_S] = S_LA_G2;
			else if (data[7] == "LP")
				obs.m_obs[I_S] = S_LP;
			else if (data[7] == "LN")
				obs.m_obs[I_S] = S_LN;

			m_SAResult.push_back(obs);
		}
	}

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

	bool CLeucopisModel::IsParamValid()const
	{
		bool bValid = true;

		if (m_P[Τᴴ¹] >= m_P[Τᴴ²])
			bValid = false;


		return bValid;
	}





	//static const int ROUND_VAL = 4;
	//CTRef CLeucopisModel::GetEmergence(const CWeatherYear& weather)
	//{
	//	CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

	//	double sumDD = 0;
	//	for (CTRef TRef = p.Begin()+172; TRef <= p.End()&& TRef<= p.Begin() + int(m_ADE[ʎ0]); TRef++)
	//	{
	//		//size_t ii = TRef - p.Begin();
	//		const CWeatherDay& wday = m_weather.GetDay(TRef);
	//		double T = wday[H_TNTX][MEAN];
	//		T = Round(max(m_ADE[ʎa], T), ROUND_VAL);

	//		double DD = min(0.0, T - m_ADE[ʎb]);//DD is negative

	//		//if (ii < m_ADE[ʎ0])
	//			sumDD += DD;
	//	}


	//	boost::math::logistic_distribution<double> begin_dist(m_ADE[ʎ2], m_ADE[ʎ3]);
	//	int begin = (int)Round(m_ADE[ʎ0] + m_ADE[ʎ1] * cdf(begin_dist, sumDD), 0);
	//	return  p.Begin() + begin;
	//}
	enum TPout {P_CDD, P_CE, LA_G1= P_CE, P_LA_G2, P_LP, P_LN, NB_P};//CE = cumulative emergence
	void CLeucopisModel::GetPobs(CModelStatVector& P)
	{
		string ID = GetInfo().m_loc.m_ID;
		string SY = ID.substr(0, ID.length() - 2);

		//compute CDD for all temperature rprofile
		array< double, 4> total = { 0 };
		vector<tuple<double, CTRef, double, bool, size_t>> d;
		const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();

		for (size_t i = 0; i < SA.size(); i++)
		{
			string IDi = SA[i]->GetInfo().m_loc.m_ID;
			string SYi = IDi.substr(0, IDi.length() - 2);
			if (SYi == SY)
			{
				CModelStatVector CDD;
				GetCDD(SA[i]->m_weather, CDD);
				const CSAResultVector& v = SA[i]->GetSAResult();
				for (size_t ii = 0; ii < v.size(); ii++)
				{
					d.push_back(make_tuple(CDD[v[ii].m_ref][0], v[ii].m_ref, v[ii].m_obs[I_N], IDi == ID, v[ii].m_obs[I_S]));
					total[v[ii].m_obs[I_S]] += v[ii].m_obs[I_N];
				}
			}
		}

		sort(d.begin(), d.end());

		P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_P, 0);
		array< double, 4> sum = { 0 };
		for (size_t i = 0; i < d.size(); i++)
		{
			size_t s = std::get<4>(d[i]);
			sum[s] += std::get<2>(d[i]);
			if (std::get<3>(d[i]))
			{
				CTRef Tref = std::get<1>(d[i]);
				/*double obsP = -999;
				for (size_t k = 0; k < m_SAResult.size(); k++)
					if (m_SAResult[k].m_ref == Tref)
						obsP = m_SAResult[k].m_obs[I_P];*/


				double CDD = std::get<0>(d[i]);
				double p = Round(100 * sum[s] / total[s], 1);
				
				P[Tref][P_CDD] = CDD;
				P[Tref][P_CE + s] = p;
			}
		}
	}

	void CLeucopisModel::CalibrateEmergence(CStatisticXY& stat)
	{
		if (m_SAResult.empty())
			return;

		//boost::math::lognormal_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);



		//boost::math::weibull_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::beta_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::exponential_distribution<double> emerge_dist(m_P[ѕ]);
		//boost::math::rayleigh_distribution<double> emerge_dist(m_P[ѕ]);


		//CModelStatVector CDD; 
		//GetCDD(m_weather, CDD);

		double n = 0;

		for (size_t i = 0; i < m_SAResult.size(); i++)
			n += m_SAResult[i].m_obs[I_N];


		CModelStatVector P;
		GetPobs(P);

		//array<boost::math::logistic_distribution<double>,4> emerge_dist(mu, S);
		//		array<boost::math::logistic_distribution<double>, 4> emerge_dist = { {m_P[μ1], m_P[ѕ1], m_P[μ1], m_P[ѕ1]} };

		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			size_t s = m_SAResult[i].m_obs[I_S];
			double mu = m_P[μ1 + 2 * s];
			double S = m_P[ѕ1 + 2 * s];

			boost::math::logistic_distribution<double> emerge_dist(mu, S);

			double CDD = P[m_SAResult[i].m_ref][P_CDD];
			double obs = P[m_SAResult[i].m_ref][P_CE+s];
			ASSERT(obs >= 0 && obs <= 100);

			double sim = Round(100 * cdf(emerge_dist, max(0.0, CDD)), 1);
			for (size_t ii = 0; ii < log(n); ii++)
				stat.Add(obs, sim);

		}//for all results

		return;

	}


	void CLeucopisModel::GetFValueDaily(CStatisticXY& stat)
	{
		//bitset<3> test;
		//test.reset();

		//test.set(I_EGGS);
		//test.set(I_LARVAE);
		//test.set(I_EMERGED_ADULT);

		if (!IsParamValid())
			return;

		return CalibrateEmergence(stat);

		//return CalibrateOviposition(stat);

		//if (!m_SAResult.empty())
		//{
		//	if (!m_bCumul)
		//		m_bCumul = true;//SA always cumulative

		//	if (!m_weather.IsHourly())
		//		m_weather.ComputeHourlyVariables();

		//	//low and hi relative development rate must be approximatively the same
		//	//if (!IsParamValid())
		//		//return;



		//	for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		//	{
		//		int year = m_weather[y].GetTRef().GetYear();
		//		if ((test[0] && m_years[I_EGGS].find(year) != m_years[I_EGGS].end()) ||
		//			(test[1] && m_years[I_LARVAE].find(year) != m_years[I_LARVAE].end()) ||
		//			(test[2] && m_years[I_EMERGED_ADULT].find(year) != m_years[I_EMERGED_ADULT].end()))
		//		{

		//			CModelStatVector output;
		//			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
		//			//not possible to add a second year without having problem in evaluation....
		//			//if (m_weather[y].HaveNext())
		//				//p.End() = m_weather[y + 1].GetEntireTPeriod(CTM(CTM::DAILY)).End();

		//			output.Init(p, NB_STATS, 0);
		//			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);

		//			static const size_t STAT_STAGE[3] = { S_EGG, S_L1, S_ACTIVE_ADULT };

		//			for (size_t i = 0; i < m_SAResult.size(); i++)
		//			{
		//				if (output.IsInside(m_SAResult[i].m_ref))
		//				{

		//					for (size_t j = 0; j < NB_INPUTS; j++)
		//					{
		//						if (test[j])
		//						{
		//							double obs_y = Round(m_SAResult[i].m_obs[j], ROUND_VAL);
		//							double sim_y = Round(output[m_SAResult[i].m_ref][STAT_STAGE[j]], ROUND_VAL);

		//							if (obs_y > -999)
		//							{
		//								stat.Add(obs_y, sim_y);

		//								double obs_x = m_SAResult[i].m_ref.GetJDay();
		//								double sim_x = GetSimX(STAT_STAGE[j], m_SAResult[i].m_ref, obs_y, output);

		//								/*if (sim_x > -999)
		//								{
		//									obs_x = Round(100 * (obs_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE],ROUND_VAL);
		//									sim_x = Round(100 * (sim_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE],ROUND_VAL);
		//									stat.Add(obs_x, sim_x);
		//								}*/
		//							}
		//						}
		//					}
		//				}
		//			}//for all results
		//		}//have data
		//	}
		//}
	}



	//void CLeucopisModel::FinalizeStat(CStatisticXY& stat)
	//{
	//	
	//}
}
