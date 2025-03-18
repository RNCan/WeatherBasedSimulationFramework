//***********************************************************
// 13/03/2025	1.1.0	Rémi Saint-Amant   New ophenology based on Dietschler 2025, La pass winter in larval stage
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
		VERSION = "1.1.0 (2025)";


		m_bApplyAttrition = false;
		m_bCumul = false;

		//set with default values
		m_adult_emerg = CLeucotaraxisPiniperdaEquations::ADULT_EMERG;
		m_pupa_param = CLeucotaraxisPiniperdaEquations::PUPA_PARAM;
		m_C_param = CLeucotaraxisPiniperdaEquations::C_PARAM;

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

		if (parameters.size() == 2 + NB_EMERGENCE_PARAMS + NB_PUPA_PARAMS + NB_C_PARAMS)
		{
			for (size_t p = 0; p < NB_EMERGENCE_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
				m_pupa_param[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_C_PARAMS; p++)
				m_C_param[p] = parameters[c++].GetFloat();

			
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

	void CLeucotaraxisPiniperdaModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output, bool in_calibration)
	{
		//Create stand and init it
		CLPMStand stand(this);
		stand.m_bApplyAttrition = m_bApplyAttrition;
		stand.m_equations.m_adult_emerg = m_adult_emerg;
		stand.m_equations.m_pupa_param = m_pupa_param;
		stand.m_equations.m_C_param = m_C_param;
		stand.m_in_calibration = in_calibration;
		stand.init(year, weather);


		//Create host
		CLPMHostPtr pHost(new CLPMHost(&stand));

		//pHost->m_nbMinObjects = 10;
		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		//pHost->Initialize<CLeucotaraxisPiniperda>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, PUPAE));
		pHost->Initialize<CLeucotaraxisPiniperda>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, LARVAE+ m_C_param[0]));

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
	enum TInput { I_SYC, I_SITE, I_YEAR, I_COLLECTION, I_SPECIES, I_G, I_DATE, I_CDD, I_TMIN, I_N, I_P, NB_INPUTS };
	enum TInputInternal { O_S, O_N, O_P, NB_INPUTS_INTERNAL };
	void CLeucotaraxisPiniperdaModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		CStatistic egg_creation_date;
		//|| data[I_SYC] == "TAC_2020_3"|| data[I_SYC] == "TAC_2020_4"
		if (data[I_SPECIES] == "Lp" )
		{
			obs.m_ref.FromFormatedString(data[I_DATE]);
			obs.m_obs.resize(NB_INPUTS_INTERNAL);
			obs.m_obs[O_S] = S_LP;
			obs.m_obs[O_N] = stod(data[I_N]);
			obs.m_obs[O_P] = stod(data[I_P]);


			ASSERT(obs.m_obs[I_N] >= 0);
			ASSERT(obs.m_obs[I_P] >= 0 && obs.m_obs[I_P]<=100);
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

	enum TPout { P_CDD, P_CE, LA_G1 = P_CE, P_LA_G2, P_LP, P_LN, NB_P };//CE = cumulative emergence
	void CLeucotaraxisPiniperdaModel::GetPobs(CModelStatVector& P)
	{
		//string ID = GetInfo().m_loc.m_ID;
		//string SY = ID.substr(0, ID.length() - 2);
		//
		////compute CDD for all temperature profile
		//array< double, 4> total = { 0 };
		//vector<tuple<double, CTRef, double, bool, size_t>> d;
		//const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();
		//
		//for (size_t i = 0; i < SA.size(); i++)
		//{
		//	string IDi = SA[i]->GetInfo().m_loc.m_ID;
		//	string SYi = IDi.substr(0, IDi.length() - 2);
		//	if (SYi == SY)
		//	{
		//		CModelStatVector CDD;
		//
		//		//degree day of the La g2 
		//		CDegreeDays DDmodel(CDegreeDays::ALLEN_WAVE, m_adult_emerg[Τᴴ¹], m_adult_emerg[Τᴴ²]);
		//		DDmodel.GetCDD(int(m_adult_emerg[delta]), m_weather, CDD);
		//
		//		const CSAResultVector& v = SA[i]->GetSAResult();
		//		for (size_t ii = 0; ii < v.size(); ii++)
		//		{
		//			d.push_back(make_tuple(CDD[v[ii].m_ref][0], v[ii].m_ref, v[ii].m_obs[I_N], IDi == ID, v[ii].m_obs[O_S]));
		//			total[v[ii].m_obs[O_S]] += v[ii].m_obs[O_N];
		//		}
		//	}
		//}
		//
		//sort(d.begin(), d.end());
		//
		//P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_P, 0);
		//array< double, 4> sum = { 0 };
		//for (size_t i = 0; i < d.size(); i++)
		//{
		//	size_t s = std::get<4>(d[i]);
		//	sum[s] += std::get<2>(d[i]);
		//	if (std::get<3>(d[i]))
		//	{
		//		CTRef Tref = std::get<1>(d[i]);
		//		double CDD = std::get<0>(d[i]);
		//		double p = 100 * sum[s] / total[s];
		//
		//		P[Tref][P_CDD] = CDD;
		//		P[Tref][P_CE + s] = p;
		//	}
		//}
	}

	bool CLeucotaraxisPiniperdaModel::CalibratePupa(CStatisticXY& stat)
	{

		if (!m_SAResult.empty())
		{
			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();



			m_bCumul = true;//SA always cumulative
			//Always used the same seed for calibration
			m_randomGenerator.Randomize(CRandomGenerator::FIXE_SEED);



			//if (m_SAResult.back().m_obs.size() == NB_INPUTS_INTERNAL)
			//{
			//	CModelStatVector P;
			//	GetPobs(P);
			//
			//	for (size_t i = 0; i < m_SAResult.size(); i++)
			//	{
			//		double cumul_obs = P[m_SAResult[i].m_ref][P_LP];
			//		ASSERT(cumul_obs >= 0 && cumul_obs <= 100);
			//
			//		m_SAResult[i].m_obs[O_P] = cumul_obs;
			//	}
			//}


			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();

				CModelStatVector output;
				CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

				output.Init(p, NB_STATS, 0);
				ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output, true);


				for (size_t i = 0; i < m_SAResult.size(); i++)
				{
					if (output.IsInside(m_SAResult[i].m_ref))
					{

						double obs_y = Round(m_SAResult[i].m_obs[O_P], 4);
						double sim_y = Round(output[m_SAResult[i].m_ref][S_EMERGENCE0], 4);

						if (obs_y > -999)
						{
							for (size_t ii = 0; ii < m_SAResult[i].m_obs[O_N]; ii++)
								stat.Add(obs_y, sim_y);
						}
					}
				}//for all results

			}
		}

		return true;
	}


	bool CLeucotaraxisPiniperdaModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;

		return CalibratePupa(stat);
	}


}
