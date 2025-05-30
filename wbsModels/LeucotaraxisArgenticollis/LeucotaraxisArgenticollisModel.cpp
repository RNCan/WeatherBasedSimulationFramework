//***********************************************************
// 2024/11/01	1.0.3	Rémi Saint-Amant   Add CDDg1
// 2024/07/01	1.0.2	Rémi Saint-Amant   Add Tjan(Tmin)
// 2022/10/18	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LeucotaraxisArgenticollisModel.h"
#include "LeucotaraxisArgenticollisEquations.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/SimulatedAnnealingVector.h"
#include <boost/math/distributions/weibull.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LAZ;
using namespace std;


namespace WBSF
{
	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLeucotaraxisArgenticollisModel::CreateObject);

	CLeucotaraxisArgenticollisModel::CLeucotaraxisArgenticollisModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.1.0 (2025)";


		m_bApplyAttrition = false;
		m_bCumul = false;

		
		m_adult_emerg = CLeucotaraxisArgenticollisEquations::ADULT_EMERG;
		m_pupa_param = CLeucotaraxisArgenticollisEquations::PUPA_PARAM;
		m_C_param = CLeucotaraxisArgenticollisEquations::C_PARAM;
		m_EOD_param = CLeucotaraxisArgenticollisEquations::EOD_PARAM;
	}

	CLeucotaraxisArgenticollisModel::~CLeucotaraxisArgenticollisModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLeucotaraxisArgenticollisModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bApplyAttrition = parameters[c++].GetBool();
		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 2 + LAZ::NB_EMERGENCE_PARAMS + NB_PUPA_PARAMS + NB_C_PARAMS + NB_EOD_PARAMS)
		{
			for (size_t p = 0; p < LAZ::NB_EMERGENCE_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PUPA_PARAMS; p++)
				m_pupa_param[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_C_PARAMS; p++)
				m_C_param[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_EOD_PARAMS; p++)
				m_EOD_param[p] = parameters[c++].GetFloat();
			

			//m_pupa_param = { {0.0196,	0.0530, 3.1,	34.9,	34.9,	1.6836, 0.3828} };
		}


		return msg;
	}





	//This method is called to compute the solution
	ERMsg CLeucotaraxisArgenticollisModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CLeucotaraxisArgenticollisModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLAZStand stand(this);
		stand.CALIBRATE_PUPAE_AND_EMERGENCE_G2 = this->m_info.m_modelName == "LeucotaraxisArgenticollis(param)";

		stand.m_bApplyAttrition = m_bApplyAttrition;
		//Set parameters to equation
		stand.m_equations.m_adult_emerg = m_adult_emerg;
		stand.m_equations.m_pupa_param = m_pupa_param;
		stand.m_equations.m_C_param = m_C_param;
		stand.m_equations.m_EOD_param = m_EOD_param;

		stand.init(year, weather);

		//Create host
		CLAZHostPtr pHost(new CLAZHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CLeucotaraxisArgenticollis>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, PUPAE));

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

			for (size_t s = 0; s < NB_STATS; s++)
			{
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
	enum TInput { I_SYC, I_SITE, I_YEAR, I_COLLECTION, I_SPECIES, I_G, I_DATE, I_CDD, I_TMIN, I_DAILY_COUNT, I_P, NB_INPUTS };
	enum TInputInternal { O_G, O_N, O_P, NB_INPUTS_INTERNAL };
	void CLeucotaraxisArgenticollisModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		CStatistic egg_creation_date;

		if (data[I_SPECIES] == "La" )
		{
			obs.m_ref.FromFormatedString(data[I_DATE]);
			obs.m_obs.resize(NB_INPUTS_INTERNAL);
			obs.m_obs[O_G] = stod(data[I_G]);
			obs.m_obs[O_N] = stod(data[I_DAILY_COUNT]);
			obs.m_obs[O_P] = stod(data[I_P]);


			ASSERT(obs.m_obs[O_N] >= 0);
			ASSERT(obs.m_obs[O_P] >= 0 && obs.m_obs[O_P] <= 100);
			m_SAResult.push_back(obs);

			if (obs.m_obs[O_P] >= 5 && obs.m_obs[O_P] <= 95)
			{
				m_DOY[obs.m_obs[O_G]].insert(obs.m_ref.GetJDay());
			}
		}
		





	}

	double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		double x = -999;

		if (obs > -999)
		{
			//if (obs > 0.01 && obs < 99.99)
			if (obs >= 100)
				obs = 99.99;//to avoid some problem of truncation

			long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), JANUARY, DAY_01, TRefO.GetYear(), DECEMBER, DAY_31));
			if (index >= 1)
			{
				double obsX1 = output.GetFirstTRef().GetJDay() + index;
				double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

				double obsY1 = output[index][s];
				double obsY2 = output[index + 1][s];
				if (obsY2 != obsY1)
				{
					double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
					double obsX = obsX1 + (obs - obsY1) * slope;
					ASSERT(!_isnan(obsX) && _finite(obsX));

					x = obsX;
				}
			}
		}

		return x;
	}

	bool CLeucotaraxisArgenticollisModel::IsParamValid()const
	{
		bool bValid = true;


		if (m_adult_emerg[Τᴴ¹] >= m_adult_emerg[Τᴴ²])
			bValid = false;

		return bValid;
	}

	enum TPout { P_CDD, P_CE, LA_G1 = P_CE, P_LA_G2, P_LP, P_LN, NB_P };//CE = cumulative emergence
	void CLeucotaraxisArgenticollisModel::GetPobs(CModelStatVector& P)
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
		//			d.push_back(make_tuple(CDD[v[ii].m_ref][0], v[ii].m_ref, v[ii].m_obs[I_N], IDi == ID, v[ii].m_obs[I_S]));
		//			total[v[ii].m_obs[I_S]] += v[ii].m_obs[I_N];
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
		//		double p = Round(100 * sum[s] / total[s], 1);
		//
		//		P[Tref][P_CDD] = CDD;
		//		P[Tref][P_CE + s] = p;
		//	}
		//}
	}

	bool CLeucotaraxisArgenticollisModel::CalibratePupaWithoutDiapause(CStatisticXY& stat)
	{
		bool bSuccess = true;
		if (!m_SAResult.empty())
		{
			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();



			m_bCumul = true;//SA always cumulative
			//Always used the same seed for calibration
			m_randomGenerator.Randomize(CRandomGenerator::FIXE_SEED);



			//if (m_SAResult.back().m_obs.size() == NB_INPUTS_INTERNAL)
			//{
			//	
			//
			//
			//	CModelStatVector P;
			//	GetPobs(P);
			//
			//	for (size_t i = 0; i < m_SAResult.size(); i++)
			//	{
			//		double cumul_obs = P[m_SAResult[i].m_ref][P_LA_G2];
			//		ASSERT(cumul_obs >= 0 && cumul_obs <= 100);
			//
			//		m_SAResult[i].m_obs.push_back(cumul_obs);
			//	}
			//
			//
			//}


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
						size_t G = m_SAResult[i].m_obs[O_G];
						size_t stage =  G == 0 ? S_EMERGENCE0 : S_EMERGENCE1a;
						double obs_y = Round(m_SAResult[i].m_obs[O_P], 4);
						double sim_y = Round(output[m_SAResult[i].m_ref][stage], 4);
						

						if (obs_y > -999)
						{
							//for (size_t ii = 0; ii < m_SAResult[i].m_obs[O_N]; ii++)
								stat.Add(obs_y, sim_y);


							if (obs_y >= 5 && obs_y<= 95)
							{
								double obs_x = m_SAResult[i].m_ref.GetJDay();
								double sim_x = GetSimX(stage, m_SAResult[i].m_ref, obs_y, output);

								if (sim_x > -999)
								{
									obs_x = Round(100 * (obs_x - m_DOY[G][LOWEST]) / m_DOY[G][RANGE], 4);
									sim_x = Round(100 * (sim_x - m_DOY[G][LOWEST]) / m_DOY[G][RANGE], 4);
									
									//for (size_t ii = 0; ii < m_SAResult[i].m_obs[O_N]; ii++)
										stat.Add(obs_x, sim_x);
								}
								else
								{
									bSuccess = false;
								}
							}
						}
						else
						{
							bSuccess = false;
						}
					}
				}//for all results

			}
		}

		return bSuccess;// stat[NB_VALUE] == (m_SAResult.size() + m_DOY[0][NB_VALUE] + m_DOY[1][NB_VALUE]);
	}


	bool CLeucotaraxisArgenticollisModel::GetFValueDaily(CStatisticXY& stat)
	{
		//if (!IsParamValid())
			//return false;

		//double m = m_C_param[C_P0];//0.9 - 0.8 * boost::math::cdf(age_dist, Tdelta);
		//double s = m_C_param[C_P3];
		//
		//double a = m * (m * (1 - m) / (s * s) - 1);
		//double b = (1 - m) * (m * (1 - m) / (s * s) - 1);
		//
		//if (a <= 0 || a>=1 || b <= 0 || b >= 1)
		//	return false;


		return CalibratePupaWithoutDiapause(stat);
	}


}
