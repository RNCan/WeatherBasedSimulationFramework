//***********************************************************
// 2016-05-06   1.2.0	Rémi Saint-Amant	Bug correction in attrition 
//											Clean up for publication
//											Add annual model for 10 days sampling optimum
//											Add survival and fecundity in lookup table
// 2025-03-27	1.1.0	Rémi Saint-Amant	Update for article
// 2019-10-15   1.0.0	Rémi Saint-Amant	Creation
//***********************************************************




#include "LaricobiusNigrinusModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LNF;
using namespace std;

namespace WBSF
{

	const std::array<size_t, CLaricobiusNigrinusModel::NB_EVALUATED_STAGES> CLaricobiusNigrinusModel::STAT_STAGE = { S_EGGS, S_LARVAE, S_LARVAL_DROP, S_ACTIVE_ADULTS };



	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	//enum { ACTUAL_CDD, DATE_DD717, DIFF_DAY, NB_OUTPUTS };

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CLaricobiusNigrinusModel::CreateObject);

	CLaricobiusNigrinusModel::CLaricobiusNigrinusModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.2.0 (2026)";

		m_bCumul = false;
		m_bApplyAttrition = true;
		m_OVP = CLaricobiusNigrinusEquations::OVP;
		m_ADE = CLaricobiusNigrinusEquations::ADE;
		m_EAS = CLaricobiusNigrinusEquations::EAS;

	}

	CLaricobiusNigrinusModel::~CLaricobiusNigrinusModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CLaricobiusNigrinusModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_bCumul = parameters[c++].GetBool();

		if (parameters.size() == 1 + NB_STAGES * NB_RDR_PARAMS + NB_OVP_PARAMS + NB_ADE_PARAMS + NB_EAS_PARAMS)
		{
			for (size_t s = 0; s < NB_STAGES; s++)
			{
				for (size_t p = 0; p < NB_RDR_PARAMS; p++)
				{
					parameters[c++].GetFloat();
				}
			}

			for (size_t p = 0; p < NB_OVP_PARAMS; p++)
				m_OVP[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_ADE_PARAMS; p++)
				m_ADE[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_EAS_PARAMS; p++)
				m_EAS[p] = parameters[c++].GetFloat();

			m_bApplyAttrition = false;
		}
		else
		{
			m_bApplyAttrition = parameters[c++].GetBool();
		}

		return msg;
	}




	//Annual model return the 10 days optimum
	ERMsg CLaricobiusNigrinusModel::OnExecuteAnnual()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM::ANNUAL);
		m_output.Init(p, NB_ANNUAL_OUTPUTS);

		
		for (size_t y = 0; y < m_weather.GetNbYears(); y++)
		{
			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			
			CModelStatVector output;
			output.Init(p, NB_STATS, 0);

			//take 50% larval emergence
			m_bCumul = true;
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);

			CTRef end = output.GetFirstTRef(S_LARVAE,">",50.0);

			m_output[y][AO_BEGIN] = (end-10).GetRef();
			m_output[y][AO_END] = end.GetRef();
		}

		return msg;
	}

	//This method is called to compute the solution
	ERMsg CLaricobiusNigrinusModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
		m_output.Init(p, NB_STATS, 0);

		//simulation overall years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CLaricobiusNigrinusModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLNFStand stand(this, m_OVP[Τᴴ¹], m_OVP[Τᴴ²]);
		stand.m_bApplyAttrition = m_bApplyAttrition;

		//Set parameters to equation
		stand.m_equations.m_OVP = m_OVP;
		stand.m_equations.m_ADE = m_ADE;
		stand.m_equations.m_EAS = m_EAS;


		stand.init(year, weather);

		//Create host
		CLNFHostPtr pHost(new CLNFHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CLaricobiusNigrinus>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, -1));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		//if have other year extend period to July
		if (weather[year].HaveNext())
			p.End() = CTRef(year + 1, JUNE, DAY_30);


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
			for (size_t ss = 0; ss < NB_CUMULATIVE_STATS; ss++)
			{
				CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

				size_t s = CUMULATIVE_STATS[ss];
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




	enum TInput { I_SOURCE, I_SITE, I_DATE, I_EGGS, I_LARVAE, I_LARVAL_DROP, I_EMERGING_ADULTS, I_ACTIVE_ADULTS, I_FEMALES, I_BROODS, I_TYPE, NB_INPUTS };


	void CLaricobiusNigrinusModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[I_DATE]);
		obs.m_obs.resize(NB_EVALUATED_STAGES);
		for (size_t i = 0; i < NB_EVALUATED_STAGES; i++)
		{
			if (data[I_EGGS + i] != "NA" && data[I_TYPE] == "C")
			{
				obs.m_obs[i] = stod(data[I_EGGS + i]);

				m_cumul_stats[i] += obs.m_obs[i];
				m_nb_days[i] += obs.m_ref.GetJDay();
				m_years[i].insert(obs.m_ref.GetYear());

			}
			else
			{
				obs.m_obs[i] = -999;
			}
		}

		m_SAResult.push_back(obs);
	}



	double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		double x = -999;

		if (obs > -999)
		{
			if (obs >= 100)
				obs = 99.99;//to avoid some problem of truncation

			long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
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

	bool CLaricobiusNigrinusModel::IsParamValid()const
	{
		bool bValid = true;


		if (m_OVP[Τᴴ¹] >= m_OVP[Τᴴ²])
			bValid = false;


		return bValid;
	}



	bool CLaricobiusNigrinusModel::CalibrateDiapauseEndTh(CStatisticXY& stat)
	{
		static const double DiapauseDuration[3][3] =
		{
			{128.1,127.9,134.0},
			{156.7,162.2,166.2},
			{194.8,203.7,-999}
		};

		static const double DiapauseDurationSD[3][3] =
		{
			{2.2,3,	3.8},
			{4.1,4.4,5.4},
			{4.2,3.4,10.8}
		};

		if (m_SAResult.size() != 8)
			return false;


		for (size_t t = 0; t < 3; t++)
		{
			for (size_t dl = 0; dl < 3; dl++)
			{
				if (DiapauseDuration[t][dl] > -999)
				{
					//NbVal = 8	Bias = 0.00263	MAE = 0.95222	RMSE = 1.25691	CD = 0.99785	R² = 0.99786
					//lam0 = 15.81011 {  15.80907, 15.81142}	VM = { 0.00021,   0.00060 }
					//lam1 = 2.50857 {   2.50779, 2.50943}	VM = { 0.00021,   0.00073 }
					//lam2 = 6.64395 {   6.63745, 6.64922}	VM = { 0.00113,   0.00379 }
					//lam3 = 7.81911 {   7.80857, 7.82666}	VM = { 0.00183,   0.00492 }
					//lam_a = 0.16346 {   0.16328, 0.16369}	VM = { 0.00006,   0.00019 }
					//lam_b = 0.26484 {   0.26458, 0.26499}	VM = { 0.00007,   0.00020 }

					double T = 10 + 5 * t;
					double DL = 8 + dl * 4;
					double DD = 120.0 + (215.0 - 120.0) * 1 / (1 + exp(-(T - m_ADE[ʎ0]) / m_ADE[ʎ1]));
					double f = exp(-m_ADE[ʎa] + m_ADE[ʎb] * 1 / (1 + exp(-(DL - m_ADE[ʎ2]) / m_ADE[ʎ3])));

					stat.Add(DiapauseDuration[t][dl], DD * f);
				}
			}
		}

		return true;
	}



	static const int ROUND_VAL = 4;
	CTRef CLaricobiusNigrinusModel::GetDiapauseEnd(const CWeatherYear& weather)
	{
		CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

		double sumDD = 0;
		for (CTRef TRef = p.Begin() + int(m_ADE[ʎ0] - 1); TRef <= p.End() && TRef <= p.Begin() + int(m_ADE[ʎ1] - 1); TRef++)
		{
			const CWeatherDay& wday = m_weather.GetDay(TRef);
			double T = wday[H_TNTX][MEAN];
			double DD = min(0.0, T - m_ADE[ʎb]);//DD is negative

			sumDD += DD;
		}

		boost::math::logistic_distribution<double> begin_dist(m_ADE[ʎ2], m_ADE[ʎ3]);
		int begin = (int)Round(m_ADE[ʎ1] + m_ADE[ʎa] * cdf(begin_dist, sumDD), 0);
		return  p.Begin() + begin;
	}


	bool CLaricobiusNigrinusModel::CalibrateDiapauseEnd(const bitset<NB_EVALUATED_STAGES>& test, CStatisticXY& stat)
	{
		for (size_t i = 0; i < NB_EVALUATED_STAGES; i++)
		{
			if (test[i])
			{

				if (m_OVP[Τᴴ¹] >= m_OVP[Τᴴ²])
					return false;


				if (m_SAResult.empty())
					return true;

				if (!m_weather.IsHourly())
					m_weather.ComputeHourlyVariables();



				for (size_t y = 0; y < m_weather.GetNbYears(); y++)
				{
					int year = m_weather[y].GetTRef().GetYear();
					if (m_years[i].find(year) == m_years[i].end())
						continue;



					double sumDD = 0;
					vector<double> CDD;
					CTPeriod p;

					if (i == E_EMERGING_ADULTS)
					{

						p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
						CDD.resize(p.size(), 0);

						CTRef diapauseEnd = GetDiapauseEnd(m_weather[year]);
						for (CTRef TRef = diapauseEnd; TRef <= p.End(); TRef++)
						{
							const CWeatherDay& wday = m_weather.GetDay(TRef);
							double T = wday[H_TNTX][MEAN];
							double DD = max(0.0, T - m_EAS[Τᴴ]);//DD is positive

							sumDD += DD;

							size_t ii = TRef - p.Begin();
							CDD[ii] = sumDD;
						}



					}
					else
					{
						p = m_weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

						CDD.resize(p.size(), 0);


						CDegreeDays DDModel(CDegreeDays::MODIFIED_ALLEN_WAVE, m_OVP[Τᴴ¹], m_OVP[Τᴴ²]);
						for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
						{
							const CWeatherDay& wday = m_weather.GetDay(TRef);
							size_t ii = TRef - p.Begin();
							sumDD += DDModel.GetDD(wday);
							CDD[ii] = sumDD;
						}
					}



					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						size_t ii = m_SAResult[i].m_ref - p.Begin();
						if (m_SAResult[i].m_ref.GetYear() == year && ii < CDD.size())
						{
							double obs_y = m_SAResult[i].m_obs[STAT_STAGE[i]];

							if (obs_y > -999)
							{

								double sim_y = 0;

								if (i == E_EMERGING_ADULTS)
								{
									boost::math::logistic_distribution<double> emerged_dist(m_EAS[μ], m_EAS[ѕ]);
									sim_y = Round(cdf(emerged_dist, CDD[ii]) * 100, ROUND_VAL);

								}
								else
								{
									boost::math::logistic_distribution<double> create_dist(m_OVP[μ], m_OVP[ѕ]);
									sim_y = Round(cdf(create_dist, CDD[ii]) * 100, ROUND_VAL);

								}


								if (sim_y < 0.1)
									sim_y = 0;
								if (sim_y > 99.9)
									sim_y = 100;

								stat.Add(obs_y, sim_y);
							}
						}
					}
				}//for all years
			}//if
		}//for

		return true;

	}

	bool CLaricobiusNigrinusModel::Calibrate(const bitset<NB_EVALUATED_STAGES>& test, CStatisticXY& stat)
	{
		if (!m_SAResult.empty())
		{
			if (!m_bCumul)
				m_bCumul = true;//SA always cumulative

			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();



			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if ((test[E_EGGS] && m_years[E_EGGS].find(year) != m_years[E_EGGS].end()) ||
					(test[E_LARVAE] && m_years[E_LARVAE].find(year) != m_years[E_LARVAE].end()) ||
					(test[E_LARVAL_DROP] && m_years[E_LARVAL_DROP].find(year) != m_years[E_LARVAL_DROP].end()) ||
					(test[E_EMERGING_ADULTS] && m_years[E_EMERGING_ADULTS].find(year) != m_years[E_EMERGING_ADULTS].end()))
				{

					CModelStatVector output;
					CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

					output.Init(p, NB_STATS, 0);
					ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);



					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						if (output.IsInside(m_SAResult[i].m_ref))
						{

							for (size_t j = 0; j < NB_EVALUATED_STAGES; j++)
							{
								if (test[j])
								{
									double obs_y = Round(m_SAResult[i].m_obs[j], ROUND_VAL);
									double sim_y = Round(output[m_SAResult[i].m_ref][STAT_STAGE[j]], ROUND_VAL);

									if (obs_y > -999)
									{
										stat.Add(obs_y, sim_y);

										double obs_x = m_SAResult[i].m_ref.GetJDay();
										double sim_x = GetSimX(STAT_STAGE[j], m_SAResult[i].m_ref, obs_y, output);

										if (obs_y > 5.0 && obs_y < 95.0)
										{
											obs_x = Round(100 * (obs_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE], ROUND_VAL);
											sim_x = Round(100 * (sim_x - m_nb_days[j][LOWEST]) / m_nb_days[j][RANGE], ROUND_VAL);
											stat.Add(obs_x, sim_x);
										}
									}
								}
							}
						}
					}//for all results
				}//have data
			}
		}

		return true;
	}

	bool CLaricobiusNigrinusModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;


		//return CalibrateDiapauseEndTh(stat);

		//bitset<NB_EVALUATED_STAGES> test;
		//test.reset();

		//test.set(E_EGGS);
		//test.set(E_LARVAE);
		//test.set(E_LARVAL_DROP);
		//test.set(E_EMERGING_ADULTS);
		//return CalibrateDiapauseEnd(test, stat);


		bitset<NB_EVALUATED_STAGES> test;
		test.reset();
		//test.set(E_EGGS);
		//test.set(E_LARVAE);
		//test.set(E_LARVAL_DROP);
		test.set(E_EMERGING_ADULTS);
		return Calibrate(test, stat);
	}
}
