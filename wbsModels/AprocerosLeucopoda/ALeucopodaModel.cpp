//***********************************************************
// 26/01/2021	1.0.0	Rémi Saint-Amant   Creation
//***********************************************************
#include "ALeucopodaModel.h"
#include "ModelBase/EntryPoint.h"
#include "Basic\DegreeDays.h"
#include <boost/math/distributions/logistic.hpp>
#include "ModelBase/SimulatedAnnealingVector.h"

using namespace WBSF::HOURLY_DATA;
using namespace std;
using namespace WBSF::TZZ;


//static const bool BEGIN_JULY = true;
//static const size_t FIRST_Y = BEGIN_JULY ? 1 : 0;

namespace WBSF
{

	static const size_t NB_GENERATIONS_MAX = 6;

	//static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::MODIFIED_ALLEN_WAVE;
	static const CDegreeDays::TDailyMethod DD_METHOD = CDegreeDays::ALLEN_WAVE;
	//enum { O_CDD, O_GENERATION, O_DIAPAUSED, O_EGG, O_LARVA, O_PUPA, O_ADULT, O_DEAD_ADULT, O_BROOD, NB_OUTPUTS };
	enum { O_EGG, O_LARVA, O_PREPUPA, O_PUPA, O_ADULT, O_DEAD_ADULT, O_BROOD, O_DEAD_ATTRITION, NB_OUTPUT_ONE_G, O_IN_DIAPAUSE = (NB_OUTPUT_ONE_G * NB_GENERATIONS_MAX - O_PUPA), O_D_DAY_LENGTH, NB_DAILY_OUTPUTS };


	//this line link this model with the EntryPoint of the DLL
	static const bool bRegistred =
		CModelFactory::RegisterModel(CAprocerosLeucopodaModel::CreateObject);

	CAprocerosLeucopodaModel::CAprocerosLeucopodaModel()
	{
		//NB_INPUT_PARAMETER is used to determine if the dll
		//uses the same number of parameters than the model interface
		NB_INPUT_PARAMETER = -1;
		VERSION = "1.0.1 (2022)";

		m_bApplyAttrition = false;
		m_generationSurvival = 0.5;
//		m_bCumul = false;
//		m_stage = 0;
	//	m_T = 0;

		//m_EWD.fill(0);
		//m_EAS.fill(0);
		//Set parameters to equation
		//ASSERT(stand.m_equations.m_EWD.size() == m_EWD.size());
		for (size_t p = 0; p < m_EWD.size(); p++)
			m_EWD[p] = CAprocerosLeucopodaEquations::EWD[p];

		//ASSERT(stand.m_equations.m_EAS.size() == m_EAS.size());
		for (size_t p = 0; p < m_EAS.size(); p++)
			m_EAS[p] = CAprocerosLeucopodaEquations::EAS[p];


		//m_P.fill(0);
		/*m_P[Τᴴ²] = 19.1;
		m_P[delta] = 45;
		m_P[μ1] = 239.6;
		m_P[ѕ1] = 41.4;
		m_P[μ2] = 876.0;
		m_P[ѕ2] = 55.6;
		m_P[μ3] = 625.2;
		m_P[ѕ3] = 38.3;
			*/
	}

	CAprocerosLeucopodaModel::~CAprocerosLeucopodaModel()
	{
	}


	//this method is call to load your parameter in your variable
	ERMsg CAprocerosLeucopodaModel::ProcessParameters(const CParameterVector& parameters)
	{
		ERMsg msg;

		size_t c = 0; 

		//		m_stage = parameters[c++].GetInt() + 1;
			//	m_T = parameters[c++].GetInt()+1;

		//m_bCumul = parameters[c++].GetBool();
		if (parameters.size() == m_EWD.size() + m_EAS.size() + 1)
		{
			//entering winter diapause  parameters
			for (size_t p = 0; p < m_EWD.size(); p++)
			{
				m_EWD[p] = parameters[c++].GetFloat();
			}

			//Emerging Adult from Soil (spring) parameters
			for (size_t p = 0; p < m_EAS.size(); p++)
			{
				m_EAS[p] = parameters[c++].GetFloat();
			}
		}
		

		return msg;
	}





	ERMsg CAprocerosLeucopodaModel::OnExecuteDaily()
	{
		ERMsg msg;

		/*	if (m_weather.GetNbYears() < 2)
			{
				msg.ajoute("Laricobius nigrinus model need at least 2 years of data");
				return msg;
			}*/

		if (!m_weather.IsHourly())
			m_weather.ComputeHourlyVariables();

		//This is where the model is actually executed
		m_output.Init(m_weather.GetEntireTPeriod(CTM(CTM::DAILY)), NB_DAILY_OUTPUTS, 0);

		//we simulate 2 years at a time. 
		//we also manager the possibility to have only one year
		for (size_t y = 0; y < m_weather.size(); y++)
		{
			//one output by generation
			vector<CModelStatVector> outputs;
			ExecuteDaily(m_weather[y].GetTRef().GetYear(), m_weather, outputs);


			//merge generations vector into one output vector (max of 5 generations)
			size_t maxG = min(NB_GENERATIONS_MAX, outputs.size());

			CTPeriod p = m_weather[y].GetEntireTPeriod(CTM(CTM::DAILY));
			for (CTRef TRef = p.Begin(); TRef <= p.End(); TRef++)
			{
				CStatistic diapause;
				for (size_t g = 0, ss = 0; g < maxG; g++)
				{
					size_t s_i = (g==0)?PUPA:EGG;
					for (size_t s = s_i; s < NB_OUTPUT_ONE_G; s++, ss++)
					{
						m_output[TRef][ss] = outputs[g][TRef][s];
						diapause += outputs[g][TRef][S_DIAPAUSE];
					}
				}

				
				m_output[TRef][O_IN_DIAPAUSE] = diapause[SUM];
				m_output[TRef][O_D_DAY_LENGTH] = m_weather.GetDayLength(TRef) / 3600.;
			}
		}

		return msg;
	}

	void CAprocerosLeucopodaModel::ExecuteDaily(int year, const CWeatherYears& weather, vector<CModelStatVector>& output)
	{
		//Create stand
		CTZZStand stand(this);
		stand.m_bApplyAttrition = m_bApplyAttrition;
		stand.m_generationSurvival = m_generationSurvival;

		
		//Set parameters to equation

		ASSERT(stand.m_equations.m_EWD.size() == m_EWD.size());
		for (size_t p = 0; p < m_EWD.size(); p++)
			stand.m_equations.m_EWD[p] = m_EWD[p];

		ASSERT(stand.m_equations.m_EAS.size() == m_EAS.size());
		for (size_t p = 0; p < m_EAS.size(); p++)
			stand.m_equations.m_EAS[p] = m_EAS[p];


		stand.init(year, weather);

		//compute 30 days avg
		//stand.ComputeTavg30(year, weather);


		//Create host
		CTZZHostPtr pHost(new CTZZHost(&stand));

		pHost->m_nbMinObjects = 10;
		pHost->m_nbMaxObjects = 1000;


		pHost->Initialize<CAprocerosLeucopoda>(CInitialPopulation(CTRef(year, JANUARY, DAY_01), 0, 400, 100, TZZ::PUPA, WBSF::FEMALE, true, 0));

		//add host to stand			
		stand.m_host.push_front(pHost);

		CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));
		


		for (CTRef d = p.Begin(); d <= p.End(); d++)
		{

			stand.Live(weather.GetDay(d));
			//if (output.IsInside(d))
				//stand.GetStat(d, output[d]);

			size_t nbGenerations = stand.GetFirstHost()->GetNbGeneration();
			if (nbGenerations > output.size())
				output.push_back(CModelStatVector(p, NB_STATS, 0));

			for (size_t g = 0; g < nbGenerations; g++)
				stand.GetStat(d, output[g][d], g);



			stand.AdjustPopulation();
			HxGridTestConnection();
		}




		//if (m_bCumul)
		//{
		//	for (size_t g = 0; g < output.size(); g++)
		//	{
		//		//cumulative result
		//		for (size_t s = S_EGG; s < S_ADULT; s++)
		//		{
		//			CTPeriod p = weather[year].GetEntireTPeriod(CTM(CTM::DAILY));

		//			CStatistic stat = output[g].GetStat(s, p);
		//			if (stat.IsInit() && stat[SUM] > 0)
		//			{
		//				output[g][0][s] = output[g][0][s] * 100 / stat[SUM];//when first day is not 0
		//				for (CTRef d = p.Begin() + 1; d <= p.End(); d++)
		//				{
		//					output[g][d][s] = output[g][d - 1][s] + output[g][d][s] * 100 / stat[SUM];
		//					_ASSERTE(!_isnan(output[g][d][s]));
		//				}
		//			}
		//		}
		//	}
		//}
	}

	

	//void CAprocerosLeucopodaModel::GetCDD(int year, const CWeatherYears& weather, CModelStatVector& CDD)
	//{
	//	CDegreeDays DDmodel(DD_METHOD, m_P[Τᴴ¹], m_P[Τᴴ²]);
	//	CModelStatVector DD;
	//	DDmodel.Execute(weather[year], DD);



	//	CDD.Init(DD.GetTPeriod(), 1, 0.0);

	//	//for (CTRef TRef = DD.GetFirstTRef(); TRef <= DD.GetLastTRef(); TRef++)
	//	CDD[0][0] = DD[0][CDegreeDays::S_DD];
	//	for (size_t i = 1; i < DD.size(); i++)
	//		CDD[i][0] = CDD[i - 1][0] + DD[i][CDegreeDays::S_DD];
	//}

	void CAprocerosLeucopodaModel::GetCDD(const CWeatherYears& weather, CModelStatVector& CDD)
	{
		//CTZZStand* pStand = GetStand();
		CDegreeDays DDmodel(DD_METHOD, m_EAS[Τᴴ¹], m_EAS[Τᴴ²]);
		CModelStatVector DD;
		DDmodel.Execute(weather, DD);


		CDD.Init(DD.GetTPeriod(), 1, -999);

		for (size_t y = 0; y < weather.GetNbYears(); y++)
		{
			CTPeriod p = weather[y].GetEntireTPeriod();
			//p.Begin() = p.Begin() + int(m_P[delta]);
			CDD[p.Begin()][0] = DD[p.Begin()][CDegreeDays::S_DD];

			for (CTRef TRef = p.Begin() + 1; TRef <= p.End(); TRef++)
				CDD[TRef][0] = CDD[TRef - 1][0] + DD[TRef][CDegreeDays::S_DD];
		}

	}

	//enum TSpecies { S_LA_G1, S_LA_G2, S_LP, S_LN };
	enum TInput { I_KEYID, I_DATE, I_STAGE, I_GENERATION, I_N, I_CUMUL, NB_INPUTS };
	void CAprocerosLeucopodaModel::AddDailyResult(const StringVector& header, const StringVector& data)
	{
		//KeyID	Date	Stage	Generation	N	Cumul
		ASSERT(data.size() == NB_INPUTS);
		//SYC	site	Year	collection	col_date	emerge_date	daily_count	species	n_days P Time
		//if (stoi(data[I_VARIABLE]) == m_stage && stoi(data[I_T]) == m_T)
		//{
		CSAResult obs;
		obs.m_ref.FromFormatedString(data[I_DATE]);
		obs.m_obs.resize(NB_INPUTS);
		for (size_t i = 2; i < NB_INPUTS; i++)
		{
			if (i == I_STAGE)
				obs.m_obs[i] = 0;
			else
				obs.m_obs[i] = stod(data[i]);
		}
			
		//CSAResult obs;

		//CStatistic egg_creation_date;

		
		//obs.m_obs.resize(3);
		//obs.m_obs[0] = 0;
		//obs.m_obs[1] = stod(data[I_GENERATION]);
		//obs.m_obs[2] = stod(data[I_CUMUL]);
		////obs.m_obs[I_S] = ;
		////obs.m_obs[I_P] = stod(data[10]);
		////obs.m_obs[I_CDD] = stod(data[11]);
		////obs.m_obs[I_P] = stod(data[12]);


		//if (data[7] == "LA" && data[8] == "1")
		//	obs.m_obs[I_S] = S_LA_G1;
		//else if (data[7] == "LA" && data[8] == "2")
		//	obs.m_obs[I_S] = S_LA_G2;
		//else if (data[7] == "LP")
		//	obs.m_obs[I_S] = S_LP;
		//else if (data[7] == "LN")
		//	obs.m_obs[I_S] = S_LN;

		m_SAResult.push_back(obs);
	}

	//ASSERT(data.size() == NB_INPUTS);
	////SYC	site	Year	collection	col_date	emerge_date	daily_count	species	n_days P Time
	//if (stoi(data[I_VARIABLE]) == m_stage && stoi(data[I_T]) == m_T)
	//{
	//	CSAResult obs;
	//	obs.m_obs.resize(NB_INPUTS);
	//	for (size_t i = 0; i < NB_INPUTS; i++)
	//		obs.m_obs[i] = stod(data[i]);
	//	//CSAResult obs;

	//	//CStatistic egg_creation_date;

	//	//obs.m_ref.FromFormatedString(data[5]);
	//	//obs.m_obs.resize(NB_INPUTS);
	//	//obs.m_obs[I_N] = stod(data[6]);
	//	////obs.m_obs[I_S] = ;
	//	////obs.m_obs[I_P] = stod(data[10]);
	//	////obs.m_obs[I_CDD] = stod(data[11]);
	//	////obs.m_obs[I_P] = stod(data[12]);


	//	//if (data[7] == "LA" && data[8] == "1")
	//	//	obs.m_obs[I_S] = S_LA_G1;
	//	//else if (data[7] == "LA" && data[8] == "2")
	//	//	obs.m_obs[I_S] = S_LA_G2;
	//	//else if (data[7] == "LP")
	//	//	obs.m_obs[I_S] = S_LP;
	//	//else if (data[7] == "LN")
	//	//	obs.m_obs[I_S] = S_LN;

	//	m_SAResult.push_back(obs);
	//}
//	}



	//double GetSimX(size_t s, CTRef TRefO, double obs, const CModelStatVector& output)
	//{
	//	double x = -999;

	//	if (obs > -999)
	//	{
	//		//if (obs > 0.01 && obs < 99.99)
	//		if (obs >= 100)
	//			obs = 99.99;//to avoid some problem of truncation

	//		long index = output.GetFirstIndex(s, ">=", obs, 1, CTPeriod(TRefO.GetYear(), FIRST_MONTH, FIRST_DAY, TRefO.GetYear(), LAST_MONTH, LAST_DAY));
	//		if (index >= 1)
	//		{
	//			double obsX1 = output.GetFirstTRef().GetJDay() + index;
	//			double obsX2 = output.GetFirstTRef().GetJDay() + index + 1;

	//			double obsY1 = output[index][s];
	//			double obsY2 = output[index + 1][s];
	//			if (obsY2 != obsY1)
	//			{
	//				double slope = (obsX2 - obsX1) / (obsY2 - obsY1);
	//				double obsX = obsX1 + (obs - obsY1)*slope;
	//				ASSERT(!_isnan(obsX) && _finite(obsX));

	//				x = obsX;
	//			}
	//		}
	//	}

	//	return x;
	//}

	bool CAprocerosLeucopodaModel::IsParamValid()const
	{
		bool bValid = true;

		return bValid;
	}





	//static const int ROUND_VAL = 4;
	//CTRef CAprocerosLeucopodaModel::GetEmergence(const CWeatherYear& weather)
	//{
	//	CTPeriod p = weather.GetEntireTPeriod(CTM(CTM::DAILY));

	//	double sumDD = 0;
	//	for (CTRef TRef = p.Begin()+172; TRef <= p.End()&& TRef<= p.Begin() + int(m_ADE[ʎ0]); TRef++)
	//	{
	//		//size_t ii = TRef - p.Begin();
	//		const CWeatherDay& wday = m_weather.GetDay(TRef);
	//		double T = wday[H_TNTX][MEAN];
	//		T = Round(max(m_ADE[ʎa], T), ROUND_VAL);

	//		double DD = min(0.0, T - m_ADE[ʎb]);//DD is negative

	//		//if (ii < m_ADE[ʎ0])
	//			sumDD += DD;
	//	}


	//	boost::math::logistic_distribution<double> begin_dist(m_ADE[ʎ2], m_ADE[ʎ3]);
	//	int begin = (int)Round(m_ADE[ʎ0] + m_ADE[ʎ1] * cdf(begin_dist, sumDD), 0);
	//	return  p.Begin() + begin;
	//}
	//enum TPout {P_CDD, P_CE, LA_G1= P_CE, P_LA_G2, P_LP, P_LN, NB_P};//CE = cumulative emergence
	//void CAprocerosLeucopodaModel::GetPobs(CModelStatVector& P)
	//{
	//	string ID = GetInfo().m_loc.m_ID;
	//	string SY = ID.substr(0, ID.length() - 2);

	//	//compute CDD for all temperature rprofile
	//	array< double, 4> total = { 0 };
	//	vector<tuple<double, CTRef, double, bool, size_t>> d;
	//	const CSimulatedAnnealingVector& SA = GetSimulatedAnnealingVector();

	//	for (size_t i = 0; i < SA.size(); i++)
	//	{
	//		string IDi = SA[i]->GetInfo().m_loc.m_ID;
	//		string SYi = IDi.substr(0, IDi.length() - 2);
	//		if (SYi == SY)
	//		{
	//			CModelStatVector CDD;
	//			GetCDD(SA[i]->m_weather, CDD);
	//			const CSAResultVector& v = SA[i]->GetSAResult();
	//			for (size_t ii = 0; ii < v.size(); ii++)
	//			{
	//				d.push_back(make_tuple(CDD[v[ii].m_ref][0], v[ii].m_ref, v[ii].m_obs[I_N], IDi == ID, v[ii].m_obs[I_S]));
	//				total[v[ii].m_obs[I_S]] += v[ii].m_obs[I_N];
	//			}
	//		}
	//	}

	//	sort(d.begin(), d.end());

	//	P.Init(m_weather.GetEntireTPeriod(CTM::DAILY), NB_P, 0);
	//	array< double, 4> sum = { 0 };
	//	for (size_t i = 0; i < d.size(); i++)
	//	{
	//		size_t s = std::get<4>(d[i]);
	//		sum[s] += std::get<2>(d[i]);
	//		if (std::get<3>(d[i]))
	//		{
	//			CTRef Tref = std::get<1>(d[i]);
	//			/*double obsP = -999;
	//			for (size_t k = 0; k < m_SAResult.size(); k++)
	//				if (m_SAResult[k].m_ref == Tref)
	//					obsP = m_SAResult[k].m_obs[I_P];*/


	//			double CDD = std::get<0>(d[i]);
	//			double p = Round(100 * sum[s] / total[s], 1);
	//			
	//			P[Tref][P_CDD] = CDD;
	//			P[Tref][P_CE + s] = p;
	//		}
	//	}
	//}

	//void CAprocerosLeucopodaModel::CalibrateEmergence(CStatisticXY& stat)
	//{
	//	if (m_SAResult.empty())
	//		return;

	//	//boost::math::lognormal_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);



	//	//boost::math::weibull_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
	//	//boost::math::beta_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
	//	//boost::math::exponential_distribution<double> emerge_dist(m_P[ѕ]);
	//	//boost::math::rayleigh_distribution<double> emerge_dist(m_P[ѕ]);


	//	//CModelStatVector CDD; 
	//	//GetCDD(m_weather, CDD);

	//	double n = 0;

	//	for (size_t i = 0; i < m_SAResult.size(); i++)
	//		n += m_SAResult[i].m_obs[I_N];


	//	CModelStatVector P;
	//	GetPobs(P);

	//	//array<boost::math::logistic_distribution<double>,4> emerge_dist(mu, S);
	//	//		array<boost::math::logistic_distribution<double>, 4> emerge_dist = { {m_P[μ1], m_P[ѕ1], m_P[μ1], m_P[ѕ1]} };

	//	for (size_t i = 0; i < m_SAResult.size(); i++)
	//	{
	//		size_t s = m_SAResult[i].m_obs[I_S];
	//		double mu = m_P[μ1 + 2 * s];
	//		double S = m_P[ѕ1 + 2 * s];

	//		boost::math::logistic_distribution<double> emerge_dist(mu, S);

	//		double CDD = P[m_SAResult[i].m_ref][P_CDD];
	//		double obs = P[m_SAResult[i].m_ref][P_CE+s];
	//		ASSERT(obs >= 0 && obs <= 100);

	//		double sim = Round(100 * cdf(emerge_dist, max(0.0, CDD)), 1);
	//		for (size_t ii = 0; ii < log(n); ii++)
	//			stat.Add(obs, sim);

	//	}//for all results

	//	return;

	//}

	//static double ei(size_t n) { return pow(1.0 + 1.0 / n, n); }
	//static double cv_2_sigma(double cv, size_t n)
	//{
	//	static const double e = exp(1);
	//	static const double p[3] = { 0.528196, 2.373248, 3.493202 };//with 10 000 replication
	//	return e * cv*(1 - p[0] * sqrt(e - ei(n))) / (p[1] + cv * (1 - p[2] * sqrt(e - ei(n))));
	//}

	void CAprocerosLeucopodaModel::GetFValueDaily(CStatisticXY& stat)
	{
		
		ASSERT(!m_SAResult.empty() );
		
		//CTZZStand* pStand = GetStand();
		//for (size_t p = 0; p < m_EAS.size(); p++)
			//pStand->m_equations.m_EAS[p] = m_EAS[p];

		if (!IsParamValid())
			return;


		//boost::math::lognormal_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);



		//boost::math::weibull_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::beta_distribution<double> emerge_dist(m_P[μ], m_P[ѕ]);
		//boost::math::exponential_distribution<double> emerge_dist(m_P[ѕ]);
		//boost::math::rayleigh_distribution<double> emerge_dist(m_P[ѕ]);


		//CModelStatVector CDD; 
		//GetCDD(m_weather, CDD);

		double n = 0;

		for (size_t i = 0; i < m_SAResult.size(); i++)
			n += m_SAResult[i].m_obs[I_N];


		CModelStatVector CDD;
		GetCDD(m_weather, CDD);
		//GetPobs(P);

		
		boost::math::logistic_distribution<double> emerge_dist(m_EAS[μ], m_EAS[ѕ]);
		

		for (size_t i = 0; i < m_SAResult.size(); i++)
		{
			size_t s = m_SAResult[i].m_obs[I_STAGE];
			
			
			double GDD = CDD[m_SAResult[i].m_ref][CDegreeDays::S_DD];
			double obs = m_SAResult[i].m_obs[I_CUMUL];
			//ASSERT(obs >= 0 && obs <= 100);

			double sim = Round(100 * cdf(emerge_dist, max(0.0, GDD)), 1);
			//for (size_t ii = 0; ii < log(n); ii++)
			stat.Add(obs, sim);

		}//for all results

		return;


		//bool bZero = false;
		//CStatistic x;
		//for (size_t i = 0; i < m_SAResult.size(); i++)
		//{
		//	//ASSERT(m_SAResult[i].m_obs[I_VARIABLE] == m_stage);
		//	//ASSERT(m_SAResult[i].m_obs[I_T] == m_T);

		//	size_t n = size_t(ceil(m_P[i]));
		//	bZero |= n == 0;

		//	for (size_t b = 0; b < n; b++)
		//		x += m_SAResult[i].m_obs[I_TIME];

		//}//for all results

		////if (!bZero  && int(x[NB_VALUE]) == int(m_SAResult[0].m_obs[I_N]) )
		//{
		//	double obs = m_SAResult[0].m_obs[I_MEAN_TIME];
		//	double sim = x[MEAN];
		//	stat.Add(obs, sim);

		//	obs = m_SAResult[0].m_obs[I_TIME_SD] * 10;
		//	sim = x[STD_DEV] * 10;
		//	stat.Add(obs, sim);

		//	obs = size_t(m_SAResult[0].m_obs[I_N]);
		//	sim = size_t(x[NB_VALUE]);
		//	stat.Add(obs, sim);



		//	CStatistic stat_ws;
		//	CStatistic stat_n;


		//	//compute sigma

		//	double mean = x[MEAN];
		//	double sd = x[STD_DEV];
		//	double n = x[NB_VALUE];
		//	if (n > 0)
		//	{
		//		double cv = sd / mean;
		//		double sigma = cv_2_sigma(cv, n);

		//		stat_ws += n * sigma;
		//		stat_n += n;
		//	}

		//	static const double SIGMA_OBS[4] = { 0.102, 0.157, 0.203, 0.118 };


		//	obs = SIGMA_OBS[m_stage];
		//	sim = stat_ws[SUM] / stat_n[SUM];

		//}

	}



	//void CAprocerosLeucopodaModel::FinalizeStat(CStatisticXY& stat)
	//{
	//	
	//}
}



