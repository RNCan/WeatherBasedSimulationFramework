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
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/math/distributions/cauchy.hpp>

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



	class CModelDistribution
	{


	public:

		enum TType { NORMALS, LOG_NORMAL, LOGISTIC, WEIBULL, GAMMA, FISHER, EXTREME_VALUE, MODIFIED_LOGISTIC, NB_DISTRIBUTIONS };

		CModelDistribution(TType type, double p1, double p2)
		{
			m_type = type;
			m_p1 = p1;
			m_p2 = p2;

			switch (m_type)
			{
			case NORMALS:      p_normal_distribution.reset(new boost::math::normal_distribution<double>(p1, p2)); break;
			case LOG_NORMAL:   p_lognormal_distribution.reset(new boost::math::lognormal_distribution<double>(p1, p2)); break;
			case LOGISTIC:	   p_logistic_distribution.reset(new boost::math::logistic_distribution<double>(p1, p2)); break;
			case WEIBULL:	   p_weibull_distribution.reset(new boost::math::weibull_distribution<double>(p1, p2)); break;
			case GAMMA:		   p_fisher_f_distribution.reset(new boost::math::fisher_f_distribution<double>(p1, p2)); break;
			case FISHER:	   p_extreme_value_distribution.reset(new boost::math::extreme_value_distribution<double>(p1, p2)); break;
			case EXTREME_VALUE:p_gamma_distribution.reset(new boost::math::gamma_distribution<double>(p1, p2)); break;
			case MODIFIED_LOGISTIC: break;
			default:ASSERT(false);
			}

		}

		double get_cdf(double v)
		{

			double CDF = 0;
			switch (m_type)
			{
			case NORMALS:      CDF = cdf(*p_normal_distribution, v); break;
			case LOG_NORMAL:   CDF = cdf(*p_lognormal_distribution, v); break;
			case LOGISTIC:	   CDF = cdf(*p_logistic_distribution, v); break;
			case WEIBULL:	   CDF = cdf(*p_weibull_distribution, v); break;
			case GAMMA:		   CDF = cdf(*p_fisher_f_distribution, v); break;
			case FISHER:	   CDF = cdf(*p_extreme_value_distribution, v); break;
			case EXTREME_VALUE:CDF = cdf(*p_gamma_distribution, v); break;
			case MODIFIED_LOGISTIC:CDF = 1 / (1 + exp(-(v - m_p1) / sqrt(m_p2 * v))); break;
			default:ASSERT(false);
			}

			return CDF;
		}





	protected:

		TType m_type;
		double m_p1;
		double m_p2;

		std::unique_ptr<boost::math::normal_distribution<double>> p_normal_distribution;
		std::unique_ptr<boost::math::lognormal_distribution<double>> p_lognormal_distribution;
		std::unique_ptr<boost::math::logistic_distribution<double>> p_logistic_distribution;
		std::unique_ptr<boost::math::weibull_distribution<double>> p_weibull_distribution;
		std::unique_ptr<boost::math::fisher_f_distribution<double>> p_fisher_f_distribution;
		std::unique_ptr<boost::math::extreme_value_distribution<double>> p_extreme_value_distribution;
		std::unique_ptr<boost::math::gamma_distribution<double>> p_gamma_distribution;

	};




	CEmeraldAshBorerModel::CEmeraldAshBorerModel()
	{
		NB_INPUT_PARAMETER = -1;
		VERSION = "2.0.0 (2022)";



		//m_distribution_e = CModelDistribution::FISHER;
		//m_adult_emerg = { CModelDistribution::FISHER, 268.318,71.2378, 60, 12.5, 30.0, 0, 1 };

		//m_distribution_d = CModelDistribution::FISHER;
		//m_adult_dead = { CModelDistribution::FISHER, 152.733,55.6577, 60, 16, 50, 0, 1 };

		m_adult_emerg = { CModelDistribution::MODIFIED_LOGISTIC, 423.84,13.676,60,10.2,28.7,0,1 };
		m_adult_dead = { CModelDistribution::MODIFIED_LOGISTIC, 1247.5,4.6753,60,4.0,31, 0, 1 };






		/*<Parameter Name = "mu1">275.173 < / Parameter >
		< Parameter Name = "a1">10.4325 < / Parameter >
		< Parameter Name = "delta">60 < / Parameter >
		< Parameter Name = "Th1">13.1524 < / Parameter >
		< Parameter Name = "Th2">28.409 < / Parameter >



		< Parameter Name = "mu2">253.586 < / Parameter >
		< Parameter Name = "delta2">60 < / Parameter >
		< Parameter Name = "a2">0.0545667 < / Parameter >
		< Parameter Name = "ThL">8.30619 < / Parameter >
		< Parameter Name = "ThH">11.471 < / Parameter >
				*/

	}

	CEmeraldAshBorerModel::~CEmeraldAshBorerModel()
	{}

	//this method is called to load the generic parameters vector into the specific class member
	ERMsg CEmeraldAshBorerModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0;

		if (parameters.size() == 2 * NB_PARAMS)
		{
			for (size_t p = 0; p < NB_PARAMS; p++)
				m_adult_emerg[p] = parameters[c++].GetFloat();

			for (size_t p = 0; p < NB_PARAMS - 1; p++)
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


		CModelDistribution emerge_dist(CModelDistribution::TType(m_adult_emerg[d_type]), m_adult_emerg[μ], m_adult_emerg[ѕ]);
		CModelDistribution dead_dist(CModelDistribution::TType(m_adult_dead[d_type]), m_adult_dead[μ], m_adult_dead[ѕ]);

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


			//double logis = 1 / pow((1 + m_adult_emerg[alpha] * exp(-(CDDe[TRef][0]-m_adult_emerg[μ])/ m_adult_emerg[ѕ])), 1/ m_adult_emerg[beta]);
			//double logis_e = 1 / (1 + exp(-(CDDe[TRef][0] - m_adult_emerg[μ]) / pow(m_adult_emerg[alpha] * CDDe[TRef][0], m_adult_emerg[beta])));
			//double emergence = Round(100 * logis_e, 1);
			double emergence = Round(100 * emerge_dist.get_cdf(CDDe[TRef][0]), 1);
			if (emergence <= 0.1)
				emergence = 0.0;
			if (emergence >= 99.9)
				emergence = 100.0;

			double dead = bColdEvent ? 100 : Round(100 * dead_dist.get_cdf(CDDd[TRef][0]), 1);
			//double logis_d = 1 / (1 + exp(-(CDDd[TRef][0] - m_adult_dead[μ]) / pow(m_adult_dead[alpha] * CDDd[TRef][0], m_adult_dead[beta])));
			//double dead = bColdEvent ? 100 : Round(100 * logis_d, 1);
			if (dead <= 0.1)
				dead = 0.0;
			if (dead >= 99.9)
				dead = 100.0;


			double adult = max(0.0, min(100.0, emergence - dead));
			//double adult = emergence - dead;


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
		//m_DOY[obs.m_ref.GetYear()] += obs.m_ref.GetJDay();
		m_DOY += obs.m_ref.GetJDay();

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



	double CEmeraldAshBorerModel::GetSimDOY(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	{
		ASSERT(obs > -999);

		double DOY = -999;



		//if (obs > 0.01 && obs < 99.99)
		//if (obs >= 100)
			//obs = 99.99;//to avoid some problem of truncation

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

				DOY = obsX;
			}

		}

		return DOY;
	}

	double  CEmeraldAshBorerModel::GetDOYPercent(double DOY)const
	{
		//ASSERT(m_DOY.find(year) != m_DOY.end());
		//return value can be negative of greater than 100%
		return 100 * (DOY - m_DOY[LOWEST]) / m_DOY[RANGE];
	}
	bool CEmeraldAshBorerModel::CalibrateEmergence(CStatisticXY& stat)
	{
		double Ne = 0;
		double Nc = 0;
		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			if (m_SAResult[i].m_obs[I_EMERGENCE] > -999)
				Ne += m_SAResult[i].m_obs[I_EMERGENCE];
			if (m_SAResult[i].m_obs[I_CATCH] > -999)
				Nc += m_SAResult[i].m_obs[I_CATCH];
		}


		for (auto it = m_years.begin(); it != m_years.end(); it++)
		{
			int year = *it;

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

						for (size_t ii = 0; ii < log(3 * Ne); ii++)
							stat.Add(obs, sim);

						ASSERT(obs >= 0 && obs <= 100);
						ASSERT(sim >= 0 && sim <= 100);


						if (obs >= 0.5 && obs <= 99.5)
						{

							double sim_DOY = GetSimDOY(O_CUMUL_EMERGENCE, m_SAResult[i].m_ref, obs, output);
							if (sim_DOY > -999)
							{
								double obs_DOYp = GetDOYPercent(m_SAResult[i].m_ref.GetJDay());
								double sim_DOYp = GetDOYPercent(sim_DOY);

								for (size_t ii = 0; ii < log(3 * Ne); ii++)
									stat.Add(obs_DOYp, sim_DOYp);
							}
						}
					}

					//flies catch 
					if (m_SAResult[i].m_obs[I_CUMUL_CATCH] > -999)
					{
						double obs = m_SAResult[i].m_obs[I_CUMUL_CATCH];
						double sim = output[m_SAResult[i].m_ref][O_CUMUL_ADULT];

						for (size_t ii = 0; ii < log(Nc); ii++)
							stat.Add(obs, sim);


						if (obs >= 0.5 && obs <= 99.5)
						{

							double sim_DOY = GetSimDOY(O_CUMUL_ADULT, m_SAResult[i].m_ref, obs, output);
							if (sim_DOY > -999)
							{
								double obs_DOYp = GetDOYPercent(m_SAResult[i].m_ref.GetJDay());
								double sim_DOYp = GetDOYPercent(sim_DOY);

								for (size_t ii = 0; ii < log(3 * Ne); ii++)
									stat.Add(obs_DOYp, sim_DOYp);
							}
						}
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