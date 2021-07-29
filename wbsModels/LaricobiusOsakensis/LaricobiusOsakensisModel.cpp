//***********************************************************
// 07/07/2021	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "LaricobiusOsakensisModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/non_central_f.hpp>
#include <boost/math/distributions/extreme_value.hpp>
#include <boost/math/distributions/fisher_f.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LOF;
using namespace std;

static const bool BEGIN_DECEMBER = true;
static const size_t FIRST_Y = BEGIN_DECEMBER ? 1 : 0;

namespace WBSF
{
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLaricobiusOsakensisModel::CreateObject);

	CLaricobiusOsakensisModel::CLaricobiusOsakensisModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2021)";


		m_bApplyAttrition = false;
		m_bCumul = false;

		for (size_t p = 0; p < NB_CEC_PARAMS; p++)
			m_CEC[p] = CLaricobiusOsakensisEquations::CEC[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			m_ADE[p] = CLaricobiusOsakensisEquations::ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			m_EAS[p] = CLaricobiusOsakensisEquations::EAS[p];
	}

	CLaricobiusOsakensisModel::~CLaricobiusOsakensisModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLaricobiusOsakensisModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bApplyAttrition = parameters[c++].GetBool();
		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 2 + NB_CEC_PARAMS + NB_ADE_PARAMS + NB_EAS_PARAMS)
		{
			for (size_t p = 0; p < NB_CEC_PARAMS; p++)
				m_CEC[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_ADE_PARAMS; p++)
				m_ADE[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_EAS_PARAMS; p++)
				m_EAS[p] = parameters[c++].GetFloat();

		}

		return msg;
	}





	//This method is called to compute the solution
	ERMsg CLaricobiusOsakensisModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (m_weather.GetNbYears() < 2)
		{
			msg.ajoute("Laricobius Osakensis model need at least 2 years of data");
			return msg;
		}

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		//CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		//m_output.Init(p, NB_STATS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = FIRST_Y; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CLaricobiusOsakensisModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLNFStand stand(this, m_CEC[Τᴴ¹], m_CEC[Τᴴ²]);

		stand.m_bApplyAttrition = m_bApplyAttrition;
		//Set parameters to equation
		for (size_t p = 0; p < NB_CEC_PARAMS; p++)
			stand.m_equations.m_CEC[p] = m_CEC[p];

		for (size_t p = 0; p < NB_ADE_PARAMS; p++)
			stand.m_equations.m_ADE[p] = m_ADE[p];

		for (size_t p = 0; p < NB_EAS_PARAMS; p++)
			stand.m_equations.m_EAS[p] = m_EAS[p];


		stand.init(year, weather);

		//Create host
		CLNFHostPtr pHost(new CLNFHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		CTRef begin = BEGIN_DECEMBER ? CTRef(year-1, SEPTEMBER, DAY_01) : CTRef(year, JANUARY, DAY_01);
		CTRef end = BEGIN_DECEMBER ? CTRef(year, AUGUST, DAY_31) : CTRef(year, DECEMBER, DAY_31);
		pHost->Initialize<CLaricobiusOsakensis>(CInitialPopulation(begin, 0, 400, 100, -1));

		//add host to stand			
		stand.m_host.push_front(pHost);

		//CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		CTPeriod p(begin, end);
		output.Init(p, NB_STATS, 0);

		//if have other year extend period to February
		ASSERT(weather[year].HavePrevious());
		//if (BEGIN_DECEMBER)
		//{
		//	p.Begin() = CTRef(year - 1, JULY, DAY_01);
		//}
		//else
		//{
		//	//if have other year extend period to February
		//	if (weather[year].HaveNext())
		//		p.End() = CTRef(year + 1, JUNE, DAY_30);

		//}
		//if have previous year extend period to DECEMBER
		//if (weather[year].HavePrevious())
			//p.Begin() = CTRef(year - 1, DECEMBER, DAY_01);

		



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
			for (size_t s = S_EGG; s < S_ACTIVE_ADULT; s++)
			{
				//CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

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

			CStatistic stat = output.GetStat(S_LARVAE, p);
			if (stat.IsInit() && stat[SUM] > 0)
			{
				output[0][S_LARVAE] = output[0][S_LARVAE] * 100 / stat[SUM];//when first day is not 0
				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
				{
					output[d][S_LARVAE] = output[d - 1][S_LARVAE] + output[d][S_LARVAE] * 100 / stat[SUM];
					_ASSERTE(!_isnan(output[d][S_LARVAE]));
				}
			}
		}
	}


	void CLaricobiusOsakensisModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 4);

		CSAResult obs;

		CStatistic egg_creation_date;

		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t i = 0; i < NB_INPUTS; i++)
		{
			obs.m_obs[i] = stod(data[i + 2]);

			if (i == 0 && obs.m_obs[i] > -999)
				m_egg_creation_date[data[0] + "_" + to_string(obs.m_ref.GetYear())] += obs.m_ref.GetJDay();

			if (obs.m_obs[i] > -999)
			{
				m_nb_days[i] += obs.m_ref.GetJDay();
				m_years[i].insert(obs.m_ref.GetYear());
			}
		}

		m_SAResult.push_back(obs);


	}

	double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		double x = -999;

		if (obs > -999)
		{
			//if (obs > 0.01 && obs < 99.99)
			if (obs >= 100)
				obs = 99.99;//to avoid some problem of truncation

			long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear()-1, DECEMBER, DAY_01, TRefO.GetYear(), DECEMBER, DAY_31));
			if (index >= 1)
			{
				double obsX1 = output.GetFirstTRef().GetJDay() + index;
				double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

				double obsY1 = output[index][s];
				double obsY2 = output[index + 1][s];
				if (obsY2 != obsY1)
				{
					double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
					double obsX = obsX1 + (obs - obsY1)*slope;
					ASSERT(!_isnan(obsX) && _finite(obsX));

					x = obsX;
				}
			}
		}

		return x;
	}

	bool CLaricobiusOsakensisModel::IsParamValid()const
	{
		if (m_CEC[Τᴴ¹] >= m_CEC[Τᴴ²])
			return false;


		bool bValid = true;

		return bValid;
	}



	static const int ROUND_VAL = 4;

	//void CLaricobiusOsakensisModel::CalibrateCumulativeEggCreation(CStatisticXY& stat)
	//{
	//	size_t EVALUATE_STAGE = 0;

	//	if (m_CEC[Τᴴ¹] >= m_CEC[Τᴴ²])
	//		return;


	//	if (m_SAResult.empty())
	//		return;

	//	if (!m_weather.IsHourly())
	//		m_weather.ComputeHourlyVariables();



	//	for (size_t y = 0; y < m_weather.GetNbYears(); y++)
	//	{
	//		int year = m_weather[y].GetTRef().GetYear();
	//		if (m_years[EVALUATE_STAGE].find(year) == m_years[EVALUATE_STAGE].end())
	//			continue;



	//		double sumDD = 0;
	//		vector<double> CDD;
	//		CTPeriod p;


	//		p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
	//		CDD.resize(p.size(), 0);

	//		CDegreeDays DDModel(CDegreeDays::MODIFIED_ALLEN_WAVE, m_CEC[Τᴴ¹], m_CEC[Τᴴ²]);

	//		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	//		{
	//			const CWeatherDay& wday = m_weather.GetDay(TRef);
	//			size_t ii = TRef - p.Begin();
	//			sumDD += DDModel.GetDD(wday);
	//			CDD[ii] = sumDD;
	//		}

	//		for (size_t i = 0; i < m_SAResult.size(); i++)
	//		{
	//			size_t ii = m_SAResult[i].m_ref - p.Begin();
	//			if (m_SAResult[i].m_ref.GetYear() == year && ii < CDD.size())
	//			{
	//				double obs_y = m_SAResult[i].m_obs[EVALUATE_STAGE];

	//				if (obs_y > -999)
	//				{

	//					double sim_y = 0;

	//					/*if (EVALUATE_STAGE == I_EMERGED_ADULT)
	//					{
	//						boost::math::weibull_distribution<double> emerged_dist(m_EAS[ʎ], m_EAS[к]);
	//						sim_y = Round(cdf(emerged_dist, CDD[ii]) * 100, ROUND_VAL);
	//					}
	//					else
	//					{*/
	//					boost::math::logistic_distribution<double> create_dist(m_CEC[μ], m_CEC[ѕ]);
	//					sim_y = Round(cdf(create_dist, CDD[ii]) * 100, ROUND_VAL);
	//					//}


	//					if (sim_y < 0.1)
	//						sim_y = 0;
	//					if (sim_y > 99.9)
	//						sim_y = 100;

	//					stat.Add(obs_y, sim_y);
	//				}
	//			}
	//		}
	//	}//for all years




	//	return;
	//}

	void CLaricobiusOsakensisModel::CalibrateCumulativeEggCreation(CStatisticXY& stat)
	{

		if (m_CEC[Τᴴ¹] >= m_CEC[Τᴴ²])
			return;


		if (m_SAResult.empty())
			return;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();



		for (size_t y = FIRST_Y; y < m_weather.GetNbYears(); y++)
		{
			int year = m_weather[y].GetTRef().GetYear();
			//if (m_years[EVALUATE_STAGE].find(year) == m_years[EVALUATE_STAGE].end())
				//continue;



			double sumDD = 0;
			vector<double> CDD;
			CTPeriod p;


			p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
			if(BEGIN_DECEMBER)
				p.Begin() = CTRef(year - 1, DECEMBER, DAY_01);

			CDD.resize(p.size(), 0);

			CDegreeDays DDModel(CDegreeDays::MODIFIED_ALLEN_WAVE, m_CEC[Τᴴ¹], m_CEC[Τᴴ²]);

			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				const CWeatherDay& wday = m_weather.GetDay(TRef);
				size_t ii = TRef - p.Begin();
				sumDD += DDModel.GetDD(wday);
				CDD[ii] = sumDD;
			}

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				size_t ii = m_SAResult[i].m_ref - p.Begin();
				if (p.IsInside(m_SAResult[i].m_ref) && ii < CDD.size())
				{
					double obs_y = m_SAResult[i].m_obs[I_EGGS];

					if (obs_y > -999)
					{

						double sim_y = 0;
						//boost::math::logistic_distribution<double> create_dist(m_CEC[μ], m_CEC[ѕ]);
						boost::math::weibull_distribution<double> create_dist(m_CEC[μ], m_CEC[ѕ]);
						//boost::math::non_central_f_distribution<double> create_dist( m_CEC[μ], m_CEC[ѕ], m_CEC[ʎf]);

						sim_y = Round(cdf(create_dist, CDD[ii]) * 100, ROUND_VAL);
						

						if (sim_y < 0.1)
							sim_y = 0;
						if (sim_y > 99.9)
							sim_y = 100;

						stat.Add(obs_y, sim_y);
					}
				}
			}
		}//for all years




		return;
	}



	void CLaricobiusOsakensisModel::GetFValueDaily(CStatisticXY& stat)
	{
		//return CalibrateCumulativeEggCreation(stat);


		if (!m_SAResult.empty())
		{
			if (!m_bCumul)
				m_bCumul = true;//SA always cumulative

			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();

			//low and hi relative development rate must be approximatively the same
			//if (!IsParamValid())
				//return;



			for (size_t y = FIRST_Y; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if (m_years[I_EGGS].find(year) != m_years[I_EGGS].end())
				{

					CModelStatVector output;
					CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
//					p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
					if (BEGIN_DECEMBER)
						p.Begin() = CTRef(year - 1, DECEMBER, DAY_01);

					//not possible to add a second year without having problem in evaluation....
					//if (m_weather[y].HaveNext())
						//p.End() = m_weather[y + 1].GetEntireTPeriod(CTM(CTM::DAILY)).End();

					output.Init(p, NB_STATS, 0);
					ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);

					static const std::array<bool, NB_INPUTS> TEST_STAGES = { {true, true} };
					static const size_t STAT_STAGE[NB_INPUTS] = { S_EGG, S_LARVAE };

					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						if (output.IsInside(m_SAResult[i].m_ref))
						{
							for (size_t j = 0; j < NB_INPUTS; j++)
							{
								if (TEST_STAGES[j])
								{
									double obs_y = Round(m_SAResult[i].m_obs[j], ROUND_VAL);
									double sim_y = Round(output[m_SAResult[i].m_ref][STAT_STAGE[j]], ROUND_VAL);

									if (obs_y > -999)
									{
										stat.Add(obs_y, sim_y);

										//double obs_x = m_SAResult[i].m_ref.GetJDay();
										//double sim_x = GetSimX(STAT_STAGE[j], m_SAResult[i].m_ref, obs_y, output);

										/*if (sim_x > -999)
										{
											obs_x = Round(100 * (obs_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE],ROUND_VAL);
											sim_x = Round(100 * (sim_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE],ROUND_VAL);
											stat.Add(obs_x, sim_x);
										}*/
									}
								}
							}
						}
					}//for all results
				}//have data
			}
		}
	}
}
