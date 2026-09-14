//***********************************************************
// 2026-09-08   1.0.0	Rémi Saint-Amant	Creation
//***********************************************************


#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>


#include "GypsyMothEggHatchModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic/DegreeDays.h"
#include "ModelBase/DevRateEquation.h"


using namespace WBSF::HOURLY_DATA;
using namespace WBSF::LNF;
using namespace std;

namespace WBSF
{

	//const std::array<size_t, CGypsyMothEggHatchModel::NB_EVALUATED_STAGES> CGypsyMothEggHatchModel::STAT_STAGE = { S_EGGS };



	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::DOUBLE_SINE;

	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CGypsyMothEggHatchModel::CreateObject);

	CGypsyMothEggHatchModel::CGypsyMothEggHatchModel():
		m_EOD(CGypsyMothEggHatchEquations::EOD),
		m_EDP(CGypsyMothEggHatchEquations::EDP),
		m_RDR(CGypsyMothEggHatchEquations::RDR)
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.0 (2026)";

		m_eggHatchModel = EH_SAINT_AMANT;
		m_bCumul = false;
		m_bApplyAttrition = true;
	}

	CGypsyMothEggHatchModel::~CGypsyMothEggHatchModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CGypsyMothEggHatchModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		m_eggHatchModel = parameters[c++].GetInt();
		if (m_eggHatchModel >= NB_MODELS)
		{
			msg.ajoute("Invalid egg hatch model type");
			return msg;
		}

		m_bCumul = parameters[c++].GetBool();
		m_bApplyAttrition = parameters[c++].GetBool();

		

		if (parameters.size() == 3 + NB_EDP_PARAMS + NB_EOD_PARAMS + NB_RDR_PARAMS )
		{
			//m_begin = parameters[c++].GetInt();

			for (size_t p = 0; p < NB_EOD_PARAMS; p++)
				m_EOD[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_EDP_PARAMS; p++)
				m_EDP[p] = parameters[c++].GetFloat();
			
			for (size_t p = 0; p < NB_RDR_PARAMS; p++)
				m_RDR[p] = parameters[c++].GetFloat();
		}
		

		return msg;
	}



	//This method is called to compute the solution
	ERMsg CGypsyMothEggHatchModel::OnExecuteDaily()
	{
		ERMsg msg;

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		CTPeriod p = m_weather.GetEntireTPeriod(CTM(CTM::DAILY));
//		p.Begin().m_year += 1;
		m_output.Init(p, NB_STATS, 0);

		//simulation overall years
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, m_output);
		}

		return msg;
	}

	void CGypsyMothEggHatchModel::ExecuteDaily(int year, const CWeatherYears& weather, CModelStatVector& output)
	{
		//Create stand
		CLNFStand stand(this);
		stand.m_bApplyAttrition = m_bApplyAttrition;

		//Set parameters to equation
		stand.m_equations.m_EDP = m_EDP;
		stand.m_equations.m_EOD = m_EOD;
		stand.m_equations.m_RDR = m_RDR;


		stand.init(year, weather);

		//Create host
		CLNFHostPtr pHost(new CLNFHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;

		
		//pHost->Initialize<CGypsyMothEggHatch>(CInitialPopulation(400, 100, DIAPAUSE_EGG));
		pHost->Initialize<CGypsyMothEggHatch>(CInitialPopulation(InCalibration()?100:400, 100, DIAPAUSE_EGG));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		//CTPeriod p_current = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		
		//assert(weather[year].HavePrevious());
		//if (!weather[year].HavePrevious())
		//	return;

		//if they have other year extend period to July
		//p.Begin() = CJDayRef(year - 1, m_EOD[NCDDb] - 1);//-1: convert base 1 into base 0
		//p.Begin() = CJDayRef(year - 1, m_begin - 1);//-1: convert base 1 into base 0
		


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




	enum TInput { I_SOURCE, I_SITE, I_DATE, I_EGG_HATCH, I_EGG_HATCH_CUMUL, I_CV, NB_INPUTS };


	void CGypsyMothEggHatchModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == NB_INPUTS);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[I_DATE]);
		obs.m_obs.resize(1);
		
		if (data[I_EGG_HATCH] != "NA" && data[I_CV] == "C")
		{
			obs.m_obs[0] = stod(data[I_EGG_HATCH_CUMUL]);

			m_cumul_stats += obs.m_obs[0];
			m_nb_days += obs.m_ref.GetJDay();
			m_years.insert(obs.m_ref.GetYear());

		}
		else
		{
			obs.m_obs[0] = -999;
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

	bool CGypsyMothEggHatchModel::IsParamValid()const
	{
		bool bValid = true;

#if RSA_MODEL==1

		if (m_EOD[DOYb] >= m_EOD[DOYe])
			bValid = false;
		else if (m_EOD[Τᴴ¹] >= m_EOD[Τᴴ²]) 
			bValid = false;
		else 
			bValid = CDevRateEquation::IsParamValid(CDevRateEquation::Régnière_2012, { m_EDP.begin(), m_EDP.end() });
#else
		bValid = CDevRateEquation::IsParamValid(CDevRateEquation::Régnière_2012, { m_EOD.begin(), m_EOD.end() });
		bValid &= CDevRateEquation::IsParamValid(CDevRateEquation::Régnière_2012, { m_EDP.begin(), m_EDP.end() });
#endif


		return bValid;
	}


	static const int ROUND_VAL = 4;
	
	bool CGypsyMothEggHatchModel::Calibrate(CStatisticXY& stat)
	{
		if (!m_SAResult.empty())
		{
			
			m_bCumul = true;//SA always cumulative
			m_bApplyAttrition = false;//no attrition

			if (!m_weather.IsHourly())
				m_weather.ComputeHourlyVariables();

			for (size_t y = 0; y < m_weather.GetNbYears(); y++)
			{
				int year = m_weather[y].GetTRef().GetYear();
				if ( m_years.find(year) != m_years.end() )
				{
					CModelStatVector output;
					CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));

					output.Init(p, NB_STATS, 0);
					ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, output);


					for (size_t i = 0; i < m_SAResult.size(); i++)
					{
						if (output.IsInside(m_SAResult[i].m_ref))
						{
							double obs_y = Round(m_SAResult[i].m_obs[0], ROUND_VAL);
							double sim_y = Round(output[m_SAResult[i].m_ref][S_EGGS_HATCH], ROUND_VAL);

							if (obs_y > -999)
							{
								stat.Add(obs_y, sim_y);

								double obs_x = m_SAResult[i].m_ref.GetJDay();
								double sim_x = GetSimX(S_EGGS_HATCH, m_SAResult[i].m_ref, obs_y, output);

								if (obs_y > 5.0 && obs_y < 95.0)
								{
									if(sim_x<=-998)
										return false;

									obs_x = Round(100 * (obs_x - m_nb_days[LOWEST]) / m_nb_days[RANGE], 1);
									sim_x = Round(100 * (sim_x - m_nb_days[LOWEST]) / m_nb_days[RANGE], 1);
									stat.Add(obs_x, sim_x);
								}
							}
						}
					}//for all results
				}//have data
			}
		}

		return true;
	}

	bool CGypsyMothEggHatchModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;

		return Calibrate(stat);
	}
}
