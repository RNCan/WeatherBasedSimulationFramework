//***********************************************************
// 19/02/2024	1.0.1   Rémi Saint-Amant   Compile with VS 2022
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



//Best parameters separate, v 1.0.1 (2024-02-25)
//Sp	G	n		To	Th1		Th2		mu		s		R²
//La	1	459		45	2.5		18.5	239.1	44.01	0.870
//Lp	1	1842	45	2.5		18.5	646.1	41.37	0.980
//La	2	1674	45	2.5		50.0	933.3	53.02	0.987


using namespace WBSF::HOURLY_DATA;
using namespace std;


static const bool BEGIN_JULY = true;
static const size_t FIRST_Y = BEGIN_JULY ? 1 : 0;

namespace WBSF
{

	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
	enum { O_CDD_LA_G1, O_EMERGENCE_LA_G1, O_CDD_LP, O_EMERGENCE_LP, O_CDD_LA_G2, O_EMERGENCE_LA_G2, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLeucopisModel::CreateObject);

	CLeucopisModel::CLeucopisModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2024)";

		m_bCumul = false;


		//La g1
		m_P[P_LA_G1 + Τᴴ¹] = 2.5;
		m_P[P_LA_G1 + Τᴴ²] = 18.5;
		m_P[P_LA_G1 + delta] = 45;
		m_P[P_LA_G1 + μ] = 239.1;
		m_P[P_LA_G1 + ѕ] = 44.01;
		
		//Lp
		m_P[P_LP + Τᴴ¹] = 2.5;
		m_P[P_LP + Τᴴ²] = 18.5;
		m_P[P_LP + delta] = 45;
		m_P[P_LP + μ] = 646.1;
		m_P[P_LP + ѕ] = 41.37;
		
		//La g2
		m_P[P_LA_G2 + Τᴴ¹] = 2.5;
		m_P[P_LA_G2 + Τᴴ²] = 50.0;
		m_P[P_LA_G2 + delta] = 45;
		m_P[P_LA_G2 + μ] = 933.3;
		m_P[P_LA_G2 + ѕ] = 53.02;
		

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

		if (parameters.size() == 1 + NB_PARAMS + 2)
		{
			for (size_t p = 0; p < NB_PARAMS; p++)
				m_P[p] = parameters[c++].GetFloat();


			if (parameters[c++].GetBool())//same as La g1 (Th)
			{
				m_P[P_LA_G2 + Τᴴ¹] = m_P[P_LA_G1 + Τᴴ¹];
				m_P[P_LA_G2 + delta] = m_P[P_LA_G1 + delta];
			}

			if (parameters[c++].GetBool())//same as La g1
			{
				m_P[P_LP + Τᴴ¹] = m_P[P_LA_G1 + Τᴴ¹];
				m_P[P_LP + Τᴴ²] = m_P[P_LA_G1 + Τᴴ²];
				m_P[P_LP + delta] = m_P[P_LA_G1 + delta];
			}
		}

		return msg;
	}





	//This method is called to compute the solution
	ERMsg CLeucopisModel::OnExecuteDaily()
	{
		ERMsg msg;


		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_OUTPUTS, 0);


		array<CModelStatVector, NB_SPECIES> CDD;
		GetCDD(m_weather, CDD);

		array< boost::math::logistic_distribution<double>, NB_SPECIES> emerge_dist = { { {m_P[P_LA_G1 + μ], m_P[P_LA_G1 + ѕ]},{m_P[P_LP + μ], m_P[P_LP + ѕ]},{m_P[P_LA_G2 + μ], m_P[P_LA_G2 + ѕ]} } };
		for (size_t s = 0; s < NB_SPECIES; s++)
		{
			for (CTRef d = p.Begin(); d <= p.End(); d++)
			{
				if (d.GetJDay() >= m_P[s * NB_CDD_PARAMS + delta])
				{
					m_output[d][s * 2 + 0] = CDD[s][d][0];
					m_output[d][s * 2 + 1] = Round(100 * cdf(emerge_dist[s], CDD[s][d][0]), 1);
				}
			}
		}

		if (!m_bCumul)
		{
			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				CTPeriod p = m_weather[y].GetEntireTPeriod();
				for (CTRef d = p.End(); d > p.Begin(); d--)
				{
					m_output[d][O_EMERGENCE_LA_G1] = m_output[d][O_EMERGENCE_LA_G1] - m_output[d - 1][O_EMERGENCE_LA_G1];
					m_output[d][O_EMERGENCE_LP] = m_output[d][O_EMERGENCE_LP] - m_output[d - 1][O_EMERGENCE_LP];
					m_output[d][O_EMERGENCE_LA_G2] = m_output[d][O_EMERGENCE_LA_G2] - m_output[d - 1][O_EMERGENCE_LA_G2];

				}
			}

		}


		return msg;
	}


	void CLeucopisModel::GetCDD(const CWeatherYears& weather, array<CModelStatVector, NB_SPECIES>& CDD)
	{
		_ASSERTE(int(m_P[delta]) >= 0);

		for (size_t s = 0; s < NB_SPECIES; s++)
		{
			CDegreeDays DDmodel(DD_METHOD, m_P[s * NB_CDD_PARAMS + Τᴴ¹], m_P[s * NB_CDD_PARAMS + Τᴴ²]);
			DDmodel.GetCDD(m_P[s * NB_CDD_PARAMS + delta], weather, CDD[s]);
		}
	}

	
	enum TInput { I_SYC, I_SITE, I_YEAR, I_COLLECTION, I_SPECIES, I_G, I_DATE, I_CDD, I_DAILY_COUNT, NB_INPUTS };
	enum TInputInternal { I_S, I_N, NB_INPUTS_INTERNAL };

	void CLeucopisModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);
		//SYC	site	Year	collection	species	G	emerge_date	daily_count

		CSAResult obs;

		CStatistic egg_creation_date;

		obs.m_ref.FromFormatedString(data[I_DATE]);
		obs.m_obs.resize(NB_INPUTS_INTERNAL);
		obs.m_obs[I_N] = stod(data[I_DAILY_COUNT]);


		if (data[I_SPECIES] == "La" && data[I_G] == "1")
			obs.m_obs[I_S] = S_LA_G1;
		else if (data[I_SPECIES] == "Lp")
			obs.m_obs[I_S] = S_LP;
		else if (data[I_SPECIES] == "La" && data[I_G] == "2")
			obs.m_obs[I_S] = S_LA_G2;


		m_SAResult.push_back(obs);
	}

	bool CLeucopisModel::IsParamValid()const
	{
		bool bValid = true;

		for (size_t s = 0; s < NB_SPECIES && bValid; s++)
		{
			if (m_P[s * NB_CDD_PARAMS + Τᴴ¹] >= m_P[s * NB_CDD_PARAMS + Τᴴ²])
				bValid = false;

			if (m_P[s * NB_CDD_PARAMS + Τᴴ²] - m_P[s * NB_CDD_PARAMS + Τᴴ¹] < 12.5)
				bValid = false;
		}

		return bValid;
	}




	enum TPout { P_CDD, P_CUMUL_EMERG, NB_P_OUT, NB_P = CLeucopisModel::NB_SPECIES * NB_P_OUT };
	void CLeucopisModel::GetPobs(CModelStatVector& P)
	{
		string ID = GetInfo().m_loc.m_ID;
		string SY = ID.substr(0, ID.length() - 2);

		//compute CDD for all temperature profile
		array< double, NB_SPECIES> total = { 0 };
		vector<tuple<double, CTRef, double, bool, size_t>> d;
		const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();

		for (size_t i = 0; i < SA.size(); i++)
		{
			string IDi = SA[i]->GetInfo().m_loc.m_ID;
			string SYi = IDi.substr(0, IDi.length() - 2);
			if (SYi == SY)
			{
				array<CModelStatVector, NB_SPECIES> CDD;
				GetCDD(SA[i]->m_weather, CDD);
				const CSAResultVector& v = SA[i]->GetSAResult();
				for (size_t ii = 0; ii < v.size(); ii++)
				{
					size_t s = v[ii].m_obs[I_S];
					CTRef TRef = v[ii].m_ref;

					d.push_back(make_tuple(CDD[s][TRef][0], TRef, v[ii].m_obs[I_N], IDi == ID, s));
					total[s] += v[ii].m_obs[I_N];
				}
			}
		}

		sort(d.begin(), d.end());

		P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_P, -999);
		array< double, NB_SPECIES> sum = { 0 };
		for (size_t i = 0; i < d.size(); i++)
		{
			size_t s = std::get<4>(d[i]);
			sum[s] += std::get<2>(d[i]);
			if (std::get<3>(d[i]))
			{
				double CDD = std::get<0>(d[i]);
				CTRef Tref = std::get<1>(d[i]);
				double p = Round(100 * sum[s] / total[s], 1);

				P[Tref][NB_P_OUT * s + P_CDD] = CDD;
				P[Tref][NB_P_OUT * s + P_CUMUL_EMERG] = p;
			}
		}
	}

	void CLeucopisModel::CalibrateEmergence(CStatisticXY& stat)
	{
		if (m_SAResult.empty())
			return; 

		//double n = 0;

		//for (size_t i = 0; i < m_SAResult.size(); i++)
			//n += m_SAResult[i].m_obs[I_N];


		CModelStatVector P;
		GetPobs(P);




		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			size_t s = m_SAResult[i].m_obs[I_S];//s is compute at new run
			CTRef TRef = m_SAResult[i].m_ref;

			double mu = m_P[s * NB_CDD_PARAMS + μ];
			double S = m_P[s * NB_CDD_PARAMS + ѕ];

			//Theoretical curve
			boost::math::logistic_distribution<double> emerge_dist(mu, S);

			double CDD = P[TRef][s * NB_P_OUT + P_CDD];
			double obs = P[TRef][s * NB_P_OUT + P_CUMUL_EMERG];
			ASSERT(obs >= 0 && obs <= 100);

			double sim = Round(100 * cdf(emerge_dist, max(0.0, CDD)), 1);

			for (size_t ii = 0; ii < m_SAResult[i].m_obs[I_N]; ii++)
				stat.Add(obs, sim);

		}//for all results

		return;

	}


	bool CLeucopisModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;

		CalibrateEmergence(stat);
		return true;

	}
}
