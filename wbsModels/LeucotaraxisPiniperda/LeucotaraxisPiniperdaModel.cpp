//***********************************************************
// 07/11/2024	1.0.2	Rémi Saint-Amant   Final calibration for publication
// 18/10/2022	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LeucotaraxisPiniperdaModel.h"
#include "LeucotaraxisPiniperdaEquations.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include <boost/math/distributions/logistic.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LPM;
using namespace std;


namespace WBSF
{

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLeucotaraxisPiniperdaModel::CreateObject);

	CLeucotaraxisPiniperdaModel::CLeucotaraxisPiniperdaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.2 (2024)";


		m_bApplyAttrition = false;
		m_bCumul = false;

		for (size_t p = 0; p < LPM::NB_EMERGENCE_PARAMS; p++)
			m_adult_emerg[p] = CLeucotaraxisPiniperdaEquations::ADULT_EMERG[p];


		for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
			m_pupa_param[p] = CLeucotaraxisPiniperdaEquations::PUPA_PARAM[p];


		for (size_t p = 0; p < NB_C_PARAMS; p++)
			m_C_param[p] = CLeucotaraxisPiniperdaEquations::C_PARAM[p];

	}

	CLeucotaraxisPiniperdaModel::~CLeucotaraxisPiniperdaModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLeucotaraxisPiniperdaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bApplyAttrition = parameters[c++].GetBool();
		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 2 + LPM::NB_EMERGENCE_PARAMS + NB_PUPA_PARAMS + NB_C_PARAMS)
		{
			for (size_t p = 0; p < LPM::NB_EMERGENCE_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
				m_pupa_param[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_C_PARAMS; p++)
				m_C_param[p] = parameters[c++].GetFloat();

			//Always used the same seed for calibration
		//	m_randomGenerator.Randomize(CRandomGenerator::FIXE_SEED);
		}


		return msg;
	}





	//This method is called to compute the solution
	ERMsg CLeucotaraxisPiniperdaModel::OnExecuteDaily()
	{
		ERMsg msg;


		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//For all years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CLeucotaraxisPiniperdaModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLPMStand stand(this);

		stand.m_bApplyAttrition = m_bApplyAttrition;
		//Set parameters to equation
		for (size_t p = 0; p < LPM::NB_EMERGENCE_PARAMS; p++)
			stand.m_equations.m_adult_emerg[p] = m_adult_emerg[p];

		for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
			stand.m_equations.m_pupa_param[p] = m_pupa_param[p];

		for (size_t p = 0; p < NB_C_PARAMS; p++)
			stand.m_equations.m_C_param[p] = m_C_param[p];

		stand.init(year, weather);

		//Create host
		CLPMHostPtr pHost(new CLPMHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CLeucotaraxisPiniperda>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, PUPAE));

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


			//cumulative result
			for (size_t ss = 0; ss < NB_CUMUL_STATS; ss++)
			{
				size_t s = CUM_STAT[ss];


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

	enum TSpecies { S_LA_G1, S_LA_G2, S_LP, S_LN };
	enum TInput { I_SYC, I_SITE, I_YEAR, I_COLLECTION, I_SPECIES, I_G, I_DATE, I_CDD, I_DAILY_COUNT, NB_INPUTS };
	enum TInputInternal { I_S, I_N, NB_INPUTS_INTERNAL };
	void CLeucotaraxisPiniperdaModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		CStatistic egg_creation_date;

		if (data[I_SPECIES] == "La" && data[I_G] == "2")
		{
			obs.m_ref.FromFormatedString(data[I_DATE]);
			obs.m_obs.resize(NB_INPUTS_INTERNAL);
			obs.m_obs[I_S] = S_LA_G2;
			obs.m_obs[I_N] = stod(data[I_DAILY_COUNT]);


			ASSERT(obs.m_obs[I_N] >= 0);
			m_SAResult.push_back(obs);
		}





	}

	bool CLeucotaraxisPiniperdaModel::IsParamValid()const
	{
		bool bValid = true;


		if (m_adult_emerg[Τᴴ¹] >= m_adult_emerg[Τᴴ²])
			bValid = false;

		return bValid;
	}


	enum TPout { P_CDD, P_CE, LA_G1 = P_CE, P_LA_G2, P_LP, NB_P };//CE = cumulative emergence
	void CLeucotaraxisPiniperdaModel::GetPobs(CModelStatVector& P)
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

				//degree day of the Lp
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


	bool CLeucotaraxisPiniperdaModel::CalibrateEmergenceG2(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{

			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();


			m_bCumul = true;//SA always cumulative

			//Always used the same seed for calibration
			m_randomGenerator.Randomize(CRandomGenerator::FIXE_SEED);


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

			}
		}

		return true;
	}


	bool CLeucotaraxisPiniperdaModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;

		return CalibrateEmergenceG2(stat);
	}


}
