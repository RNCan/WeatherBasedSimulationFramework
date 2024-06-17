//***********************************************************
// 18/10/2022	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LeucopisArgenticollisModel.h"
#include "LeucopisArgenticollisEquations.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/non_central_f.hpp>
#include <boost/math/distributions/extreme_value.hpp>
#include <boost/math/distributions/fisher_f.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LAZ;
using namespace std;

static const bool BEGIN_DECEMBER = false;
static const size_t FIRST_Y = BEGIN_DECEMBER ? 1 : 0;

namespace WBSF
{
	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
	//enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLeucopisArgenticollisModel::CreateObject);

	CLeucopisArgenticollisModel::CLeucopisArgenticollisModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.2 (2024)";


		m_bApplyAttrition = false;
		m_bCumul = false;

		for (size_t p = 0; p < NB_EMERGENCE_PARAMS; p++)
			m_adult_emerg[p] = CLeucopisArgenticollisEquations::ADULT_EMERG[p];


		for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
			m_pupa_param[p] = CLeucopisArgenticollisEquations::PUPA_PARAM[p];


		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
			m_ovip_param[p] = CLeucopisArgenticollisEquations::OVIP_PARAM[p];

		//for (size_t p = 0; p < m_EOD_param.size(); p++)
			//m_EOD_param[p] = CLeucopisArgenticollisEquations::OVIP_PARAM[p];


	}

	CLeucopisArgenticollisModel::~CLeucopisArgenticollisModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLeucopisArgenticollisModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bApplyAttrition = parameters[c++].GetBool();
		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 2 + LAZ::NB_EMERGENCE_PARAMS + NB_PUPA_PARAMS + NB_OVIP_PARAMS)
		{
			for (size_t p = 0; p < LAZ::NB_EMERGENCE_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
				m_pupa_param[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
				m_ovip_param[p] = parameters[c++].GetFloat();

			//for (size_t p = 0; p < m_EOD_param.size(); p++)
			//m_EOD_param[p] = parameters[c++].GetFloat();
			
		}


		return msg;
	}





	//This method is called to compute the solution
	ERMsg CLeucopisArgenticollisModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = FIRST_Y; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CLeucopisArgenticollisModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLAZStand stand(this);

		stand.m_bApplyAttrition = m_bApplyAttrition;
		//Set parameters to equation
		for (size_t p = 0; p < LAZ::NB_EMERGENCE_PARAMS; p++)
			stand.m_equations.m_adult_emerg[p] = m_adult_emerg[p];

		for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
			stand.m_equations.m_pupa_param[p] = m_pupa_param[p];

		for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
			stand.m_equations.m_ovip_param[p] = m_ovip_param[p];

		stand.init(year, weather);

		//Create host
		CLAZHostPtr pHost(new CLAZHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CLeucopisArgenticollis>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, PUPAE));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		if (output.empty())
			output.Init(p, NB_STATS, 0);


		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{
			stand.Live(weather.GetDay(d));
			if (output.IsInside(d))
				stand.GetStat(d, output[d]);

			stand.AdjustPopulation();
			HxGridTestConnection();
		}



		if (m_bCumul)
		{
	

			//for (size_t s = S_EMERGENCE0; s <= S_EMERGENCE1; s++)
			//{
			//	//CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

			//	CStatistic stat = output.GetStat(s, p);
			//	if (stat.IsInit() && stat[SUM] > 0)
			//	{
			//		output[0][s] = output[0][s] * 100 / stat[SUM];//when first day is not 0
			//		for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
			//		{
			//			output[d][s] = output[d - 1][s] + output[d][s] * 100 / stat[SUM];
			//			_ASSERTE(!_isnan(output[d][s]));
			//		}
			//	}
			//}

			for (size_t s = 0; s < NB_STATS; s++)
			{
				//size_t s = CUM_STAT[ss];
				if (CUM_STAT[s])
				{
					CStatistic stat = output.GetStat(s, p);
					if (stat.IsInit() && stat[SUM] > 0)
					{
						output[0][s] = output[0][s] * 100 / stat[SUM];//when first day is not 0
						for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
						{
							output[d][s] = output[d - 1][s] + output[d][s] * 100 / stat[SUM];
							_ASSERTE(!_isnan(output[d][s]));
						}
					}
				}
			}
		}
	}

	enum TSpecies { S_LA_G0, S_LA_G1, S_LP, S_LN };
	enum TInput { I_SYC, I_SITE, I_YEAR, I_COLLECTION, I_SPECIES, I_G, I_DATE, I_CDD, I_TMIN, I_DAILY_COUNT, NB_INPUTS };
	enum TInputInternal { I_S, I_N, NB_INPUTS_INTERNAL };
	void CLeucopisArgenticollisModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		CStatistic egg_creation_date;

		if (data[I_SPECIES] == "La" && data[I_G] == "1")
		{
			obs.m_ref.FromFormatedString(data[I_DATE]);
			obs.m_obs.resize(NB_INPUTS_INTERNAL);
			obs.m_obs[I_S] = S_LA_G1;
			obs.m_obs[I_N] = stod(data[I_DAILY_COUNT]);


			ASSERT(obs.m_obs[I_N] >= 0);
			m_SAResult.push_back(obs);
		}





	}

	bool CLeucopisArgenticollisModel::IsParamValid()const
	{
		bool bValid = true;


		if (m_adult_emerg[Τᴴ¹] >= m_adult_emerg[Τᴴ²])
			bValid = false;

		return bValid;
	}

	enum TPout { P_CDD, P_CE, LA_G1 = P_CE, P_LA_G2, P_LP, P_LN, NB_P };//CE = cumulative emergence
	void CLeucopisArgenticollisModel::GetPobs(CModelStatVector& P)
	{
		string ID = GetInfo().m_loc.m_ID;
		string SY = ID.substr(0, ID.length() - 2);

		//compute CDD for all temperature profile
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

				//degree day of the La g2 
				CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
				DDmodel.GetCDD(int(m_adult_emerg[delta]), m_weather, CDD);

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
				double CDD = std::get<0>(d[i]);
				double p = Round(100 * sum[s] / total[s], 1);

				P[Tref][P_CDD] = CDD;
				P[Tref][P_CE + s] = p;
			}
		}
	}


	//enum TPout { P_CDD, P_CE };//CE = cumulative emergence
	//void CLeucopisArgenticollisModel::GetPobsUni(CModelStatVector& P)
	//{
	//	CLeucopisArgenticollisEquations equations(RandomGenerator());

	//	for (size_t p = 0; p < LAZ::NB_EMERGENCE_PARAMS; p++)
	//		equations.m_adult_emerg[p] = m_adult_emerg[p];

	//	for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
	//		equations.m_pupa_param[p] = m_pupa_param[p];

	//	for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
	//		equations.m_ovip_param[p] = m_ovip_param[p];


	//	string ID = GetInfo().m_loc.m_ID;
	//	string SY = ID.substr(0, ID.length() - 2);

	//	//compute CDD for all temperature profile
	//	double total = 0;
	//	vector<tuple<double, CTRef, double, bool>> d;
	//	const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();

	//	for (size_t i = 0; i < SA.size(); i++)
	//	{

	//		string IDi = SA[i]->GetInfo().m_loc.m_ID;
	//		//string SYi = IDi.substr(0, IDi.length() - 2);
	//		//if (SYi == SY)
	//		//{
	//		CModelStatVector CDD;
	//		equations.GetAdultEmergingCDD(SA[i]->m_weather, CDD);
	//		const CSAResultVector& v = SA[i]->GetSAResult();
	//		for (size_t ii = 0; ii < v.size(); ii++)
	//		{
	//			d.push_back(make_tuple(CDD[v[ii].m_ref][0], v[ii].m_ref, v[ii].m_obs[0], IDi == ID));
	//			total += v[ii].m_obs[0];
	//		}
	//		//}
	//	}

	//	sort(d.begin(), d.end());

	//	P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), 2, -999);
	//	double sum = 0;
	//	for (size_t i = 0; i < d.size(); i++)
	//	{
	//		//size_t s = std::get<4>(d[i]);
	//		sum += std::get<2>(d[i]);

	//		if (std::get<3>(d[i]))//used only the appropriate values for this site/year
	//		{
	//			CTRef Tref = std::get<1>(d[i]);
	//			double CDD = std::get<0>(d[i]);
	//			double p = Round(100 * sum / total, 1);

	//			P[Tref][P_CDD] = CDD;
	//			P[Tref][P_CE] = p;
	//		}
	//	}
	//}

	//void CLeucopisArgenticollisModel::GetPobs(CModelStatVector& P)
	//{
	//	CLeucopisArgenticollisEquations equations(RandomGenerator());

	//	for (size_t p = 0; p < LAZ::NB_EMERGENCE_PARAMS; p++)
	//		equations.m_adult_emerg[p] = m_adult_emerg[p];

	//	for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
	//		equations.m_pupa_param[p] = m_pupa_param[p];

	//	for (size_t p = 0; p < NB_OVIP_PARAMS; p++)
	//		equations.m_ovip_param[p] = m_ovip_param[p];


	//	CModelStatVector CDD;
	//	equations.GetAdultEmergingCDD(m_weather, CDD);


	//	//compute CDD for all temperature profile
	//	double total = 0;
	//	for (size_t i = 0; i < m_SAResult.size(); i++)
	//	{
	//		total += m_SAResult[i].m_obs[0];
	//	}

	//	P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), 2, -999);
	//	double sum = 0;
	//	for (size_t i = 0; i < m_SAResult.size(); i++)
	//	{
	//		CTRef Tref = m_SAResult[i].m_ref;
	//		sum += m_SAResult[i].m_obs[0];

	//		P[Tref][P_CDD] = CDD[Tref][0];
	//		P[Tref][P_CE] = Round(100 * sum / total, 1);
	//	}
	//}

//	bool CLeucopisArgenticollisModel::CalibrateEmergence(CStatisticXY& stat)
//	{
//		if (m_SAResult.size() < 4)
//			return true;
//
//		//boost::math::lognormal_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
//
//
//
//		//boost::math::weibull_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
//		//boost::math::beta_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
//		//boost::math::exponential_distribution<double> emerge_dist(m_P[ѕ]);
//		//boost::math::rayleigh_distribution<double> emerge_dist(m_P[ѕ]);
//
//		double n = 0;
//		for (size_t i = 0; i < m_SAResult.size(); i++)
//			n += m_SAResult[i].m_obs[0];
//
//
//		CModelStatVector P;
//		GetPobs(P);
//
//		//array<boost::math::logistic_distribution<double>,4> emerge_dist(mu, S);
//		//		array<boost::math::logistic_distribution<double>, 4> emerge_dist = { {m_P[μ1], m_P[ѕ1], m_P[μ1], m_P[ѕ1]} };
//
//		double mu = m_adult_emerg[μ]; 
//		double S = m_adult_emerg[ѕ];
//		boost::math::logistic_distribution<double> emerge_dist(mu, S);
//
//		for (size_t i = 0; i < m_SAResult.size(); i++)
//		{
//			//size_t s = m_SAResult[i].m_obs[I_S];
//
////			double CDD = P[m_SAResult[i].m_ref][P_CDD];
//	//		double obs = P[m_SAResult[i].m_ref][P_CE + s];
//			//double CDD = CDD_output[m_SAResult[i].m_ref][0];
//			double CDD = P[m_SAResult[i].m_ref][P_CDD];
//			//double obs = m_SAResult[i].m_obs[1];
//			double obs = P[m_SAResult[i].m_ref][P_CE];
//
//			ASSERT(CDD >= 0);
//			ASSERT(obs >= 0 && obs <= 100);
//
//
//			double sim = Round(100 * cdf(emerge_dist, max(0.0, CDD)), 1);
//			for (size_t ii = 0; ii < log(n); ii++)
//				stat.Add(obs, sim);
//
//		}//for all results
//
//
//		return true;
//
//	}

	bool CLeucopisArgenticollisModel::CalibrateEmergenceG2(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{
			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();



			m_bCumul = true;//SA always cumulative
			//Always used the same seed for calibration
			m_randomGenerator.Randomize(CRandomGenerator::FIXE_SEED);


			
			//low and hi relative development rate must be approximatively the same
			//if (!IsParamValid())
				//return;

			if (m_SAResult.back().m_obs.size() == NB_INPUTS_INTERNAL)
			{
				


				CModelStatVector P;
				GetPobs(P);

				for (size_t i = 0; i < m_SAResult.size(); i++)
				{
					//double CDD = P[m_SAResult[i].m_ref][P_CDD];
					double cumul_obs = P[m_SAResult[i].m_ref][P_LA_G2];
					ASSERT(cumul_obs >= 0 && cumul_obs <= 100);

					m_SAResult[i].m_obs.push_back(cumul_obs);
				}


			}


			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();

				CModelStatVector output;
				CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

				output.Init(p, NB_STATS, 0);
				ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);


				for (size_t i = 0; i < m_SAResult.size(); i++)
				{
					if (output.IsInside(m_SAResult[i].m_ref))
					{

						double obs_y = Round(m_SAResult[i].m_obs[NB_INPUTS_INTERNAL], 4);
						double sim_y = Round(output[m_SAResult[i].m_ref][S_EMERGENCE1a], 4);

						if (obs_y > -999)
						{
							for (size_t ii = 0; ii < m_SAResult[i].m_obs[I_N]; ii++)
								stat.Add(obs_y, sim_y);
						}
					}
				}//for all results

			}
		}

		return true;
	}


	bool CLeucopisArgenticollisModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;

		//return CalibrateEmergence(stat);

		return CalibrateEmergenceG2(stat);
	}


}
