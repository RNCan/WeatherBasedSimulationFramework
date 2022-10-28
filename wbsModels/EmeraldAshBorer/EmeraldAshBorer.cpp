//**************************************************************************************************************
// 27/10/2022	2.0.0	Rémi Saint-Amant	New version base on Poland 2011 and Duarte 2013 data
// 10/10/2019	1.0.2	Rémi Saint-Amant	New compile
// 13/04/2018	1.0.1	Rémi Saint-Amant	Compile with VS 2017
// 08/05/2017	1.0.0	Rémi Saint-Amant	Create from articles Lyons and Jones 2006
//**************************************************************************************************************

#include "ModelBase/EntryPoint.h"
#include "ModelBase/ContinuingRatio.h"
#include "EmeraldAshBorer.h"
#include "TreeMicroClimate.h"


#include "ModelBase/SimulatedAnnealingVector.h"
#include <boost/math/distributions/weibull.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/Rayleigh.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/non_central_f.hpp>
#include <boost/math/distributions/extreme_value.hpp>
#include <boost/math/distributions/fisher_f.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <random>

using namespace std;
using namespace WBSF::HOURLY_DATA;


namespace WBSF
{

	//links this class with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CEmeraldAshBorerModel::CreateObject);

	enum TOutput { O_PUPAE, O_ADULT, O_DEAD_ADULT, O_EMERGENCE, O_CUMUL_EMERGENCE, O_CUMUL_ADULT, O_CDD_EMERGENCE, O_CDD_DEAD, NB_OUTPUTS };
	//Defining a simple continuing ratio model
	//enum TInstar { EGG, L1, L2, L3, L4, PRE_PUPA, PUPA, TENERAL, ADULT, DEAD_ADULT, EMERGENCE, NB_STAGES };
	//extern const char HEADER[] = "EGG,L1,L2,L3,L4,Pre-Pupa,Pupa,Teneral,Adult,DeadAdult,Emergence";
	//typedef CContinuingRatio<NB_PARAMS, EGG, ADULT, HEADER> CHemlockLooperCR;
	/*static const double T_THRESHOLD[2][NB_STAGES] =
	{
		{ 13.9, 0, 0, 0, 0, 12.0, 13.6, 13.6, 0 },
		{ 13.9, 0, 0, 0, 0, 11.5, 14.7, 10.1, 0 },
	};

	static const double DD_THRESHOLD[2][NB_STAGES] =
	{
		{ 155.2, 0, 0, 0, 0, 118.3, 139.2, 43.1, 0 },
		{ 155.2, 0, 0, 0, 0, 121.0, 114.6, 64.4, 0 },
	};*/

	//const size_t CEmeraldAshBorerModel::FISRT_HL_JDAY = 61 - 1;   //first of March zero base
	//const double CEmeraldAshBorerModel::THRESHOLD = 5.4;			//°C
	//const double CEmeraldAshBorerModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 1075.5 };	//accumulated degree-days
	//const double CEmeraldAshBorerModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 1.2437 };	//variability
	//const double CEmeraldAshBorerModel::A[NB_PARAMS] = { 179.7, 259.8, 398.0, 495.1, 723.8, 945.9};	//accumulated degree-days
	//const double CEmeraldAshBorerModel::B[NB_PARAMS] = { 0.637, 2.591, 1.564, 1.414, 1.531, 0.741 };	//variability


	CEmeraldAshBorerModel::CEmeraldAshBorerModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "2.0.0 (2022)";

		//logistic parameters
		//m_adult_emerg =  {257.095, 42.7487, 60, 13.5, 28.5 } ;
		//m_adult_dead = {53.4461, 12.4678, 60, 22.5, 50};
		
		//Weibull parameters
		m_adult_emerg =  { 4.69451,272.817, 60, 13.5, 28.5 } ;
		m_adult_dead = { 3.53651,66.4979, 60, 22.5, 50};
	
		//m_bCumul = false;
	}

	CEmeraldAshBorerModel::~CEmeraldAshBorerModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CEmeraldAshBorerModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;
		//m_bCumul = 
		

		if (parameters.size() == 1 + 2 * NB_PARAMS)
		{
			parameters[c++].GetBool();
			for (size_t p = 0; p < NB_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PARAMS; p++)
				m_adult_dead[p] = parameters[c++].GetFloat();

		}

		return msg;
	}

	ERMsg CEmeraldAshBorerModel::OnExecuteDaily()
	{
		ERMsg msg;

		m_output.Init(m_weather.GetEntireTPeriod(), NB_OUTPUTS, 0);

		//Execute model for each year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			ExecuteDaily(m_weather[y], m_output);
		}

		return msg;
	}

	//void CEmeraldAshBorerModel::ExecuteDaily(CModelStatVector& output)
	//{
	//	
	//	output.Init(m_weather.GetEntireTPeriod(), NB_STAGES, 0, HEADER);//+3 for DD, death and AI
	//	COverheat overheating(0.0);

	//	for (size_t y = 0; y < m_weather.size(); y++)
	//	{
	//		size_t stage[2] = { PRE_PUPA, PRE_PUPA };
	//		double DD[2] = { 0 };
	//		double DD_E[2] = { 0 };

	//		//double DD = 0.0;
	//		CTPeriod p = m_weather[y].GetEntireTPeriod(CTM::DAILY);

	//		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
	//		{
	//			const CWeatherDay& wDay = m_weather.GetDay(TRef);
	//			CMicroClimate micro(wDay);
	//			double T = micro.GetTair();
	//			
	//			for (size_t s = 0; s < 2; s++)
	//			{
	//				if (stage[s] < ADULT)
	//				{
	//					//if (TRef.GetJDay() >= m_startJday)
	//					DD[s] += max(0.0, T - T_THRESHOLD[s][stage[s]]);
	//					if (DD[s] > DD_THRESHOLD[s][stage[s]])
	//					{
	//						DD[s] = 0;
	//						stage[s] ++;
	//					}
	//				}
	//				else if (stage[s] == ADULT)
	//				{
	//					for (size_t h = 0; h < 24; h++)
	//					{
	//						double R = 0.0004130*exp(0.187197*T);
	//						DD[s] += R/24;
	//					}

	//					if (DD[s] >= 1)
	//					{
	//						DD[s] = 0;
	//						stage[s] ++;
	//					}
	//				}

	//				static const double MAX_DDE[2] = {303.0,344.8};
	//				if (DD_E[s] < MAX_DDE[s] && DD_E[s] + max(0.0, T - 13.5 ) >= MAX_DDE[s])
	//				{
	//					output[TRef][EMERGENCE] += 50;
	//				}
	//				DD_E[s] += max(0.0, T - 13.5);

	//				output[TRef][stage[s]] += 50;
	//			}
	//		}
	//	}
	//	
	//	//CR.Execute(m_weather, output);
	//}


	//adult
	//Adult,Taylor_1981,psi=5.157545e-01 To=5.000000e+01 deltaT=1.166271e+01 sigma=0.0937978,"psi*exp(-1/2*((T-To)/deltaT)^2)
	void CEmeraldAshBorerModel::ExecuteDaily(const CWeatherYear& weather, CModelStatVector& output)
	{
		if (output.empty())
			output.Init(weather.GetEntireTPeriod(), NB_OUTPUTS, 0);

		
		
		//boost::math::logistic_distribution<double> emerge_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);
		//boost::math::logistic_distribution<double> dead_dist(m_adult_dead[μ], m_adult_dead[ѕ]);
		boost::math::weibull_distribution<double> emerge_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);
		boost::math::weibull_distribution<double> dead_dist(m_adult_dead[μ], m_adult_dead[ѕ]);



		int year = weather.GetTRef().GetYear();

		CModelStatVector CDDe;
		GetCDD(m_adult_emerg, m_weather[year], CDDe);
		CModelStatVector CDDd;
		GetCDD(m_adult_dead, m_weather[year], CDDd);

		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		bool bColdEvent = false;

		for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
		{
			if (TRef.GetJDay() >= 200 && weather.GetDay(TRef)[H_TMIN][LOWEST] < -5.0)
				bColdEvent = true;


			double emergence = Round(100 * cdf(emerge_dist, max(0.0, CDDe[TRef][0])), 1);
			if (emergence < 1.0)
				emergence = 0.0;
			if (emergence > 99.0)
				emergence = 100.0;
			
			double dead = bColdEvent?100:Round(100 * cdf(dead_dist, max(0.0, CDDd[TRef][0])), 1);
			if (dead < 1.0)
				dead = 0.0;
			if (dead > 99.0)
				dead = 100.0;


			double adult = max(0.0, min(100.0, emergence - dead));


			output[TRef][O_PUPAE] = 100 - emergence;
			output[TRef][O_ADULT] = adult;
			output[TRef][O_DEAD_ADULT] = dead;
			output[TRef][O_EMERGENCE] = emergence - ((TRef == p.Begin()) ? 0 : output[TRef - 1][O_CUMUL_EMERGENCE]);
			output[TRef][O_CUMUL_EMERGENCE] = emergence;
			output[TRef][O_CDD_EMERGENCE] = CDDe[TRef][0];
			output[TRef][O_CDD_DEAD] = CDDd[TRef][0];
		}

		CStatistic stat = output.GetStat(O_ADULT, p);
		if (stat.IsInit() && stat[SUM] > 0)
		{
			output[p.Begin()][O_CUMUL_ADULT] = 0;
			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				output[TRef][O_CUMUL_ADULT] = output[TRef - 1][O_CUMUL_ADULT] + 100 * output[TRef][O_ADULT] / stat[SUM];
		}

	}

	enum TInput { I_EMERGENCE, I_CUMUL_EMERGENCE, I_CATCH, I_CUMUL_CATCH };
	void CEmeraldAshBorerModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		ASSERT(data.size() == 6);

		CSAResult obs;

		obs.m_ref.FromFormatedString(data[1]);
		obs.m_obs.resize(4);
		obs.m_obs[I_EMERGENCE] = stod(data[2]);//Emergence
		obs.m_obs[I_CUMUL_EMERGENCE] = stod(data[3]);//Emergence
		obs.m_obs[I_CATCH] = stod(data[4]);//Catch
		obs.m_obs[I_CUMUL_CATCH] = stod(data[5]);//Catch

		m_years.insert(obs.m_ref.GetYear());


		m_SAResult.push_back(obs);

	}





	void CEmeraldAshBorerModel::GetCDD(const std::array<double, NB_PARAMS>& params, const CWeatherYears& weather, CModelStatVector& CDD)const
	{
		CDegreeDays DDmodel(CDegreeDays::MODIFIED_ALLEN_WAVE, params[Τᴴ¹], params[Τᴴ²]);
		CModelStatVector DD_daily;
		DDmodel.Execute(weather, DD_daily);

		CDD.Init(DD_daily.GetTPeriod(), 1, 0);

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod(CTM::DAILY);
			p.Begin() = p.Begin() + int(params[delta]);

			CDD[p.Begin()][0] = DD_daily[p.Begin()][CDegreeDays::S_DD];
			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				CDD[TRef][0] = CDD[TRef - 1][0] + DD_daily[TRef][CDegreeDays::S_DD];
		}

	}

	void CEmeraldAshBorerModel::GetCDD(const std::array<double, NB_PARAMS>& params, const CWeatherYear& weather, CModelStatVector& CDD)const
	{
		CDegreeDays DDmodel(CDegreeDays::MODIFIED_ALLEN_WAVE, params[Τᴴ¹], params[Τᴴ²]);
		CModelStatVector DD_daily;
		DDmodel.Execute(weather, DD_daily);

		CDD.Init(DD_daily.GetTPeriod(), 1, 0);

		CTPeriod p = weather.GetEntireTPeriod(CTM::DAILY);
		p.Begin() = p.Begin() + int(params[delta]);

		CDD[p.Begin()][0] = DD_daily[p.Begin()][CDegreeDays::S_DD];
		for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
			CDD[TRef][0] = CDD[TRef - 1][0] + DD_daily[TRef][CDegreeDays::S_DD];


	}

	enum TPout { P_CDD, P_CE };//CE = cumulative emergence
	void CEmeraldAshBorerModel::GetPobsUni(size_t v, const std::array<double, NB_PARAMS>& param, CModelStatVector& P)
	{
		string ID = GetInfo().m_loc.m_ID;

		//compute CDD for all temperature profile
		double total = 0;
		vector<tuple<double, CTRef, double, bool>> d;
		const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();
		for (size_t i = 0; i < SA.size(); i++)
		{
			string IDi = SA[i]->GetInfo().m_loc.m_ID;

			CModelStatVector CDD;
			GetCDD(param, SA[i]->m_weather, CDD);
			const CSAResultVector& SAResult = SA[i]->GetSAResult();
			for (size_t ii = 0; ii < SAResult.size(); ii++)
			{
				if (SAResult[ii].m_obs[v] > -999)
				{
					d.push_back(make_tuple(CDD[SAResult[ii].m_ref][CDegreeDays::S_DD], SAResult[ii].m_ref, SAResult[ii].m_obs[v], IDi == ID));
					total += SAResult[ii].m_obs[v];
				}
			}
		}

		//sor all observation
		sort(d.begin(), d.end());

		//
		P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), 2, -999);
		double sum = 0;
		for (size_t i = 0; i < d.size(); i++)
		{
			sum += std::get<2>(d[i]);

			if (std::get<3>(d[i]))//used only the appropriate values for this site/year
			{
				CTRef Tref = std::get<1>(d[i]);
				double CDD = std::get<0>(d[i]);
				double p = Round(100 * sum / total, 1);

				P[Tref][P_CDD] = CDD;
				P[Tref][P_CE] = p;
			}
		}
	}




	bool CEmeraldAshBorerModel::CalibrateEmergence(CStatisticXY& stat)
	{
		//if (m_SAResult.size() < 4)
			//return true;

		//boost::math::lognormal_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::weibull_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::beta_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::exponential_distribution<double> emerge_dist(m_P[ѕ]);
		//boost::math::rayleigh_distribution<double> emerge_dist(m_P[ѕ]);


		double Ne = 0;
		double Nc = 0;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (m_SAResult[i].m_obs[I_EMERGENCE] > -999)
				Ne += m_SAResult[i].m_obs[I_EMERGENCE];
			if (m_SAResult[i].m_obs[I_CATCH] > -999)
				Nc += m_SAResult[i].m_obs[I_CATCH];
		}



		//CModelStatVector Pe;
		//GetPobsUni(I_EMERGENCE, m_adult_emerg, Pe);

		//CModelStatVector Pc;
		//GetPobsUni(I_CATCH, m_adult_dead, Pc);

		//array<boost::math::logistic_distribution<double>,4> emerge_dist(mu, S);
		//array<boost::math::logistic_distribution<double>, 4> emerge_dist = { {m_P[μ1], m_P[ѕ1], m_P[μ1], m_P[ѕ1]} };

		//boost::math::weibull_distribution<double> emerge_dist(mu, S);
		//boost::math::logistic_distribution<double> emerge_dist(m_adult_emerg[μ], m_adult_emerg[ѕ]);

		//boost::math::weibull_distribution<double> emerge_dist(mu, S);
		//boost::math::logistic_distribution<double> dead_dist(m_adult_dead[μ], m_adult_dead[ѕ]);

		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;
			//ASSERT(m_weather.IsYearInit(year));

			//CModelStatVector CDDe;
			//GetCDD(m_adult_emerg, m_weather[year], CDDe);
			//CModelStatVector CDDd;
			//GetCDD(m_adult_dead, m_weather[year], CDDd);

			////compute adult alive and cumulative adult alive
			//CModelStatVector adult(m_weather[year].GetEntireTPeriod(CTM::DAILY), 2, 0);
			//{

			//}

			CModelStatVector output;
			ExecuteDaily(m_weather[year], output);

			for (size_t i = 0; i < m_SAResult.size(); i++)
			{
				if (m_SAResult[i].m_ref.GetYear() == year)
				{
					//emergence
					if (m_SAResult[i].m_obs[I_CUMUL_EMERGENCE] > -999)
					{

						double obs = m_SAResult[i].m_obs[I_CUMUL_EMERGENCE];
						double sim = output[m_SAResult[i].m_ref][O_CUMUL_EMERGENCE];

						for (size_t ii = 0; ii < log(2 * Ne); ii++)
							stat.Add(obs, sim);

						ASSERT(obs >= 0 && obs <= 100);
						ASSERT(sim >= 0 && sim <= 100);
					}

					//flies catch 
					if (m_SAResult[i].m_obs[I_CUMUL_CATCH] > -999)
					{
						double obs = m_SAResult[i].m_obs[I_CUMUL_CATCH];
						double sim = output[m_SAResult[i].m_ref][O_CUMUL_ADULT];

						for (size_t ii = 0; ii < log(Nc); ii++)
							stat.Add(obs, sim);
					}
				}
			}//for all results

		}
		return true;

	}

	bool CEmeraldAshBorerModel::IsParamValid()const
	{
		bool bValid = true;


		if (m_adult_emerg[Τᴴ¹] >= m_adult_emerg[Τᴴ²])
			bValid = false;
		if (m_adult_dead[Τᴴ¹] >= m_adult_dead[Τᴴ²])
			bValid = false;

		return bValid;
	}



	bool CEmeraldAshBorerModel::GetFValueDaily(CStatisticXY& stat)
	{
		if (!IsParamValid())
			return false;
		return CalibrateEmergence(stat);
	}

}